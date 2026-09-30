// frustum_planes_classify_box  (Ghidra: FUN_00554260, already named)
// address 0x554260, size 434 bytes
// name confidence: 0.85 -- matches the classic Gribb/Hartmann 8-corner-vs-N-planes classifier.
// rewrite confidence: 0.85 -- the corner enumeration is unrolled by the original compiler into 8
//   explicit dot-product/compare blocks; that unrolling is preserved exactly (each of the 8 box
//   corners against the current plane), only folded into a loop over corners for readability
//   while keeping the identical arithmetic and the identical short-circuit "all planes trivially
//   reject" exit (bVar8 == 0xff -> return 0 immediately, matching structure_bsp_overlap_none).
// evidence: types/structures.h structure_bsp_overlap enum (0/1/2 = none/partial/contained, "2
//   means no clipping needed" and 0x00553c40 taking the min of this and aabb_overlap_classify);
//   caller 0x00553f10 (bsp3d_node_query_recursive, this batch) passes EAX = a 6-float box and
//   sets EBX/DI from its own stack arguments (verified by disassembly).
// register convention: in_EAX -> box (real_rectangle3d *), unaff_EBX -> planes
//   (real_plane3d *, stride 0x10), unaff_DI -> plane_count. No stack parameters.
//   // blam-cc: EAX -> box, EBX -> planes, DI -> plane_count
// The per-plane test evaluates plane.normal . corner - plane.d for all 8 corners of the box and
// sets one of 8 bits when a corner is on the negative side of that plane; if every corner of a
// box is on the negative side of ANY plane (bVar8 == 0xff) the box is fully outside and the
// function returns immediately without checking the rest of the planes. If any corner of any
// plane came out negative but no plane rejected the whole box, the box is partially clipped.
// Otherwise (every corner of every plane is non-negative) the box is fully inside.

#include "tags.h"
#include "math.h"
#include "memory.h"
#include "structures.h"
#include "fn_structures.h"

// blam-cc: EAX -> box, EBX -> planes, DI -> plane_count
structure_bsp_overlap frustum_planes_classify_box(real_rectangle3d *box, real_plane3d *planes,
                                                   int16_t plane_count)
{
    uint8_t any_corner_outside = 0;

    for (int16_t p = 0; p < plane_count; p++) {
        real_plane3d *plane = (real_plane3d *)((uint8_t *)planes + p * 0x10);
        float nx = plane->normal.i, ny = plane->normal.j, nz = plane->normal.k, d = plane->d;
        // 8 corners: bit 0 = (xmin,ymin,zmin) ... matching the compiler's unrolled order exactly
        // (x low/high toggles fastest, then y, then z), so the bit layout matches the original.
        float xs[2] = { box->x.lower, box->x.upper };
        float ys[2] = { box->y.lower, box->y.upper };
        float zs[2] = { box->z.lower, box->z.upper };
        uint8_t outside_mask = 0;
        for (int corner = 0; corner < 8; corner++) {
            float cx = xs[corner & 1];
            float cy = ys[(corner >> 1) & 1];
            float cz = zs[(corner >> 2) & 1];
            if ((nx * cx + ny * cy + nz * cz) - d < 0.0f) {
                outside_mask |= (uint8_t)(1 << corner);
            }
        }
        if (outside_mask == 0xff) {
            return _structure_bsp_overlap_none;
        }
        any_corner_outside |= outside_mask;
    }
    if (any_corner_outside != 0) {
        return _structure_bsp_overlap_partial;
    }
    return _structure_bsp_overlap_contained;
}

#if 0
Original Ghidra decompilation (0x554260):

undefined4 frustum_planes_classify_box(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float *in_EAX;
  float *pfVar7;
  byte bVar8;
  short sVar9;
  int unaff_EBX;
  byte bVar10;
  short unaff_DI;

  bVar10 = 0;
  sVar9 = 0;
  if (0 < unaff_DI) {
    do {
      pfVar7 = (float *)(sVar9 * 0x10 + unaff_EBX);
      fVar1 = pfVar7[3];
      fVar2 = in_EAX[4] * pfVar7[2];
      fVar3 = in_EAX[2] * pfVar7[1];
      fVar4 = *in_EAX * *pfVar7;
      bVar8 = (fVar4 + fVar3 + fVar2) - fVar1 < 0.0;
      fVar5 = in_EAX[1] * *pfVar7;
      if ((fVar3 + fVar5 + fVar2) - fVar1 < 0.0) {
        bVar8 = bVar8 | 2;
      }
      fVar6 = in_EAX[3] * pfVar7[1];
      fVar2 = fVar2 + fVar6;
      if ((fVar4 + fVar2) - fVar1 < 0.0) {
        bVar8 = bVar8 | 4;
      }
      if ((fVar2 + fVar5) - fVar1 < 0.0) {
        bVar8 = bVar8 | 8;
      }
      fVar2 = in_EAX[5] * pfVar7[2];
      if ((fVar4 + fVar2 + fVar3) - fVar1 < 0.0) {
        bVar8 = bVar8 | 0x10;
      }
      if ((fVar2 + fVar5 + fVar3) - fVar1 < 0.0) {
        bVar8 = bVar8 | 0x20;
      }
      fVar2 = fVar2 + fVar6;
      if ((fVar4 + fVar2) - fVar1 < 0.0) {
        bVar8 = bVar8 | 0x40;
      }
      if ((fVar2 + fVar5) - fVar1 < 0.0) {
        bVar8 = bVar8 | 0x80;
      }
      if (bVar8 == 0xff) {
        return 0;
      }
      bVar10 = bVar10 | bVar8;
      sVar9 = sVar9 + 1;
    } while (sVar9 < unaff_DI);
    if (bVar10 != 0) {
      return 1;
    }
  }
  return 2;
}
#endif
