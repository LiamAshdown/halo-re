// polygon2d_bounds_expand  (Ghidra: FUN_00554a90, already named)
// address 0x554a90, size 94 bytes
// name confidence: 0.85 -- already named; body is an unambiguous "grow this 2D bounding box to
//   cover every point of a polygon" routine.
// rewrite confidence: 0.8 -- small and register-only, but every register role is confirmed by
//   its single caller (camera_cluster_portal_flood_recursive, this batch): EDX -> the
//   {min x, max x, min y, max y} bounds being grown (a structure_bsp_visible_cluster's
//   screen_bounds_x/y, back to back), EDI -> a polygon2d (count then real_point2d[] points).
// evidence: types/structures.h structure_bsp_visible_cluster.screen_bounds_x/y and polygon2d.
// register convention: unaff_EDX -> bounds (real_bounds[2]: x then y), unaff_EDI -> polygon.
//   // blam-cc: EDX -> bounds, EDI -> polygon
// UNSURE: none.

#include "tags.h"
#include "math.h"
#include "memory.h"
#include "structures.h"

// blam-cc: EDX -> bounds, EDI -> polygon
void polygon2d_bounds_expand(real_bounds *bounds_xy, polygon2d *polygon)
{
    for (int16_t i = 0; i < polygon->point_count; i++) {
        real_point2d *p = &polygon->points[i];
        if (p->x < bounds_xy[0].lower) { bounds_xy[0].lower = p->x; }
        if (bounds_xy[0].upper < p->x) { bounds_xy[0].upper = p->x; }
        if (p->y < bounds_xy[1].lower) { bounds_xy[1].lower = p->y; }
        if (bounds_xy[1].upper < p->y) { bounds_xy[1].upper = p->y; }
    }
}

#if 0
Original Ghidra decompilation (0x554a90):

void polygon2d_bounds_expand(void)

{
  float *pfVar1;
  float *in_EDX;
  short sVar2;
  short *unaff_EDI;

  sVar2 = 0;
  pfVar1 = (float *)(unaff_EDI + 2);
  if (0 < *unaff_EDI) {
    do {
      if (*pfVar1 < *in_EDX) {
        *in_EDX = *pfVar1;
      }
      if (in_EDX[1] < *pfVar1) {
        in_EDX[1] = *pfVar1;
      }
      if (pfVar1[1] < in_EDX[2]) {
        in_EDX[2] = pfVar1[1];
      }
      if (in_EDX[3] < pfVar1[1]) {
        in_EDX[3] = pfVar1[1];
      }
      pfVar1 = pfVar1 + 2;
      sVar2 = sVar2 + 1;
    } while (sVar2 < *unaff_EDI);
  }
  return;
}
#endif
