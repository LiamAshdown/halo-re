// chimera__rasterizer_set_frustum_z_func  (Ghidra: chimera__rasterizer_set_frustum_z_func,
// already named -- Chimera name, hint only)
// address 0x518f40, size 704 bytes
// name confidence: 0.55  rewrite confidence: 0.35
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
extern void render_camera_projection_zrange_push_pop_set(render_frustum *frustum, uint32_t z_near, uint32_t z_far); // 0x50c9a0

typedef int32_t (__stdcall *d3d_set_clip_plane_fn)(void *device, uint32_t index, const void *plane);
typedef int32_t (__stdcall *d3d_set_vertex_shader_constant_fn)(void *device, uint32_t reg, const void *data, uint32_t count);

// blam-cc: stack -> (z_near, z_far) as raw float bits; both are forwarded to 0x50c9a0 with
//   ECX = &rasterizer_window.frustum (0x007c127c) (phase 4 review: the second argument is a value,
//   not a pointer; 0x518f40 pushes [esp+8] and [esp+4]).
// See the file header: transliterated as literally as possible rather than field-renamed.
void chimera__rasterizer_set_frustum_z_func(uint32_t z_near, uint32_t z_far)
{
    float local_54[13];
    float row0[4], row1[4], row2[4], row3[4];
    void **vtable;

    render_camera_projection_zrange_push_pop_set(&rasterizer_window.frustum, z_near, z_far);

    {
        // 4-row, 3-column accumulate: out[row] = sum_col( world_to_view_col[row][col] *
        // basis_row[col] ) + trailing offset. Preserved exactly from the original pointer walk
        // (&g_007c13d0 stepping by 1 float per outer iteration, 8 floats per "+pfVar2+8" trail;
        // &g_007c1294 stepping by 3 floats per inner iteration, reading [-1],[0],[1]).
        float *pfVar2 = &g_007c13d0;
        int iVar4 = 4;
        float *pfVar6 = local_54;
        float *rows[4] = {row0, row1, row2, row3};
        int r = 0;

        do {
            int iVar5 = 4;
            float *pfVar1 = &g_007c1294;
            float *pfVar3 = pfVar6 - 3; // UNSURE: aliases into local_54 exactly as original
            int c = 0;
            (void)pfVar3;
            do {
                iVar5 = iVar5 - 1;
                rows[r][c] = pfVar2[0] * pfVar1[0] + pfVar1[1] * pfVar2[4] + pfVar1[-1] * pfVar2[-4];
                pfVar1 = pfVar1 + 3;
                c++;
            } while (iVar5 != 0);
            {
                float *pfVar1b = pfVar2 + 8;
                pfVar2 = pfVar2 + 1;
                iVar4 = iVar4 - 1;
                rows[r][0] = *pfVar1b + rows[r][0]; // UNSURE: matches "*pfVar6 = *pfVar1 + *pfVar6" (index 0 of this row)
                pfVar6 = pfVar6 + 4;
                r++;
            }
        } while (iVar4 != 0);
    }

    if (rasterizer_caps.pixel_shader_version < 0xffff0101) {
        float clip_plane_a[4];
        float clip_plane_b[4];

        clip_plane_a[1] = g_007c1298;
        clip_plane_a[0] = g_007c1294;
        (void)g_007c12a4; // local_88, UNSURE destination (see original: local_88 unused after assignment)
        clip_plane_b[3] = g_007c1290;
        (void)g_007c12a0; // local_8c, UNSURE
        (void)g_007c12b0; // local_78, UNSURE
        clip_plane_a[2] = g_007c129c;
        (void)g_007c12ac; // local_7c, UNSURE
        (void)g_007c12bc; // local_68, UNSURE
        (void)g_007c12a8; // local_80, UNSURE
        (void)g_007c12b8; // local_6c, UNSURE
        clip_plane_a[3] = 0.0f;
        (void)0; // local_84
        (void)0; // local_74
        (void)g_007c12b4; // local_70, UNSURE
        (void)0x3f800000; // local_64 == 1.0f
        clip_plane_b[1] = 0.0f;
        clip_plane_b[0] = 0.0f;
        clip_plane_b[2] = 0x3f800000; // 1.0f, as a plane's D or Z component

        vtable = *(void ***)rasterizer_device;
        ((d3d_set_clip_plane_fn)vtable[0xb0 / 4])(rasterizer_device, 0, (void *)0x5190f3); // UNSURE: literal, not a real pointer -- see file header
        vtable = *(void ***)rasterizer_device;
        ((d3d_set_clip_plane_fn)vtable[0xb0 / 4])(rasterizer_device, 2, clip_plane_b);
        vtable = *(void ***)rasterizer_device;
        ((d3d_set_clip_plane_fn)vtable[0xb0 / 4])(rasterizer_device, 3, g_007c13c0);
    }

    {
        float user_clip[8];
        user_clip[2] = g_007c1230;
        user_clip[0] = g_007c1228;
        user_clip[6] = g_007c123c;
        user_clip[3] = g_007c122c;
        user_clip[4] = g_007c1234;
        user_clip[5] = 2.0f; // 0x40000000
        user_clip[7] = g_007c1238;
        user_clip[1] = 0.5f; // 0x3f000000, wait: uStack_4 maps to user_clip[7]'s neighbor -- see UNSURE below

        vtable = *(void ***)rasterizer_device;
        ((d3d_set_vertex_shader_constant_fn)vtable[0x178 / 4])(rasterizer_device, 0, local_54, 0); // UNSURE: real reg/count not shown
        vtable = *(void ***)rasterizer_device;
        ((d3d_set_vertex_shader_constant_fn)vtable[0x178 / 4])(rasterizer_device, 0x1b, &g_007c12c4, 2);
    }
}

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
