// physics_shape_pill_test_point  (Ghidra: FUN_00503f90, still unnamed there; name from
// out/phase2/results/physics_00.json)
// address 0x503f90, size 391 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: physics_model_pill.origin_x/y/z (0x0c/0x10/0x14), .extent_i/j/k (0x18/0x1c/0x20)
//   and .radius (0x24) match unaff_ESI+0xc.. exactly (out/phase4/physics_types_notes.md).
// register convention: in_EAX -> point, unaff_ESI -> pill (physics_model_pill *), unaff_EDI ->
//   out_normal (real_plane3d *). param_1 is the Ghidra-recognized stack parameter (out_depth).
//   // blam-cc: EAX -> point, ESI -> pill, EDI -> out_normal, stack -> out_depth
// UNSURE: vector3d_normalize_with_length's call site shows no argument; reconstructed as
// out_normal itself (cast to real_vector3d *), matching that this function writes the
// unnormalized separating vector directly into *out_normal just before the call, and that
// function's own established EAX -> vector, in-place-normalize convention.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990

// blam-cc: EAX -> point, ESI -> pill, EDI -> out_normal, stack -> out_depth
// FIXED (objdump): every ret sets only AL; the upper bits of EAX are left as they were
uint8_t physics_shape_pill_test_point(real_point3d *point, physics_model_pill *pill,
                                        real_plane3d *out_normal, float *out_depth)
{
    float rel_x = point->x - pill->origin_x;
    float rel_y = point->y - pill->origin_y;
    float rel_z = point->z - pill->origin_z;
    float proj = rel_x * pill->extent_i + rel_y * pill->extent_j + rel_z * pill->extent_k;

    if (proj >= 0.0f) {
        float extent_len_sq =
            pill->extent_k * pill->extent_k + pill->extent_j * pill->extent_j +
            pill->extent_i * pill->extent_i;
        // equivalent to (proj <= extent_len_sq)
        if (proj < extent_len_sq || proj == extent_len_sq) {
            float perp_dist_sq =
                (rel_z * rel_z + rel_y * rel_y + rel_x * rel_x) * extent_len_sq - proj * proj;
            float radius_sq_scaled = pill->radius * pill->radius * extent_len_sq;

            if (radius_sq_scaled >= perp_dist_sq && radius_sq_scaled != perp_dist_sq) {
                real vector_len;

                if (extent_len_sq <= 0.0f) {
                    out_normal->normal.i = rel_x;
                    out_normal->normal.j = rel_y;
                    out_normal->normal.k = rel_z;
                } else {
                    float t = proj / extent_len_sq;
                    out_normal->normal.i = rel_x - t * pill->extent_i;
                    out_normal->normal.j = rel_y - t * pill->extent_j;
                    out_normal->normal.k = rel_z - t * pill->extent_k;
                }
                vector_len = vector3d_normalize_with_length((real_vector3d *)&out_normal->normal);
                if (vector_len == 0.0f) {
                    out_normal->normal.i = 0.0f;
                    out_normal->normal.j = 0.0f;
                    out_normal->normal.k = 1.0f;
                }
                out_normal->d = pill->origin_x * out_normal->normal.i +
                                 pill->origin_y * out_normal->normal.j +
                                 pill->origin_z * out_normal->normal.k + pill->radius;
                *out_depth = pill->radius - vector_len;
                return 1;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x503f90):

uint FUN_00503f90(float *param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float *in_EAX;
  uint uVar8;
  int unaff_ESI;
  float *unaff_EDI;
  float10 fVar9;
  float10 fVar10;

  fVar3 = *in_EAX - *(float *)(unaff_ESI + 0xc);
  fVar4 = in_EAX[1] - *(float *)(unaff_ESI + 0x10);
  fVar5 = in_EAX[2] - *(float *)(unaff_ESI + 0x14);
  fVar6 = fVar3 * *(float *)(unaff_ESI + 0x18) +
          fVar4 * *(float *)(unaff_ESI + 0x1c) + fVar5 * *(float *)(unaff_ESI + 0x20);
  uVar8 = (uint)(ushort)((ushort)(fVar6 < 0.0) << 8 | (ushort)NAN(fVar6) << 10 |
                        (ushort)(fVar6 == 0.0) << 0xe);
  if (fVar6 >= 0.0) {
    fVar1 = *(float *)(unaff_ESI + 0x20) * *(float *)(unaff_ESI + 0x20) +
            *(float *)(unaff_ESI + 0x1c) * *(float *)(unaff_ESI + 0x1c) +
            *(float *)(unaff_ESI + 0x18) * *(float *)(unaff_ESI + 0x18);
    uVar8 = (uint)(ushort)((ushort)(fVar6 < fVar1) << 8 | (ushort)(NAN(fVar6) || NAN(fVar1)) << 10 |
                          (ushort)(fVar6 == fVar1) << 0xe);
    if (fVar6 < fVar1 != (fVar6 == fVar1)) {
      fVar7 = (fVar3 * fVar3 + fVar4 * fVar4 + fVar5 * fVar5) * fVar1 - fVar6 * fVar6;
      fVar2 = *(float *)(unaff_ESI + 0x24) * *(float *)(unaff_ESI + 0x24) * fVar1;
      uVar8 = (uint)(ushort)((ushort)(fVar2 < fVar7) << 8 | (ushort)(NAN(fVar2) || NAN(fVar7)) << 10
                            | (ushort)(fVar2 == fVar7) << 0xe);
      if (fVar2 >= fVar7 && (fVar2 == fVar7) == 0) {
        if (fVar1 <= 0.0) {
          *unaff_EDI = fVar3;
          unaff_EDI[1] = fVar4;
          unaff_EDI[2] = fVar5;
        }
        else {
          fVar6 = fVar6 / fVar1;
          fVar1 = *(float *)(unaff_ESI + 0x1c);
          fVar2 = *(float *)(unaff_ESI + 0x20);
          *unaff_EDI = fVar3 - fVar6 * *(float *)(unaff_ESI + 0x18);
          unaff_EDI[1] = fVar4 - fVar6 * fVar1;
          unaff_EDI[2] = fVar5 - fVar6 * fVar2;
        }
        fVar9 = (float10)vector3d_normalize_with_length();
        fVar10 = (float10)0.0;
        if (fVar9 == fVar10) {
          *unaff_EDI = 0.0;
          unaff_EDI[1] = 0.0;
          unaff_EDI[2] = 1.0;
        }
        unaff_EDI[3] = *(float *)(unaff_ESI + 0xc) * *unaff_EDI +
                       *(float *)(unaff_ESI + 0x10) * unaff_EDI[1] +
                       *(float *)(unaff_ESI + 0x14) * unaff_EDI[2] + *(float *)(unaff_ESI + 0x24);
        *param_1 = (float)((float10)*(float *)(unaff_ESI + 0x24) - fVar9);
        return CONCAT31((uint3)(byte)(fVar9 < fVar10 |
                                      (byte)((ushort)((ushort)(NAN(fVar9) || NAN(fVar10)) << 10) >>
                                            8) |
                                     (byte)((ushort)((ushort)(fVar9 == fVar10) << 0xe) >> 8)),1);
      }
    }
  }
  return uVar8;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
