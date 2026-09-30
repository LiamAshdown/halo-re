// rasterizer_sun_glow_project_point  (Ghidra: FUN_00525130, unnamed)
// address 0x525130, size 483 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: out/phase2/results/rasterizer_02.json: "Transforms a world point with
//   matrix4x3_transform_point using the matrix at DAT_007c128c, then applies a perspective-style
//   divide against the projection constants at DAT_007c13c0-0x13fc, writing screen-space xy/depth
//   to param_3 and a 2-component scale to param_4; used by rasterizer_light_shadow_render." The
//   globals resolve against types/rasterizer.h: 0x007c1254/0x007c1258 is
//   rasterizer_window.camera.viewport_bounds (top/left/bottom/right), 0x007c128c is
//   rasterizer_window.frustum.world_to_view, and 0x007c13c0..0x007c13fc is the 4x4
//   rasterizer_window.frustum.projection matrix (row major, so the "column" reads below are
//   `x*row0[c] + y*row1[c] + z*row2[c] + row3[c]`, the standard row-vector * matrix convention).
// register convention: UNSURE overall -- Ghidra recognizes all four parameters without any
//   in_EAX/in_ECX markers (unlike most functions in this file), which is this codebase's usual
//   signal for a plain stack-passed call; declared that way here.
//   // blam-cc: stack -> (point, radius, out_screen, out_scale)
// UNSURE: the exact meaning of the two out_scale components (labelled scale_x/scale_y here) is
//   inferred from the arithmetic (radius reprojected through the same projection columns used for
//   x/y, scaled by 1/w and the viewport dimensions) but not confirmed against a caller.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern rasterizer_window_parameters rasterizer_window; // 0x007c1220

extern void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m); // 0x4cbde0

// Projects a world-space point and radius into screen space for the projected dynamic-light/
// shadow renderer. Returns 0 (without writing the outputs) when the radius is non-positive or the
// point lies behind the light's near plane (projected w <= 0).
uint8_t rasterizer_sun_glow_project_point(real_point3d *point, float radius, float *out_screen,
                                                 float *out_scale)
{
    int16_t viewport_width;
    int16_t viewport_height;
    real_point3d view;
    float proj_y, proj_w, proj_x0, proj_scale_x0, proj_scale_y0;
    float inv_w;

    if (radius <= 0.0f) {
        return 0;
    }

    viewport_width = rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left;
    viewport_height = rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top;

    matrix4x3_transform_point(&view, point, &rasterizer_window.frustum.world_to_view);

    proj_y = rasterizer_window.frustum.projection[0][1] * view.x +
             rasterizer_window.frustum.projection[1][1] * view.y +
             rasterizer_window.frustum.projection[2][1] * view.z +
             rasterizer_window.frustum.projection[3][1];
    proj_w = rasterizer_window.frustum.projection[0][2] * view.x +
             rasterizer_window.frustum.projection[1][2] * view.y +
             rasterizer_window.frustum.projection[2][2] * view.z +
             rasterizer_window.frustum.projection[3][2];
    proj_scale_x0 = rasterizer_window.frustum.projection[0][0] * radius;
    proj_scale_y0 = rasterizer_window.frustum.projection[1][1] * radius;

    if (0.0f < proj_w) {
        float proj_depth;
        float scale_y_radius;

        inv_w = 1.0f / (rasterizer_window.frustum.projection[0][3] * view.x +
                          rasterizer_window.frustum.projection[1][3] * view.y +
                          rasterizer_window.frustum.projection[2][3] * view.z +
                          rasterizer_window.frustum.projection[3][3]);

        proj_x0 = rasterizer_window.frustum.projection[0][0] * view.x +
                   rasterizer_window.frustum.projection[1][0] * view.y +
                   rasterizer_window.frustum.projection[2][0] * view.z +
                   rasterizer_window.frustum.projection[3][0];
        out_screen[0] = ((proj_x0 * inv_w + 1.0f) * (float)viewport_width - 1.0f) * 0.5f;

        out_screen[1] = ((1.0f - inv_w * proj_y) * (float)viewport_height - 1.0f) * 0.5f;

        proj_depth = inv_w * proj_w;
        if (1.0f <= proj_depth) {
            proj_depth = 1.0f;
        }
        out_screen[2] = proj_depth;

        scale_y_radius = proj_scale_y0;
        out_scale[0] = (float)viewport_width * inv_w * proj_scale_x0 * 0.5f;
        out_scale[1] = (float)viewport_height * inv_w * scale_y_radius * 0.5f;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x525130):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined2 FUN_00525130(undefined4 param_1,float param_2,float *param_3,float *param_4)

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
    *param_4 = (float)(int)sVar8 * fVar7 * fVar6 * 0.5;
    param_4[1] = fVar3 * fVar7 * param_2 * 0.5;
    return 1;
  }
  return 0;
}
#endif
