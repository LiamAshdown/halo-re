// rasterizer_lens_flare_project_to_screen  (Ghidra: rasterizer_lens_flare_project_to_screen, already named)
// address 0x536d80, size 491 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: functions.md summary ("Projects a world-space point and radius into screen-space
//   position plus billboard size, used to place a screen-space sprite (lens flare / decal) quad");
//   the 0x007c13c0..0x007c13fc block is exactly rasterizer_window.frustum.projection[4][4], and
//   0x007c128c is frustum.world_to_view (both "(used)" in types/rasterizer.h).
// register convention: none; stack -> (position, radius, out_screen (3 floats: x, y, depth),
//   out_inverse_w, out_billboard_size (2 floats)).
// blam-cc: stack -> (position, radius, out_screen, out_inverse_w, out_billboard_size)
//   Phase 4 review: both callers (0x537584, 0x537843) push all five arguments (add esp,0x14)
//   and 0x536d80 reads the position from [esp+4]; the earlier EAX note was wrong.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern rasterizer_window_parameters rasterizer_window; // 0x007c1220

extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x004cbde0

// Projects a world-space point and radius into screen-space position plus billboard size, used to
// place a screen-space sprite (lens flare / decal) quad.
uint8_t rasterizer_lens_flare_project_to_screen(const real_point3d *position, float radius, float *out_screen,
                                                 float *out_inverse_w, float *out_billboard_size)
{
    real_point3d view_point;
    float clip_w, clip_z;
    float projected_x, projected_y;
    int16_t width, height;
    const float (*proj)[4] = rasterizer_window.frustum.projection;

    if (radius <= 0.0f) {
        return 0;
    }

    width = (int16_t)(rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left);
    height = (int16_t)(rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top);

    matrix4x3_transform_point(&view_point, position, &rasterizer_window.frustum.world_to_view);

    projected_x = proj[0][1] * view_point.x + proj[1][1] * view_point.y + proj[2][1] * view_point.z + proj[3][1];
    clip_w = proj[0][2] * view_point.x + proj[1][2] * view_point.y + proj[2][2] * view_point.z + proj[3][2];
    projected_y = proj[0][0] * radius;
    radius = proj[1][1] * radius;

    if (clip_w > 0.0f) {
        float inv_w = 1.0f / (proj[0][3] * view_point.x + proj[1][3] * view_point.y + proj[2][3] * view_point.z +
                              proj[3][3]);
        float screen_x = (((proj[0][0] * view_point.x + proj[1][0] * view_point.y + proj[2][0] * view_point.z +
                            proj[3][0]) * inv_w + 1.0f) * (float)width - 1.0f) * 0.5f;
        float height_f = (float)height;
        float screen_y = ((1.0f - inv_w * projected_x) * height_f - 1.0f) * 0.5f;
        float depth = inv_w * clip_w;

        if (depth >= 1.0f) {
            depth = 1.0f;
        }

        out_screen[0] = screen_x;
        out_screen[1] = screen_y;
        out_screen[2] = depth;
        *out_inverse_w = inv_w;
        out_billboard_size[0] = (float)width * inv_w * projected_y * 0.5f;
        out_billboard_size[1] = height_f * inv_w * radius * 0.5f;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x536d80):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined2
rasterizer_lens_flare_project_to_screen
          (undefined4 param_1,float param_2,float *param_3,float *param_4,float *param_5)

{
  short sVar1;
  short sVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  short sVar8;
  float local_c;
  float local_8;
  float local_4;

  if (param_2 <= 0.0) {
    return 0;
  }
  sVar8 = DAT_007c1258._2_2_ - DAT_007c1254._2_2_;
  sVar2 = (short)DAT_007c1254;
  sVar1 = (short)DAT_007c1258;
  matrix4x3_transform_point(&DAT_007c128c);
  fVar4 = DAT_007c13c4 * local_c + DAT_007c13d4 * local_8 + DAT_007c13e4 * local_4 + _DAT_007c13f4;
  fVar5 = _DAT_007c13c8 * local_c + _DAT_007c13d8 * local_8 + _DAT_007c13e8 * local_4 +
          _DAT_007c13f8;
  fVar6 = DAT_007c13c0 * param_2;
  param_2 = DAT_007c13d4 * param_2;
  if (0.0 < fVar5) {
    fVar7 = 1.0 / (_DAT_007c13cc * local_c + _DAT_007c13dc * local_8 + _DAT_007c13ec * local_4 +
                  _DAT_007c13fc);
    *param_3 = (((DAT_007c13c0 * local_c + DAT_007c13d0 * local_8 + DAT_007c13e0 * local_4 +
                 DAT_007c13f0) * fVar7 + 1.0) * (float)(int)sVar8 - 1.0) * 0.5;
    fVar3 = (float)(int)(short)(sVar1 - sVar2);
    param_3[1] = ((1.0 - fVar7 * fVar4) * fVar3 - 1.0) * 0.5;
    fVar5 = fVar7 * fVar5;
    if (1.0 <= fVar5) {
      fVar5 = 1.0;
    }
    param_3[2] = fVar5;
    *param_4 = fVar7;
    *param_5 = (float)(int)sVar8 * fVar7 * fVar6 * 0.5;
    param_5[1] = fVar3 * fVar7 * param_2 * 0.5;
    return 1;
  }
  return 0;
}
#endif
