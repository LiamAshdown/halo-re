// render_frustum_classify_point_side_planes  (Ghidra: render_frustum_classify_point_side_planes,
// already named)
// address 0x50d4c0, size 237 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: types/render.h's render_frustum_point_flags enum documents this exact function's four
//   output bits against the four side planes' offsets (world_planes[0] +0x78 -> bit 0x01,
//   world_planes[1] +0x88 -> bit 0x02, world_planes[3] +0xa8 -> bit 0x04, world_planes[2] +0x98
//   -> bit 0x08), which matches the four dot-product tests below exactly.
// register convention: ECX = frustum (render_frustum*), EDX = point (real_point3d*).
//   // blam-cc: ECX=frustum, EDX=point

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"

// Classifies a world-space point against the camera frustum's four side planes (not near/far),
// returning a 4-bit outcode: one bit per plane the point is in front of (outside).
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
uint8_t render_frustum_classify_point_side_planes(render_frustum *frustum, real_point3d *point) // blam-cc: ECX=frustum, EDX=point
{
    uint8_t flags = 0;

    if ((frustum->world_planes[0].normal.i * point->x + frustum->world_planes[0].normal.k * point->z +
         frustum->world_planes[0].normal.j * point->y) - frustum->world_planes[0].d > 0.0f) {
        flags |= _render_frustum_point_plane0_bit;
    }
    if ((frustum->world_planes[1].normal.i * point->x + frustum->world_planes[1].normal.k * point->z +
         frustum->world_planes[1].normal.j * point->y) - frustum->world_planes[1].d > 0.0f) {
        flags |= _render_frustum_point_plane1_bit;
    }
    if ((frustum->world_planes[2].normal.i * point->x + frustum->world_planes[2].normal.k * point->z +
         frustum->world_planes[2].normal.j * point->y) - frustum->world_planes[2].d > 0.0f) {
        flags |= _render_frustum_point_plane2_bit;
    }
    if ((frustum->world_planes[3].normal.i * point->x + frustum->world_planes[3].normal.k * point->z +
         frustum->world_planes[3].normal.j * point->y) - frustum->world_planes[3].d > 0.0f) {
        flags |= _render_frustum_point_plane3_bit;
    }

    return flags;
}

#if 0
Original Ghidra decompilation (0x50d4c0):

byte render_frustum_classify_point_side_planes(void)

{
  byte bVar1;
  int in_ECX;
  float *in_EDX;
  byte bVar2;

  if ((*(float *)(in_ECX + 0x88) * *in_EDX +
      *(float *)(in_ECX + 0x90) * in_EDX[2] + *(float *)(in_ECX + 0x8c) * in_EDX[1]) -
      *(float *)(in_ECX + 0x94) <= 0.0) {
    bVar1 = 0;
  }
  else {
    bVar1 = 2;
  }
  if ((*in_EDX * *(float *)(in_ECX + 0x98) +
      *(float *)(in_ECX + 0xa0) * in_EDX[2] + *(float *)(in_ECX + 0x9c) * in_EDX[1]) -
      *(float *)(in_ECX + 0xa4) <= 0.0) {
    bVar2 = 0;
  }
  else {
    bVar2 = 8;
  }
  bVar2 = 0.0 < (*(float *)(in_ECX + 0x78) * *in_EDX +
                *(float *)(in_ECX + 0x80) * in_EDX[2] + *(float *)(in_ECX + 0x7c) * in_EDX[1]) -
                *(float *)(in_ECX + 0x84) | bVar1 | bVar2;
  if (0.0 < (*(float *)(in_ECX + 0xa8) * *in_EDX +
            *(float *)(in_ECX + 0xb0) * in_EDX[2] + *(float *)(in_ECX + 0xac) * in_EDX[1]) -
            *(float *)(in_ECX + 0xb4)) {
    return bVar2 | 4;
  }
  return bVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
