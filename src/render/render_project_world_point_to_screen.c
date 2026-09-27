// render_project_world_point_to_screen  (Ghidra: render_project_world_point_to_screen, already
// named)
// address 0x50de30, size 234 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x50de30..0x50df19: z / clip range tests, 640 x 480 virtual screen, viewport left/top at camera +0x2e/+0x2c)
// evidence: src/interface/hud_waypoint_draw_one.c already documents this exact call site: "then
//   projected by 0x50de30 (ECX screen point out, EDX view point, ESI 0x007c3168, EDI the camera at
//   0x007c3114)", i.e. ECX=screen_out, EDX=world_point, ESI=frustum, EDI=camera. The projection
//   fields (+0x144/0x158/0x164/0x168) are render_frustum.projection[0][0]/[1][1]/[2][0]/[2][1],
//   the same four this module's render_frustum_compute_screen_clip_bounds (0x50ddc0) reads,
//   and the final screen-space scale (640/480) matches
//   k_render_virtual_screen_width/k_render_virtual_screen_height (types/render.h).
// register convention: ECX = screen_out (real_point2d*), EDX = world_point (real_point3d*),
//   ESI = frustum (render_frustum*), EDI = camera (render_camera*).
//   // blam-cc: ECX=screen_out, EDX=world_point, ESI=frustum, EDI=camera
// UNSURE: Ghidra renders every comparison as a packed x87-status "class" value (uVar4); every
//   such value it can return has its low bit clear, and the one success path returns a value
//   whose low byte is 1, so this is a plain boolean success/failure result -- reproduced as such.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"

// Projects a world-space point into the camera's normalized device coordinates and then into
// screen pixel coordinates, returning whether it lies within the view (in front of the camera,
// i.e. negative view-space z, and inside the -1..1 clip range on both axes).
uint8_t render_project_world_point_to_screen(real_point2d *screen_out, real_point3d *world_point,
                                               render_frustum *frustum, render_camera *camera)
    // blam-cc: ECX=screen_out, EDX=world_point, ESI=frustum, EDI=camera
{
    float inverse_z;
    float clip_x, clip_y;

    if (world_point->z >= 0.0f) {
        return 0;
    }

    inverse_z = -1.0f / world_point->z;
    clip_x = (frustum->projection[2][0] * world_point->z + frustum->projection[0][0] * world_point->x) * inverse_z;
    clip_y = -((frustum->projection[2][1] * world_point->z + frustum->projection[1][1] * world_point->y) * inverse_z);

    // 0x50de66 / 0x50de8a: the clip-space point is stored before the range test, so a caller still gets it
    // for a point off screen (the return value says whether it was converted to screen pixels)
    screen_out->x = clip_x;
    screen_out->y = clip_y;
    if (clip_x < -1.0f || clip_x > 1.0f || clip_y < -1.0f || clip_y > 1.0f) {
        return 0;
    }

    screen_out->x = (screen_out->x + 1.0f) * 0.5f * (float)k_render_virtual_screen_width +
                    (float)camera->viewport_bounds.left;
    screen_out->y = (screen_out->y + 1.0f) * 0.5f * (float)k_render_virtual_screen_height +
                    (float)camera->viewport_bounds.top;
    return 1;
}

#if 0
Original Ghidra decompilation (0x50de30):

uint render_project_world_point_to_screen(void)

{
  float fVar1;
  short sVar2;
  float fVar3;
  uint uVar4;
  float *in_ECX;
  float *in_EDX;
  int unaff_ESI;
  int unaff_EDI;

  fVar1 = in_EDX[2];
  uVar4 = (uint)(ushort)((ushort)(fVar1 < 0.0) << 8 | (ushort)NAN(fVar1) << 10 |
                        (ushort)(fVar1 == 0.0) << 0xe);
  if (fVar1 < 0.0) {
    fVar1 = in_EDX[2];
    *in_ECX = (*(float *)(unaff_ESI + 0x164) * in_EDX[2] + *(float *)(unaff_ESI + 0x144) * *in_EDX)
              * (-1.0 / fVar1);
    fVar3 = -((*(float *)(unaff_ESI + 0x168) * in_EDX[2] + *(float *)(unaff_ESI + 0x158) * in_EDX[1]
              ) * (-1.0 / fVar1));
    in_ECX[1] = fVar3;
    fVar1 = *in_ECX;
    uVar4 = (uint)(ushort)((ushort)(fVar1 < -1.0) << 8 | (ushort)NAN(fVar1) << 10 |
                          (ushort)(fVar1 == -1.0) << 0xe);
    if (fVar1 >= -1.0) {
      fVar1 = *in_ECX;
      uVar4 = (uint)(ushort)((ushort)(fVar1 < 1.0) << 8 | (ushort)NAN(fVar1) << 10 |
                            (ushort)(fVar1 == 1.0) << 0xe);
      if (fVar1 < 1.0 != (fVar1 == 1.0)) {
        uVar4 = (uint)(ushort)((ushort)(fVar3 < -1.0) << 8 | (ushort)NAN(fVar3) << 10 |
                              (ushort)(fVar3 == -1.0) << 0xe);
        if (fVar3 >= -1.0) {
          uVar4 = (uint)(ushort)((ushort)(fVar3 < 1.0) << 8 | (ushort)NAN(fVar3) << 10 |
                                (ushort)(fVar3 == 1.0) << 0xe);
          if (fVar3 < 1.0 != (fVar3 == 1.0)) {
            *in_ECX = (*in_ECX + 1.0) * 0.5 * 640.0 + (float)(int)*(short *)(unaff_EDI + 0x2e);
            sVar2 = *(short *)(unaff_EDI + 0x2c);
            in_ECX[1] = (in_ECX[1] + 1.0) * 0.5 * 480.0 + (float)(int)sVar2;
            return CONCAT31((int3)(char)((ushort)sVar2 >> 8),1);
          }
        }
      }
    }
  }
  return uVar4;
}
#endif
