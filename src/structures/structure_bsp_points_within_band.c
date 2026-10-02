// structure_bsp_points_within_band  (Ghidra: FUN_00554a20, still unnamed there)
// address 0x554a20, size 99 bytes
// name confidence: 0.5 -- phase4 summary: "Checks whether a set of points all lie behind a given
//   camera-relative plane within a tolerance, used as a portal-visibility rejection test." The
//   verified return polarity (see below) is the opposite of that phrasing, so the name here
//   describes the literal test rather than the gloss.
// rewrite confidence: 0.5 -- fully register-passed, no formal parameters at all in Ghidra's
//   decompile; every register is confirmed by its single caller
//   (camera_cluster_portal_flood_recursive.c, this batch) and by the .rdata globals it reads.
// evidence: types/structures.h's render camera block notes (+0x14 camera position, +0x20 forward
//   vector); this function's own body reads exactly those two triples from 0x007c3114/0x7c3120.
//   Return polarity: `(dist < tol) != (dist == tol)` is true exactly when `dist <= tol`, so the
//   function returns true as soon as it finds ONE point at-or-behind the plane, and false only
//   when EVERY point is strictly in front of it -- the inverse of "all points lie behind".
//   camera_cluster_portal_flood_recursive only calls it when no sky is available, treating a
//   false return as "this portal's geometry is entirely past the tolerance plane; still reject".
// register convention: in_EAX -> points (real_point3d *), in_EDX -> point_count, stack -> tolerance.
//   // blam-cc: EDX -> points, SI -> point_count, stack -> tolerance
// UNSURE: none left in this function's own body.

#include "tags.h"
#include "math.h"
#include "memory.h"
#include "structures.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern real_point3d render_camera_global; // 0x007c3114 (render camera block +0x14)
extern real_vector3d camera_forward_x; // 0x007c3120 (render camera block +0x20)

// blam-cc: EDX -> points, SI -> point_count, stack -> tolerance
uint8_t structure_bsp_points_within_band(real_point3d *points, int16_t point_count,
                                          float tolerance)
{
    for (int16_t i = 0; i < point_count; i++) {
        real_point3d *p = &points[i];
        float distance = camera_forward_x.i * (p->x - render_camera_global.x) +
                          camera_forward_x.j * (p->y - render_camera_global.y) +
                          camera_forward_x.k * (p->z - render_camera_global.z);
        if (distance <= tolerance) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x554a20):

uint FUN_00554a20(float param_1)

{
  float *pfVar1;
  float fVar2;
  uint in_EAX;
  short sVar3;
  int in_EDX;
  short unaff_SI;

  sVar3 = 0;
  if (0 < unaff_SI) {
    do {
      pfVar1 = (float *)(in_EDX + sVar3 * 0xc);
      fVar2 = DAT_007c3120 * (*pfVar1 - DAT_007c3114) +
              DAT_007c3124 * (pfVar1[1] - DAT_007c3118) + DAT_007c3128 * (pfVar1[2] - DAT_007c311c);
      in_EAX = CONCAT22((short)((uint)pfVar1 >> 0x10),
                        (ushort)(fVar2 < param_1) << 8 | (ushort)(NAN(fVar2) || NAN(param_1)) << 10
                        | (ushort)(fVar2 == param_1) << 0xe);
      if (fVar2 < param_1 != (fVar2 == param_1)) {
        return CONCAT31((int3)(in_EAX >> 8),1);
      }
      sVar3 = sVar3 + 1;
    } while (sVar3 < unaff_SI);
  }
  return in_EAX & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
