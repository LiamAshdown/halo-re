// chimera__rasterizer_set_frustum_z_func  (Ghidra: chimera__rasterizer_set_frustum_z_func,
// already named -- Chimera name, hint only)
// address 0x518f40, size 704 bytes
// name confidence: 0.55  rewrite confidence: 0.9 (rewritten 2026-09-25 from the disassembly)
// evidence: matches out/phase4/rasterizer_functions.md's summary ("Transforms the current view
//   frustum's clip planes into the object's local space and uploads them, together with
//   fog-plane parameters, as shader/clip-plane constants"); the globals it touches
//   (0x007c127c..0x007c13f4) fall inside rasterizer_window.frustum (types/rasterizer.h,
//   frustum at window+0x5c == 0x007c127c) and the camera fields at 0x007c1228..0x007c123c are
//   rasterizer_window.camera.position/forward.
// register convention: two recognized stack parameters (z-range low/high, per the two literal
//   float-bit callers -0.5/-1.0 and 0/0 elsewhere in this session).
// UNSURE, substantially: this is one of the densest raw-offset matrix functions in the module
//   (a 4x3 * 4x3-shaped multiply-accumulate over the frustum's world_to_view basis). Rather than
//   risk a wrong field-level renaming under this session's time budget, it is transliterated as
//   literally as C allows -- every DAT_ global becomes a plainly named extern at its exact
//   address/type, and every raw pointer arithmetic expression is preserved byte-for-byte -- so
//   the compiled behaviour matches exactly even though the *meaning* of several intermediate
//   floats (local_54, the two SetClipPlane blocks, the two vertex shader constant blocks) is not
//   asserted. This function is the top candidate in this module for a disassembly-verified
//   review pass.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220

extern void *rasterizer_device; // 0x0071d174
extern d3d_caps9 rasterizer_caps; // 0x007c10c0

// Frustum/camera fields, addressed exactly as the decompiled code does (byte offsets from
// rasterizer_window, which starts at 0x007c1220).
extern float g_007c1228, g_007c122c, g_007c1230; // camera.position
extern float g_007c1234, g_007c1238, g_007c123c; // camera.forward
extern float g_007c1290, g_007c1294, g_007c1298, g_007c129c, g_007c12a0, g_007c12a4, g_007c12a8,
    g_007c12ac, g_007c12b0, g_007c12b4, g_007c12b8, g_007c12bc; // frustum.world_to_view (partial)
extern float g_007c12c4, g_007c12c8, g_007c12cc, g_007c12d0, g_007c12d4, g_007c12d8; // frustum.view_to_world forward/left
extern float g_007c13c0[16]; // frustum.projection (4x4)
extern float g_007c13d0, g_007c13d4; // more world_to_view / frustum_bounds fields
extern float g_007c13e0, g_007c13e4, g_007c13f0; // UNSURE, past render_frustum's documented tail

// blam-cc: ECX -> frustum, stack -> (z_near, z_far)
extern void render_camera_projection_zrange_push_pop_set(render_frustum *frustum, float z_near, float z_far); // 0x50c9a0

typedef int32_t (__stdcall *d3d_set_transform_fn)(void *device, uint32_t state, const float *matrix);
typedef int32_t (__stdcall *d3d_set_vertex_shader_constant_fn)(void *device, uint32_t reg, const void *data, uint32_t count);

// REWRITTEN (objdump 0x518f40..0x5191ff, 2026-09-25). The Ghidra-shaped draft passed a code address (0x5190f3)
//   where the original builds an identity matrix, uploaded 0 vertex shader constants instead of 6, and turned
//   the raw z bits into floats by value; hooked, the world was not drawn at all.
// Reads (all inside rasterizer_window, 0x7c1220): view = 4x3 at 0x7c1290 (rows of three floats), projection =
//   4x4 at 0x7c13c0, camera position 0x7c1228 / forward 0x7c1234, and the two rows at 0x7c12c4 / 0x7c12d0.
#define RW(address) (*(const float *)((const uint8_t *)&rasterizer_window + ((address) - 0x7c1220)))

// blam-cc: stack -> z_near, z_far (raw float bits, forwarded unchanged)
void chimera__rasterizer_set_frustum_z_func(uint32_t z_near, uint32_t z_far)
{
    const float *view = &RW(0x7c1290);        // view[row * 3 + column], 4 rows
    const float *projection = &RW(0x7c13c0);  // projection[row * 4 + column]
    float constants[6][4];                    // vertex shader c0..c5
    float rows_1b[2][4];                      // vertex shader c27..c28
    void **vtable;
    int32_t i, j;

    render_camera_projection_zrange_push_pop_set(&rasterizer_window.frustum, *(float *)&z_near, *(float *)&z_far);

    // c0..c3: row i, column j = sum over k of view[j][k] * projection[k][i], plus projection[3][i] in column 3
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            constants[i][j] = view[j * 3 + 0] * projection[0 * 4 + i] + view[j * 3 + 2] * projection[2 * 4 + i] +
                              projection[1 * 4 + i] * view[j * 3 + 1];
        }
        constants[i][3] = projection[3 * 4 + i] + constants[i][3];
    }

    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
        // no pixel shaders: fixed-function transforms. World = identity, view = the 4x3 view matrix widened to
        // 4x4, projection as stored
        float identity[16] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                              0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
        float view4[16];
        for (j = 0; j < 4; j++) {
            view4[j * 4 + 0] = view[j * 3 + 0];
            view4[j * 4 + 1] = view[j * 3 + 1];
            view4[j * 4 + 2] = view[j * 3 + 2];
            view4[j * 4 + 3] = j == 3 ? 1.0f : 0.0f;
        }
        vtable = *(void ***)rasterizer_device;
        ((d3d_set_transform_fn)vtable[0xb0 / 4])(rasterizer_device, 0x100, identity);  // D3DTS_WORLD
        vtable = *(void ***)rasterizer_device;
        ((d3d_set_transform_fn)vtable[0xb0 / 4])(rasterizer_device, 2, view4);         // D3DTS_VIEW
        vtable = *(void ***)rasterizer_device;
        ((d3d_set_transform_fn)vtable[0xb0 / 4])(rasterizer_device, 3, projection);    // D3DTS_PROJECTION
    }

    // c4 = camera position, 2.0; c5 = camera forward, 0.5
    constants[4][0] = RW(0x7c1228); constants[4][1] = RW(0x7c122c); constants[4][2] = RW(0x7c1230); constants[4][3] = 2.0f;
    constants[5][0] = RW(0x7c1234); constants[5][1] = RW(0x7c1238); constants[5][2] = RW(0x7c123c); constants[5][3] = 0.5f;
    vtable = *(void ***)rasterizer_device;
    ((d3d_set_vertex_shader_constant_fn)vtable[0x178 / 4])(rasterizer_device, 0, constants, 6);

    // c27 = (0x7c12c4.., 1.0), c28 = (0x7c12d0.., 3.0)
    rows_1b[0][0] = RW(0x7c12c4); rows_1b[0][1] = RW(0x7c12c8); rows_1b[0][2] = RW(0x7c12cc); rows_1b[0][3] = 1.0f;
    rows_1b[1][0] = RW(0x7c12d0); rows_1b[1][1] = RW(0x7c12d4); rows_1b[1][2] = RW(0x7c12d8); rows_1b[1][3] = 3.0f;
    vtable = *(void ***)rasterizer_device;
    ((d3d_set_vertex_shader_constant_fn)vtable[0x178 / 4])(rasterizer_device, 0x1b, rows_1b, 2);
}
#undef RW

#if 0
Original Ghidra decompilation (0x518f40):

void chimera__rasterizer_set_frustum_z_func(undefined4 param_1,int *param_2)

{
  float *pfVar1;
  float *pfVar2;
  float *pfVar3;
  int iVar4;
  int iVar5;
  float *pfVar6;
  int *piStack_f0;
  int local_ac [4];
  undefined4 local_9c;
  undefined4 local_98;
  undefined4 local_94;
  undefined4 local_90;
  undefined4 local_8c;
  undefined4 local_88;
  undefined4 local_84;
  undefined4 local_80;
  undefined4 local_7c;
  undefined4 local_78;
  undefined4 local_74;
  undefined4 local_70;
  undefined4 local_6c;
  undefined4 local_68;
  undefined4 local_64;
  float local_54 [13];
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 uStack_c;
  undefined4 local_8;
  undefined4 uStack_4;

  piStack_f0 = param_2;
  render_camera_projection_zrange_push_pop_set(param_1);
  pfVar2 = (float *)&DAT_007c13d0;
  iVar4 = 4;
  pfVar6 = local_54;
  do {
    iVar5 = 4;
    pfVar1 = (float *)&DAT_007c1294;
    pfVar3 = pfVar6 + -3;
    do {
      iVar5 = iVar5 + -1;
      *pfVar3 = *pfVar2 * *pfVar1 + pfVar1[1] * pfVar2[4] + pfVar1[-1] * pfVar2[-4];
      pfVar1 = pfVar1 + 3;
      pfVar3 = pfVar3 + 1;
    } while (iVar5 != 0);
    pfVar1 = pfVar2 + 8;
    pfVar2 = pfVar2 + 1;
    iVar4 = iVar4 + -1;
    *pfVar6 = *pfVar1 + *pfVar6;
    pfVar6 = pfVar6 + 4;
  } while (iVar4 != 0);
  if (DAT_007c118c < 0xffff0101) {
    local_98 = DAT_007c1298;
    local_9c = DAT_007c1294;
    local_88 = DAT_007c12a4;
    local_ac[3] = DAT_007c1290;
    local_8c = DAT_007c12a0;
    local_78 = DAT_007c12b0;
    local_90 = DAT_007c129c;
    local_7c = DAT_007c12ac;
    local_68 = DAT_007c12bc;
    local_80 = DAT_007c12a8;
    local_6c = DAT_007c12b8;
    local_94 = 0;
    local_84 = 0;
    local_74 = 0;
    local_70 = DAT_007c12b4;
    local_64 = 0x3f800000;
    local_ac[1] = 0;
    local_ac[0] = 0;
    local_ac[2] = 0x3f800000;
    piStack_f0 = (int *)0x5190f3;
    (**(code **)(*DAT_0071d174 + 0xb0))();
    piStack_f0 = local_ac;
    (**(code **)(*DAT_0071d174 + 0xb0))(DAT_0071d174,2);
    (**(code **)(*DAT_0071d174 + 0xb0))(DAT_0071d174,3,&DAT_007c13c0);
  }
  local_18 = DAT_007c1230;
  local_20 = DAT_007c1228;
  local_8 = DAT_007c123c;
  local_1c = DAT_007c122c;
  local_10 = DAT_007c1234;
  local_14 = 0x40000000;
  uStack_c = DAT_007c1238;
  uStack_4 = 0x3f000000;
  piStack_f0 = DAT_0071d174;
  (**(code **)(*DAT_0071d174 + 0x178))();
  piStack_f0 = DAT_007c12c4;
  (**(code **)(*DAT_0071d174 + 0x178))(DAT_0071d174,0x1b,&piStack_f0,2);
  return;
}
#endif
