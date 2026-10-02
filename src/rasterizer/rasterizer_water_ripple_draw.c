// rasterizer_water_ripple_draw  (Ghidra: FUN_0051ee60)
// address 0x51ee60, size 456 bytes
// name confidence: 0.3   rewrite confidence: 0.85
// evidence: the draw half of rasterizer_water_fade_compute_and_set_states 0x51eb20, which binds
//   the atmospheric and planar fog density maps into effect 112 (0x0069e210) and computes the
//   two factors at 0x006e09f0/0x006e09f4. With the fog latch (rasterizer_fog_enabled
//   0x0069c6a8) set and ps_1_1 or better it sets the declaration of the vertex buffer type, the
//   vertex shader 11 + chimera__shader_get_vertex_shader_permutation(shader), uploads four pixel
//   shader vectors built from the window fog (density x3 with a*density, (1-b)*planar density
//   x3 with b*planar density, atmospheric color with a, planar color with 1-a) and draws the
//   buffer once per effect pass through 0x51c1c0. The code is a per geometry fog layer; the
//   phase 2 "water ripple" name is kept (UNSURE).
//   Spot-check fix (phase 4 review): rewritten from the raw code 0x51ee60..0x51f027. The EAX
//   input is the vertex buffer (its type selects the declaration), not a vertex type, and the
//   draw arguments are the stack arguments.
// register convention: EAX = vertex_buffer, stack = (shader, dynamic_index_slot,
//   first_primitive, primitive_count).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *rasterizer_device;                                     // 0x0071d174
extern d3d_caps9 rasterizer_caps;                                   // 0x007c10c0
extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220
extern rasterizer_effect_slot rasterizer_effects[k_rasterizer_pixel_shader_effects]; // 0x0069d410; effect 112 (0x0069e210)
extern rasterizer_vertex_declaration rasterizer_vertex_declarations[k_rasterizer_vertex_type_count]; // 0x006e1a90
extern rasterizer_vertex_shader rasterizer_vertex_shaders[k_rasterizer_vertex_shaders]; // 0x0069e350
extern uint8_t rasterizer_fog_enabled;                              // 0x0069c6a8 latched by rasterizer_set_fog_constants
extern uint8_t console_debug_toggle_6893e4;                         // 0x006893e4 (some readers compare it as a word)
extern float water_fade_factor_a;                                   // 0x006e09f0
extern float water_fade_factor_b;                                   // 0x006e09f4

// blam-cc: ECX -> shader
extern int16_t chimera__shader_get_vertex_shader_permutation(const Shader *shader); // 0x53fd60
// blam-cc: EAX -> primitive_count, ESI -> vertex_buffer, stack -> (dynamic_index_slot, first_primitive)
extern void chimera__rasterizer_draw_dynamic_triangles_static_vertices(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer,
                                                                       int32_t dynamic_index_slot, int32_t first_primitive); // 0x51c1c0

typedef int32_t (__stdcall *d3d_set_pointer_fn)(void *self, void *object);
typedef int32_t (__stdcall *d3d_set_constant_f_fn)(void *self, uint32_t start_register, const float *data, uint32_t count);
typedef int32_t (__stdcall *d3dx_effect_begin_fn)(void *effect, uint32_t *passes, uint32_t flags);
typedef int32_t (__stdcall *d3dx_effect_pass_fn)(void *effect, uint32_t pass);
typedef int32_t (__stdcall *d3dx_effect_end_fn)(void *effect);

static void **device_vtable(void)
{
    return *(void ***)rasterizer_device;
}

// blam-cc: EAX -> vertex_buffer
void rasterizer_water_ripple_draw(rasterizer_vertex_buffer *vertex_buffer, const Shader *shader, int32_t dynamic_index_slot,
                                  int32_t first_primitive, int32_t primitive_count)
{
    void *effect = (void *)rasterizer_effects[112].effect;
    const render_fog *fog = &rasterizer_window.fog;
    uint8_t succeeded = 1;
    int16_t permutation;
    float constants[16];
    uint32_t pass_count;
    uint32_t pass;

    if (*(uint16_t *)&console_debug_toggle_6893e4 != 0 || rasterizer_fog_enabled == 0 ||
        rasterizer_caps.pixel_shader_version < 0xffff0101 || effect == 0) {
        return;
    }

    if (((d3d_set_pointer_fn)device_vtable()[0x15c / 4])(
            rasterizer_device, (void *)rasterizer_vertex_declarations[vertex_buffer->type].declaration) < 0) {
        succeeded = 0;
    }
    permutation = chimera__shader_get_vertex_shader_permutation(shader);
    if (((d3d_set_pointer_fn)device_vtable()[0x170 / 4])(rasterizer_device,
                                                         (void *)rasterizer_vertex_shaders[11 + permutation].shader) < 0 ||
        !succeeded) {
        return;
    }

    constants[0] = fog->atmospheric_maximum_density;
    constants[1] = fog->atmospheric_maximum_density;
    constants[2] = fog->atmospheric_maximum_density;
    constants[3] = water_fade_factor_a * fog->atmospheric_maximum_density;
    constants[4] = (1.0f - water_fade_factor_b) * fog->planar_maximum_density;
    constants[5] = constants[4];
    constants[6] = constants[4];
    constants[7] = fog->planar_maximum_density * water_fade_factor_b;
    constants[8] = fog->atmospheric_color.red;
    constants[9] = fog->atmospheric_color.green;
    constants[10] = fog->atmospheric_color.blue;
    constants[11] = water_fade_factor_a;
    constants[12] = fog->planar_color.red;
    constants[13] = fog->planar_color.green;
    constants[14] = fog->planar_color.blue;
    constants[15] = 1.0f - water_fade_factor_a;

    ((d3dx_effect_begin_fn)(*(void ***)effect)[0x100 / 4])(effect, &pass_count, 3);
    ((d3d_set_constant_f_fn)device_vtable()[0x1b4 / 4])(rasterizer_device, 0, constants, 4);
    for (pass = 0; pass < pass_count; pass++) {
        ((d3dx_effect_pass_fn)(*(void ***)effect)[0x104 / 4])(effect, pass);
        chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, vertex_buffer, dynamic_index_slot, first_primitive);
    }
    ((d3dx_effect_end_fn)(*(void ***)effect)[0x108 / 4])(effect);
}

#if 0
Original Ghidra decompilation (0x51ee60):

void FUN_0051ee60(void)

{
  undefined4 uVar1;
  undefined4 uVar2;
  short sVar3;
  short *in_EAX;
  int iVar4;
  int iVar5;
  int *piVar6;
  int *piVar7;
  undefined4 uStack_5c;
  int *piStack_58;
  undefined4 uStack_54;

  if ((((DAT_006893e4 == 0) && (DAT_0069c6a8 != '\0')) && (0xffff0100 < DAT_007c118c)) &&
     (DAT_0069e210 != (int *)0x0)) {
    uStack_54 = (&DAT_006e1a90)[*in_EAX * 3];
    piStack_58 = DAT_0071d174;
    uStack_5c = 0x51eed0;
    iVar4 = (**(code **)(*DAT_0071d174 + 0x15c))();
    uStack_5c = 0x51eedf;
    sVar3 = chimera__shader_get_vertex_shader_permutation();
    uStack_5c = *(undefined4 *)(&DAT_0069e3a8 + sVar3 * 8);
    iVar5 = (**(code **)(*DAT_0071d174 + 0x170))(DAT_0071d174);
    uVar2 = DAT_007c1438;
    uVar1 = DAT_006e09f0;
    if ((-1 < iVar5) && (-1 < iVar4)) {
      piVar7 = DAT_0069e210;
      (**(code **)(*DAT_0069e210 + 0x100))(DAT_0069e210,&uStack_54,3);
      (**(code **)(*DAT_0071d174 + 0x1b4))(DAT_0071d174,0,&uStack_5c,4);
      piVar6 = (int *)0x0;
      if (piVar7 != (int *)0x0) {
        do {
          (**(code **)(*DAT_0069e210 + 0x104))(DAT_0069e210,piVar6);
          chimera__rasterizer_draw_dynamic_triangles_static_vertices(uVar1,uVar2);
          piVar6 = (int *)((int)piVar6 + 1);
        } while (piVar6 < piVar7);
      }
      (**(code **)(*DAT_0069e210 + 0x108))(DAT_0069e210);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
