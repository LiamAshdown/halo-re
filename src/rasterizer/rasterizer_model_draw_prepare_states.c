// rasterizer_model_draw_prepare_states  (Ghidra: FUN_00526f50, unnamed)
// address 0x526f50, size 1456 bytes
// name confidence: 0.45   rewrite confidence: 0.8
// evidence: cross-checked against `objdump -d -Mintel --start-address=0x526f50
//   --stop-address=0x527500 bin/halo.exe`, since Ghidra's own decompile shows the first ~20
//   SetSamplerState/SetTransform calls with zero visible arguments (its usual failure mode for
//   this module's vtable-indirect D3D9 calls). The disassembly confirms: ten unconditional
//   IDirect3DDevice9::SetSamplerState (vtable+0x114) calls -- (stage,type,value) (0,1,1) (0,2,1)
//   (0,5,2) (0,6,2) (0,7,2) (1,1,1) (1,2,1) (1,5,2) (1,6,2) (1,7,2) -- filtering/wrap/mip states
//   for stages 0 and 1, then (gated on pixel_shader_version > ps_1_1) eleven more for stages 2/3,
//   matching exactly the literal argument triples Ghidra *did* recover for those later calls
//   ((2,1,1) (2,2,1) (2,5,2) (2,6,2) (2,7,2) (3,1,3) (3,2,3) (3,3,3) (3,5,2) (3,6,2) (3,7,2)),
//   which confirms the reconstructed pattern for the first ten. Field offsets on the ESI
//   parameter match rasterizer_model_draw_context exactly (+0x08 node_matrices, +0x10 lighting,
//   +0x8c group_parameters.mode, +0x90 group_parameters.blend_factor, +0xb4 center), and the
//   fog/camera globals resolve against rasterizer_window.fog/.camera (types/rasterizer.h). The
//   trailing arithmetic (planar fog blend factor into the undocumented global 0x007c047c) was
//   spot-checked against the disassembly's x87 sequence and matches the Ghidra decompile's shape,
//   so that part is taken from the decompile directly with field names substituted for raw
//   offsets.
// register convention: model draw context pointer in ESI, a single mode byte on the stack.
//   // blam-cc: ESI -> context, stack -> mode
// UNSURE: 0x006893ec, 0x00689421, 0x0071d1f4, 0x0071d1fb, 0x0071d1fc, 0x0071d1fd and 0x007c047c
//   are not documented by types/rasterizer.h's global list; declared here with descriptive names
//   and an UNSURE note each. chimera__rasterizer_set_model_skinning's `upload` argument
//   (`~(context->flags >> 8) & 1`) is UNSURE beyond matching the node_parts flag bit's complement.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *rasterizer_device;                       // 0x0071d174
extern d3d_caps9 rasterizer_caps;                      // 0x007c10c0
extern rasterizer_window_parameters rasterizer_window; // 0x007c1220

extern uint8_t console_debug_toggle_6893ec;            // 0x006893ec UNSURE: gates the frustum-z/shader-stage refresh below
extern uint8_t console_debug_toggle_689421;            // 0x00689421 UNSURE: gates the skinning/lighting-constants refresh below
extern uint32_t rasterizer_frustum_z_values[2];        // 0x0069c664

extern rasterizer_model_draw_context *rasterizer_active_model_context; // 0x0071d1f0
extern uint8_t rasterizer_model_scratch_valid; // 0x0071d1f4
extern uint8_t unknown_0071d1fb;              // 0x0071d1fb UNSURE: planar-fog-active flag for this model
extern uint8_t unknown_0071d1fc;              // 0x0071d1fc UNSURE
extern uint8_t unknown_0071d1fd;              // 0x0071d1fd UNSURE: copy of the incoming mode byte
extern int16_t rasterizer_active_model_mode;  // 0x0071d1f8
extern float unknown_007c047c;                // 0x007c047c UNSURE: planar fog blend factor output

extern void rasterizer_set_shader_stage_config(int16_t mode); // 0x519200, blam-cc: EAX -> mode
extern void chimera__rasterizer_set_frustum_z_func(uint32_t z_near, uint32_t z_far); // 0x518f40
extern void chimera__rasterizer_set_model_skinning(uint8_t upload, rasterizer_node_matrices *nodes); // 0x518b40
extern void rasterizer_prepare_lighting_constants(render_lighting *lighting); // 0x518ce0

typedef int32_t (__stdcall *d3d_set_sampler_state_fn)(void *device, uint32_t sampler, uint32_t type, uint32_t value);
typedef int32_t (__stdcall *d3d_set_transform_fn)(void *device, uint32_t state, const void *matrix);

// FIXED (objdump 0x5273c7..0x5274e4): every clamp is fcom 0 / test ah,5 / jp then fcom 1 / test ah,0x41 / jne, which
// only replaces a value strictly below 0 or strictly above 1 -- a NaN (e.g. 0 * inf when the planar fog depth is 0)
// passes through to 0x007c047c unchanged. `0.0f <= x ? .. : 0` turned it into 0.
#define CLAMP01_X87(x) do { if ((x) < 0.0f) (x) = 0.0f; else if ((x) > 1.0f) (x) = 1.0f; } while (0)

static void set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    void **vtable = *(void ***)rasterizer_device;
    ((d3d_set_sampler_state_fn)vtable[0x114 / 4])(rasterizer_device, sampler, type, value);
}

// blam-cc: ESI -> context, stack -> mode
// Configures the fixed-function texture-coordinate filtering and, once per model, skinning/
// lighting/planar-fog/frustum-z render states used before drawing model geometry.
void rasterizer_model_draw_prepare_states(rasterizer_model_draw_context *context, uint8_t mode)
{
    set_sampler_state(0, 1, 1);
    set_sampler_state(0, 2, 1);
    set_sampler_state(0, 5, 2);
    set_sampler_state(0, 6, 2);
    set_sampler_state(0, 7, 2);
    set_sampler_state(1, 1, 1);
    set_sampler_state(1, 2, 1);
    set_sampler_state(1, 5, 2);
    set_sampler_state(1, 6, 2);
    set_sampler_state(1, 7, 2);

    if (0xffff0100 < rasterizer_caps.pixel_shader_version) {
        set_sampler_state(2, 1, 1);
        set_sampler_state(2, 2, 1);
        set_sampler_state(2, 5, 2);
        set_sampler_state(2, 6, 2);
        set_sampler_state(2, 7, 2);
        set_sampler_state(3, 1, 3);
        set_sampler_state(3, 2, 3);
        set_sampler_state(3, 3, 3);
        set_sampler_state(3, 5, 2);
        set_sampler_state(3, 6, 2);
        set_sampler_state(3, 7, 2);
    }

    if (console_debug_toggle_6893ec != 0) {
        if ((int8_t)context->flags < 0 && mode == 0) {
            rasterizer_set_shader_stage_config(1);
            chimera__rasterizer_set_frustum_z_func(rasterizer_frustum_z_values[0], rasterizer_frustum_z_values[1]);
        }

        rasterizer_model_scratch_valid = 0;
        unknown_0071d1fd = mode;
        rasterizer_active_model_context = context;

        if (console_debug_toggle_689421 == 0 || rasterizer_window.type != 1 ||
            context->group_parameters.mode != 1 || !(context->group_parameters.blend_factor > 0.0f)) {
            if (context->group_parameters.mode == 2) {
                rasterizer_active_model_mode = 2;
            } else {
                chimera__rasterizer_set_model_skinning((uint8_t)(~(context->flags >> 8) & 1),
                                                        (rasterizer_node_matrices *)&context->node_matrices);
                rasterizer_prepare_lighting_constants(&context->lighting);
                rasterizer_active_model_mode = 0;
            }
        } else {
            rasterizer_active_model_mode = 1;
        }

        // FIXED (objdump 0x5271ee..0x5271f9): fcomp 0 / test ah,5 / jp clears the flag when the camera side is >= 0
        // OR unordered; only a strictly negative distance sets it
        if (rasterizer_window.fog.planar_mode == 0 || (context->flags & 4) != 0 ||
            ((context->flags & 0x40) != 0 &&
             !((rasterizer_window.camera.position.x * rasterizer_window.fog.plane.normal.i +
                rasterizer_window.camera.position.y * rasterizer_window.fog.plane.normal.j +
                rasterizer_window.camera.position.z * rasterizer_window.fog.plane.normal.k) -
                   rasterizer_window.fog.plane.d < 0.0f))) {
            unknown_0071d1fb = 0;
        } else {
            unknown_0071d1fb = 1;
        }
        unknown_0071d1fc = 0;

        if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
            if ((context->flags & 0x200) != 0) {
                float world_matrix[16];
                float *m = (float *)context->node_matrices;

                world_matrix[0] = m[1];
                world_matrix[1] = m[2];
                world_matrix[2] = m[3];
                world_matrix[3] = 0.0f;
                world_matrix[4] = m[4];
                world_matrix[5] = m[5];
                world_matrix[6] = m[6];
                world_matrix[7] = 0.0f;
                world_matrix[8] = m[7];
                world_matrix[9] = m[8];
                world_matrix[10] = m[9];
                world_matrix[11] = 0.0f;
                world_matrix[12] = m[10] * m[0];
                world_matrix[13] = m[11] * m[0];
                world_matrix[14] = m[12] * m[0];
                world_matrix[15] = 1.0f;

                {
                    void **vtable = *(void ***)rasterizer_device;
                    ((d3d_set_transform_fn)vtable[0xb0 / 4])(rasterizer_device, 0x100 /* D3DTS_WORLD */, world_matrix);
                }
                return;
            }
        } else {
            float inv_depth = 1.0f / rasterizer_window.fog.planar_maximum_depth;
            float inv_distance = 1.0f / rasterizer_window.fog.planar_maximum_distance;
            float density_from_depth;
            float density_from_distance;
            float blend;
            float plane_distance;
            float density_limit;

            density_from_depth = 1.0f - (rasterizer_window.fog.plane.d * inv_depth +
                                          -(rasterizer_window.fog.plane.normal.k * inv_depth) * context->center.z +
                                          -(rasterizer_window.fog.plane.normal.j * inv_depth) * context->center.y +
                                          -(inv_depth * rasterizer_window.fog.plane.normal.i) * context->center.x);
            CLAMP01_X87(density_from_depth);

            density_from_distance = 1.0f -
                (rasterizer_window.camera.forward.j * inv_distance * context->center.y +
                 rasterizer_window.camera.forward.k * inv_distance * context->center.z +
                 rasterizer_window.camera.forward.i * inv_distance * context->center.x -
                 (rasterizer_window.camera.position.x * rasterizer_window.camera.forward.i +
                  rasterizer_window.camera.position.y * rasterizer_window.camera.forward.j +
                  rasterizer_window.camera.position.z * rasterizer_window.camera.forward.k) * inv_distance);
            CLAMP01_X87(density_from_distance);

            blend = density_from_depth + density_from_distance;
            if (1.0f < blend) blend = 1.0f;
            blend = (1.0f - blend) * (1.0f - blend);

            plane_distance = -(((rasterizer_window.camera.position.x * rasterizer_window.fog.plane.normal.i +
                                 rasterizer_window.camera.position.y * rasterizer_window.fog.plane.normal.j +
                                 rasterizer_window.camera.position.z * rasterizer_window.fog.plane.normal.k) -
                                rasterizer_window.fog.plane.d) * inv_depth);
            CLAMP01_X87(plane_distance);

            density_limit = rasterizer_window.fog.planar_maximum_density;
            CLAMP01_X87(density_limit);

            unknown_007c047c = 1.0f - density_limit *
                (plane_distance * ((1.0f - density_from_distance) * (1.0f - density_from_distance) - blend) + blend);
        }
    }
}

#if 0
Original Ghidra decompilation (0x526f50):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00526f50(void)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  uint *unaff_ESI;
  float fVar5;
  float fStack_e0;
  float fStack_dc;
  int *piStack_d8;
  undefined4 uStack_d4;
  float fStack_d0;
  float fStack_cc;
  int *piStack_c8;
  undefined4 uStack_c4;
  float fStack_c0;
  float fStack_bc;
  int *piStack_b8;
  undefined4 uStack_b4;
  float fStack_b0;
  float fStack_ac;
  int *piStack_a8;
  undefined4 uStack_a4;
  undefined4 uStack_a0;
  undefined4 uStack_9c;
  int *piStack_98;
  undefined4 uStack_94;
  undefined4 uStack_90;
  undefined4 uStack_8c;
  int *piStack_88;
  undefined4 uStack_84;
  undefined4 uStack_80;
  undefined4 uStack_7c;
  int *piStack_78;
  undefined4 uStack_74;
  undefined4 uStack_70;
  undefined4 uStack_6c;
  int *piStack_68;
  undefined4 uStack_64;
  undefined4 uStack_60;
  undefined4 uStack_5c;

  [see out/phase2/rasterizer/02.md or `python tools/pack.py 0x526f50` for the full decompile;
  the SetSamplerState/SetTransform call arguments it shows are unreliable, see file header --
  the true call sequence is `objdump -d -Mintel --start-address=0x526f50
  --stop-address=0x527500 bin/halo.exe`.]
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
