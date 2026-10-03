// physics_shape_pill_test_ray  (Ghidra: FUN_005045c0, still unnamed there; name from
// out/phase2/results/physics_00.json: "Solves the cylinder/capsule-vs-ray quadratic against the
// pill axis ... clamping the along-axis parameter, then finishes with point3d_add_scaled/
// vector3d_normalize_with_length -- the ray counterpart of FUN_00503f90.")
// address 0x5045c0, size 784 bytes
// name confidence: 0.5   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x5045c0..0x5048d0 (quadratic, cap clamps both signs, hit plane via two point3d_add_scaled calls).)
// evidence: physics_model_pill.origin_x/y/z (0x0c/0x10/0x14), .extent_i/j/k (0x18/0x1c/0x20) and
//   .radius (0x24) match unaff_EDI+0xc.. exactly, the same fields physics_shape_pill_test_point
//   (0x503f90, this module) reads off unaff_ESI; the final normalize+d formula is identical to
//   that function's, substituting the ray hit point for the query point; the "a<b != (a==b)"
//   idiom resolves to plain <= (established by physics_shape_pill_test_point's own header note).
// register convention: in_ECX -> delta, in_EDX -> origin, unaff_EBX -> out_plane
//   (real_plane3d *), unaff_EDI -> pill (physics_model_pill *). param_1 is the
//   Ghidra-recognized stack parameter (out_t).
//   // blam-cc: ECX -> delta, EDX -> origin, EBX -> out_plane, EDI -> pill, stack -> out_t
// UNSURE: after the along-axis parameter t0 is found, Ghidra shows a second
// point3d_add_scaled call whose visible "base" argument (&local_c) is never written anywhere
// beforehand in this function's own decompile -- exactly the same "local_c reads as if already
// initialized" pattern physics_shape_pill_test_point.c already flagged for its own (simpler,
// non-ray) normal computation. This rewrite applies that same established formula
// (perpendicular = hit_point_relative - (hit_point_relative . extent / extent_len_sq) * extent)
// to the ray's hit point at t0 rather than to the original query point, which is the only
// reading consistent with both functions ending in an identical normalize + origin.normal + d
// finish.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "physics.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void point3d_add_scaled(real_point3d *out, real_vector3d *direction, real_point3d *base,
                                float scale); // 0x401930
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern double sqrt(double x); // SQRT is a single x87 FSQRT instruction

// Tests a world-space ray (origin, delta) against pill, the ray counterpart of
// physics_shape_pill_test_point. Solves the swept-cylinder quadratic for the along-axis
// parameter range, clips it against the pill's flat end caps, and reports the near root when
// the clipped range is non-empty. On a hit, out_plane is the outward-facing separating plane at
// the hit point (the component of the hit point perpendicular to the pill's axis).
// blam-cc: ECX -> delta, EDX -> origin, EBX -> out_plane, EDI -> pill, stack -> out_t
// FIXED (objdump): every ret sets only AL; the upper bits of EAX are left as they were
uint8_t physics_shape_pill_test_ray(real_vector3d *delta, real_point3d *origin,
                                      real_plane3d *out_plane, physics_model_pill *pill,
                                      float *out_t)
{
    float extent_len_sq = pill->extent_k * pill->extent_k + pill->extent_j * pill->extent_j +
                           pill->extent_i * pill->extent_i;
    float proj_delta_extent =
        pill->extent_i * delta->i + pill->extent_j * delta->j + pill->extent_k * delta->k;
    float denom = (delta->k * delta->k + delta->j * delta->j + delta->i * delta->i) * extent_len_sq -
                  proj_delta_extent * proj_delta_extent;

    if (denom != 0.0f) {
        real_point3d rel;
        float proj_rel_extent;
        float b, disc;

        rel.x = origin->x - pill->origin_x;
        rel.y = origin->y - pill->origin_y;
        rel.z = origin->z - pill->origin_z;
        proj_rel_extent = rel.x * pill->extent_i + rel.y * pill->extent_j + rel.z * pill->extent_k;
        b = proj_rel_extent * proj_delta_extent -
            (rel.z * delta->k + rel.x * delta->i + rel.y * delta->j) * extent_len_sq;
        disc = b * b - (((rel.x * rel.x + rel.y * rel.y + rel.z * rel.z) -
                          pill->radius * pill->radius) * extent_len_sq -
                         proj_rel_extent * proj_rel_extent) * denom;

        if (disc >= 0.0f) {
            float t0 = (b - (float)sqrt((double)disc)) * (1.0f / denom);
            float t1 = ((float)sqrt((double)disc) + b) * (1.0f / denom);

            if (t0 <= 1.0f && t1 >= 0.0f) {
                if (t0 < 0.0f) {
                    t0 = 0.0f;
                }
                if (t1 > 1.0f) {
                    t1 = 1.0f;
                }
                if (proj_delta_extent == 0.0f) {
                    if (proj_rel_extent < 0.0f) {
                        return 0;
                    }
                    if (proj_rel_extent > extent_len_sq) {
                        return 0;
                    }
                } else {
                    float cap_a = -(proj_rel_extent * (1.0f / proj_delta_extent));
                    float cap_b = (extent_len_sq - proj_rel_extent) * (1.0f / proj_delta_extent);

                    if (proj_delta_extent <= 0.0f) {
                        if (t0 < cap_b) {
                            t0 = cap_b;
                        }
                        if (cap_a < t1) {
                            t1 = cap_a;
                        }
                    } else {
                        if (t0 < cap_a) {
                            t0 = cap_a;
                        }
                        if (cap_b < t1) {
                            t1 = cap_b;
                        }
                    }
                    if (t0 > t1) {
                        return 0;
                    }
                }

                {
                    real_point3d hit_relative;
                    float proj_hit_extent;
                    real length;

                    *out_t = t0;
                    point3d_add_scaled(&hit_relative, delta, &rel, t0);

                    proj_hit_extent = hit_relative.x * pill->extent_i +
                                      hit_relative.y * pill->extent_j +
                                      hit_relative.z * pill->extent_k;
                    out_plane->normal.i = hit_relative.x - (proj_hit_extent / extent_len_sq) * pill->extent_i;
                    out_plane->normal.j = hit_relative.y - (proj_hit_extent / extent_len_sq) * pill->extent_j;
                    out_plane->normal.k = hit_relative.z - (proj_hit_extent / extent_len_sq) * pill->extent_k;

                    length = vector3d_normalize_with_length(&out_plane->normal);
                    if (length == 0.0f) {
                        out_plane->normal.i = 1.0f;
                        out_plane->normal.j = 0.0f;
                        out_plane->normal.k = 0.0f;
                    }
                    out_plane->d = pill->origin_x * out_plane->normal.i +
                                    pill->origin_y * out_plane->normal.j +
                                    pill->origin_z * out_plane->normal.k + pill->radius;
                    return 1;
                }
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x5045c0):

uint FUN_005045c0(float *param_1)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  uint uVar8;
  float *in_ECX;
  float *in_EDX;
  float *unaff_EBX;
  int unaff_EDI;
  float10 fVar9;
  float10 fVar10;
  float local_2c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  pfVar1 = (float *)(unaff_EDI + 0x18);
  fVar2 = *(float *)(unaff_EDI + 0x20) * *(float *)(unaff_EDI + 0x20) +
          *(float *)(unaff_EDI + 0x1c) * *(float *)(unaff_EDI + 0x1c) + *pfVar1 * *pfVar1;
  fVar3 = *pfVar1 * *in_ECX +
          *(float *)(unaff_EDI + 0x1c) * in_ECX[1] + *(float *)(unaff_EDI + 0x20) * in_ECX[2];
  fVar4 = (in_ECX[2] * in_ECX[2] + in_ECX[1] * in_ECX[1] + *in_ECX * *in_ECX) * fVar2 -
          fVar3 * fVar3;
  uVar8 = (uint)(ushort)((ushort)(fVar4 < 0.0) << 8 | (ushort)NAN(fVar4) << 10 |
                        (ushort)(fVar4 == 0.0) << 0xe);
  if (fVar4 != 0.0) {
    local_18 = *in_EDX - *(float *)(unaff_EDI + 0xc);
    local_14 = in_EDX[1] - *(float *)(unaff_EDI + 0x10);
    local_10 = in_EDX[2] - *(float *)(unaff_EDI + 0x14);
    fVar5 = local_18 * *pfVar1 +
            local_14 * *(float *)(unaff_EDI + 0x1c) + local_10 * *(float *)(unaff_EDI + 0x20);
    fVar6 = fVar5 * fVar3 -
            (local_10 * in_ECX[2] + local_18 * *in_ECX + local_14 * in_ECX[1]) * fVar2;
    fVar7 = fVar6 * fVar6 -
            (((local_18 * local_18 + local_14 * local_14 + local_10 * local_10) -
             *(float *)(unaff_EDI + 0x24) * *(float *)(unaff_EDI + 0x24)) * fVar2 - fVar5 * fVar5) *
            fVar4;
    uVar8 = (uint)(ushort)((ushort)(fVar7 < 0.0) << 8 | (ushort)NAN(fVar7) << 10 |
                          (ushort)(fVar7 == 0.0) << 0xe);
    if (fVar7 >= 0.0) {
      local_2c = (fVar6 - SQRT(fVar7)) * (1.0 / fVar4);
      fVar4 = (SQRT(fVar7) + fVar6) * (1.0 / fVar4);
      uVar8 = (uint)(ushort)((ushort)(local_2c < 1.0) << 8 | (ushort)NAN(local_2c) << 10 |
                            (ushort)(local_2c == 1.0) << 0xe);
      if ((local_2c < 1.0 || (local_2c == 1.0) != 0) &&
         (uVar8 = (uint)(ushort)((ushort)(fVar4 < 0.0) << 8 | (ushort)NAN(fVar4) << 10 |
                                (ushort)(fVar4 == 0.0) << 0xe), fVar4 >= 0.0)) {
        if (local_2c < 0.0) {
          local_2c = 0.0;
        }
        if (1.0 < fVar4) {
          fVar4 = 1.0;
        }
        if (fVar3 == 0.0) {
          if (fVar5 < 0.0) {
            return (uint)(ushort)((ushort)(fVar5 < 0.0) << 8 | (ushort)NAN(fVar5) << 10 |
                                 (ushort)(fVar5 == 0.0) << 0xe);
          }
          if (fVar5 >= fVar2 && (fVar5 == fVar2) == 0) {
            return (uint)(ushort)((ushort)(fVar5 < fVar2) << 8 |
                                  (ushort)(NAN(fVar5) || NAN(fVar2)) << 10 |
                                 (ushort)(fVar5 == fVar2) << 0xe);
          }
        }
        else {
          fVar6 = -(fVar5 * (1.0 / fVar3));
          fVar5 = (fVar2 - fVar5) * (1.0 / fVar3);
          if (fVar3 <= 0.0) {
            if (local_2c < fVar5) {
              local_2c = fVar5;
            }
            if (fVar6 < fVar4) {
              fVar4 = fVar6;
            }
          }
          else {
            if (local_2c < fVar6) {
              local_2c = fVar6;
            }
            if (fVar5 < fVar4) {
              fVar4 = fVar5;
            }
          }
          if (local_2c >= fVar4 && (local_2c == fVar4) == 0) {
            return (uint)(ushort)((ushort)(local_2c < fVar4) << 8 |
                                  (ushort)(NAN(local_2c) || NAN(fVar4)) << 10 |
                                 (ushort)(local_2c == fVar4) << 0xe);
          }
        }
        *param_1 = local_2c;
        point3d_add_scaled(&local_18,local_2c);
        point3d_add_scaled(&local_c,-((local_c * *pfVar1 +
                                      local_4 * *(float *)(unaff_EDI + 0x20) +
                                      local_8 * *(float *)(unaff_EDI + 0x1c)) / fVar2));
        fVar9 = (float10)vector3d_normalize_with_length();
        fVar10 = (float10)0.0;
        uVar8 = (uint)(ushort)((ushort)(fVar10 < fVar9) << 8 |
                               (ushort)(NAN(fVar10) || NAN(fVar9)) << 10 |
                              (ushort)(fVar10 == fVar9) << 0xe);
        if (fVar10 == fVar9) {
          uVar8 = 0;
          *unaff_EBX = 1.0;
          unaff_EBX[1] = 0.0;
          unaff_EBX[2] = 0.0;
        }
        unaff_EBX[3] = *(float *)(unaff_EDI + 0xc) * *unaff_EBX +
                       *(float *)(unaff_EDI + 0x10) * unaff_EBX[1] +
                       *(float *)(unaff_EDI + 0x14) * unaff_EBX[2] + *(float *)(unaff_EDI + 0x24);
        return CONCAT31((int3)(uVar8 >> 8),1);
      }
    }
  }
  return uVar8;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
