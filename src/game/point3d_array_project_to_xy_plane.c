// point3d_array_project_to_xy_plane  (Ghidra: point3d_array_extract_xz_pairs; CORRECTED here)
// address 0x46a130, size 125 bytes
// name confidence: 0.5 -> corrected   rewrite confidence: 0.55
// evidence: the naming pass's own name ("extract_xz_pairs") does not match the arithmetic: with
//   `real_point3d` a tight 3-float (x,y,z) record, the 4-wide unrolled loop reads
//   element[i].x/.y (offsets +0x0/+0x4) into the destination pair and skips element[i].z
//   (+0x8) entirely -- every stride confirms it (main loop: puVar1[-3]/[-2] = element[0].x/.y,
//   puVar1[0]/[1] = element[1].x/.y, puVar1[3]/[4] = element[2].x/.y, puVar1[6]/[7] =
//   element[3].x/.y; scalar tail: puVar2[0]/[1] = element[i].x/.y). This is a straight
//   ground-plane (X,Y) projection dropping the vertical Z component -- consistent with its only
//   caller, FUN_0046a240 (this batch), building a 2D convex hull of starting-location points
//   where the OTHER two fields it separately tracks as "vertical extent" are the source points'
//   Z values. Renamed accordingly; corrected in this rewrite rather than left under the old name.
// register convention: source real_point3d array in the stack parameter (Ghidra's own
//   `param_1`); destination Point2D array in unaff_EDI; element count in unaff_EBX.
//   // blam-cc: stack -> source, EDI -> destination, EBX -> count

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: stack -> source, EDI -> destination, EBX -> count
// Projects `count` real_point3d entries from `source` into `destination` as (x, y) pairs,
// dropping z. Functionally a simple per-element copy; the original is manually unrolled 4-wide.
void point3d_array_project_to_xy_plane(real_point3d *source, Point2D *destination, int32_t count)
{
    int32_t i;
    for (i = 0; i < count; i++) {
        destination[i].x = source[i].x;
        destination[i].y = source[i].y;
    }
}

#if 0
Original Ghidra decompilation (0x46a130), from tools/pack.py 0x46a130:

void point3d_array_extract_xz_pairs(int param_1)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  int iVar3;
  int unaff_EBX;
  int iVar4;
  int unaff_EDI;

  iVar3 = 0;
  if (3 < unaff_EBX) {
    puVar1 = (undefined4 *)(param_1 + 0xc);
    iVar4 = (unaff_EBX - 4U >> 2) + 1;
    puVar2 = (undefined4 *)(unaff_EDI + 8);
    iVar3 = iVar4 * 4;
    do {
      puVar2[-2] = puVar1[-3];
      puVar2[-1] = puVar1[-2];
      *puVar2 = *puVar1;
      puVar2[1] = puVar1[1];
      puVar2[2] = puVar1[3];
      puVar2[3] = puVar1[4];
      puVar2[4] = puVar1[6];
      puVar2[5] = puVar1[7];
      puVar1 = puVar1 + 0xc;
      puVar2 = puVar2 + 8;
      iVar4 = iVar4 + -1;
    } while (iVar4 != 0);
  }
  if (iVar3 < unaff_EBX) {
    puVar2 = (undefined4 *)(param_1 + iVar3 * 0xc);
    do {
      *(undefined4 *)(unaff_EDI + iVar3 * 8) = *puVar2;
      *(undefined4 *)(unaff_EDI + 4 + iVar3 * 8) = puVar2[1];
      iVar3 = iVar3 + 1;
      puVar2 = puVar2 + 3;
    } while (iVar3 < unaff_EBX);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
