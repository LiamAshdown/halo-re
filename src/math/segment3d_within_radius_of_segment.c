// segment3d_within_radius_of_segment  (Ghidra: segment3d_within_radius_of_segment, already named)
// address 0x4ceae0, size 1198 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: math_functions.md: "Tests whether two 3D segments come within radius param_2 of
//   each other, i.e. a segment-vs-thick-segment (capsule-like) proximity test." Shares almost
//   all of its structure -- including the exact parallel-segment s/t derivation -- with
//   segment3d_distance_squared_to_segment @0x4cdef0, but the non-parallel out-of-range fallback
//   tests a sphere-vs-ray via ray_intersects_sphere_test @0x4ce6c0 instead of computing a point-
//   to-segment distance.
// register convention: a_start pointer as the recognized parameter (param_1 -> EAX per the
//   project convention), b_start pointer in EBX (unaff_EBX), a_direction pointer in ESI
//   (unaff_ESI), b_direction pointer in EDI (unaff_EDI); radius as the recognized stack
//   parameter (param_2).
//   // blam-cc: EBX -> b_start, ESI -> a_direction, EDI -> b_direction, stack -> (a_start, radius)
//   CORRECTED 2026-09-28: 0x4ceae6 reads a_start from the first stack slot (mov ebp,[esp+0x34] after sub 0x2c /
//   push ebp); callers happen to also hold it in EAX (0x42b216).
//
// UNSURE, significantly: as in segment3d_distance_squared_to_segment, the raw s/t computation
// via vector3d_scalar_triple_product is a mathematically-motivated reconstruction, not a
// literal transliteration (see that file's header note). Additionally, the original's
// out-of-range fallback only ever explicitly computes a snapped "point on A" (when s is out of
// range) before calling ray_intersects_sphere_test @0x4ce6c0 -- and it calls that function
// *twice* with, as far as Ghidra could tell, unchanged arguments, via a bVar5 = bVar6
// short-circuit trick. No "point on B" is ever visibly computed for the t-out-of-range case.
// Rewritten here as the symmetric, semantically sensible version (mirroring
// segment3d_distance_squared_to_segment's two-sided fallback: snap+test A when s is out of
// range, snap+test B when t is out of range) rather than reproducing the apparent redundant
// double call, since the latter cannot be expressed without inventing an unverifiable register
// mapping.

#include "tags.h"
#include "math.h"

extern double fabs(double x); // ABS is a single x87 FABS instruction
extern real vector3d_scalar_triple_product(const real_vector3d *a, const real_vector3d *b, const real_vector3d *c); // 0x44d8e0, outside this module
extern uint8_t ray_intersects_sphere_test(real_point3d *center, real_point3d *origin, real_vector3d *direction, real radius); // 0x4ce6c0

int segment3d_within_radius_of_segment(real_point3d *a_start, real_point3d *b_start, real_vector3d *a_direction, real_vector3d *b_direction, real radius)
{
    real_vector3d w0; // b_start - a_start
    real_vector3d cross_ab;
    real s;
    real t;
    real dx, dy, dz;

    w0.i = b_start->x - a_start->x;
    w0.j = b_start->y - a_start->y;
    w0.k = b_start->z - a_start->z;

    cross_ab.i = a_direction->j * b_direction->k - b_direction->j * a_direction->k;
    cross_ab.j = a_direction->k * b_direction->i - a_direction->i * b_direction->k;
    cross_ab.k = b_direction->j * a_direction->i - a_direction->j * b_direction->i;

    if (fabs((double)(cross_ab.i * cross_ab.i + cross_ab.j * cross_ab.j + cross_ab.k * cross_ab.k)) < 0.0001) {
        // (near-)parallel: identical derivation to segment3d_distance_squared_to_segment's
        // parallel branch.
        real dot_dirs = a_direction->i * b_direction->i + a_direction->j * b_direction->j + a_direction->k * b_direction->k;
        real len_a2 = a_direction->i * a_direction->i + a_direction->j * a_direction->j + a_direction->k * a_direction->k;
        real len_b2 = b_direction->i * b_direction->i + b_direction->j * b_direction->j + b_direction->k * b_direction->k;

        if (len_a2 <= 0.0001f) {
            s = 0.0f;
        } else {
            real inv_a2 = 1.0f / len_a2;
            real s0 = (w0.i * a_direction->i + w0.k * a_direction->k + w0.j * a_direction->j) * inv_a2;
            real s1 = inv_a2 * dot_dirs + s0;
            real s0_clamped = s0;
            if (s0_clamped < 0.0f) { s0_clamped = 0.0f; } else if (1.0f < s0_clamped) { s0_clamped = 1.0f; }
            if (0.0f <= s1 && s1 <= 1.0f) { s = (s1 + s0_clamped) * 0.5f; }
            else if (1.0f < s1) { s = (1.0f + s0_clamped) * 0.5f; }
            else { s = (0.0f + s0_clamped) * 0.5f; }
        }

        if (len_b2 <= 0.0001f) {
            t = 0.0f;
        } else {
            real inv_b2 = 1.0f / len_b2;
            real t0 = -((w0.k * b_direction->k + w0.i * b_direction->i + w0.j * b_direction->j) * inv_b2);
            real t1 = inv_b2 * dot_dirs + t0;
            real t0_clamped = t0;
            if (t0_clamped < 0.0f) { t0_clamped = 0.0f; } else if (1.0f < t0_clamped) { t0_clamped = 1.0f; }
            if (0.0f <= t1 && t1 <= 1.0f) { t = (t1 + t0_clamped) * 0.5f; }
            else if (1.0f < t1) { t = (t0_clamped + 1.0f) * 0.5f; }
            else { t = (t0_clamped + 0.0f) * 0.5f; }
        }
    } else {
        real denom = cross_ab.i * cross_ab.i + cross_ab.j * cross_ab.j + cross_ab.k * cross_ab.k;
        real raw_s = vector3d_scalar_triple_product(&w0, b_direction, &cross_ab) / denom;
        real raw_t = vector3d_scalar_triple_product(&w0, a_direction, &cross_ab) / denom;
        int s_out_of_range = raw_s < 0.0f || 1.0f < raw_s;
        int t_out_of_range = raw_t < 0.0f || 1.0f < raw_t;

        if (!s_out_of_range && !t_out_of_range) {
            s = raw_s;
            t = raw_t;
        } else {
            if (s_out_of_range) {
                real_point3d point_on_a;
                real snapped_s = (0.0f <= raw_s) ? 1.0f : 0.0f;
                point_on_a.x = snapped_s * a_direction->i + a_start->x;
                point_on_a.y = snapped_s * a_direction->j + a_start->y;
                point_on_a.z = snapped_s * a_direction->k + a_start->z;
                if (ray_intersects_sphere_test(&point_on_a, b_start, b_direction, radius)) {
                    return 1;
                }
            }
            if (t_out_of_range) {
                real_point3d point_on_b;
                real snapped_t = (0.0f <= raw_t) ? 1.0f : 0.0f;
                point_on_b.x = snapped_t * b_direction->i + b_start->x;
                point_on_b.y = snapped_t * b_direction->j + b_start->y;
                point_on_b.z = snapped_t * b_direction->k + b_start->z;
                if (ray_intersects_sphere_test(&point_on_b, a_start, a_direction, radius)) {
                    return 1;
                }
            }
            return 0;
        }
    }

    dx = (t * b_direction->i + b_start->x) - (s * a_direction->i + a_start->x);
    dy = (t * b_direction->j + b_start->y) - (s * a_direction->j + a_start->y);
    dz = (t * b_direction->k + b_start->z) - (s * a_direction->k + a_start->z);
    if (dx * dx + dy * dy + dz * dz <= radius * radius) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4ceae0):

undefined4 segment3d_within_radius_of_segment(float *param_1,float param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  bool bVar4;
  bool bVar5;
  bool bVar6;
  char cVar7;
  float *unaff_EBX;
  float *unaff_ESI;
  float *unaff_EDI;
  float10 fVar8;
  float10 fVar9;
  float10 fVar10;
  float local_2c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  local_18 = *unaff_EBX - *param_1;
  local_14 = unaff_EBX[1] - param_1[1];
  local_10 = unaff_EBX[2] - param_1[2];
  local_c = unaff_ESI[1] * unaff_EDI[2] - unaff_EDI[1] * unaff_ESI[2];
  local_8 = unaff_ESI[2] * *unaff_EDI - *unaff_ESI * unaff_EDI[2];
  local_4 = unaff_EDI[1] * *unaff_ESI - unaff_ESI[1] * *unaff_EDI;
  if (ABS(local_c * local_c + local_8 * local_8 + local_4 * local_4) < 0.0001) {
    fVar1 = *unaff_ESI * *unaff_EDI + unaff_ESI[1] * unaff_EDI[1] + unaff_ESI[2] * unaff_EDI[2];
    fVar2 = *unaff_ESI * *unaff_ESI + unaff_ESI[2] * unaff_ESI[2] + unaff_ESI[1] * unaff_ESI[1];
    if (fVar2 <= 0.0001) {
      local_2c = 0.0;
    }
    else {
      fVar2 = 1.0 / fVar2;
      fVar3 = (local_18 * *unaff_ESI + local_10 * unaff_ESI[2] + local_14 * unaff_ESI[1]) * fVar2;
      fVar2 = fVar2 * fVar1 + fVar3;
      if (0.0 <= fVar3) {
        if (1.0 < fVar3) {
          fVar3 = 1.0;
        }
      }
      else {
        fVar3 = 0.0;
      }
      if (0.0 <= fVar2) {
        if (fVar2 <= 1.0) {
          local_2c = (fVar2 + fVar3) * 0.5;
        }
        else {
          local_2c = (fVar3 + 1.0) * 0.5;
        }
      }
      else {
        local_2c = (fVar3 + 0.0) * 0.5;
      }
    }
    fVar2 = *unaff_EDI * *unaff_EDI + unaff_EDI[2] * unaff_EDI[2] + unaff_EDI[1] * unaff_EDI[1];
    if (fVar2 <= 0.0001) {
      fVar8 = (float10)0.0;
    }
    else {
      fVar2 = 1.0 / fVar2;
      fVar3 = -((local_10 * unaff_EDI[2] + local_18 * *unaff_EDI + local_14 * unaff_EDI[1]) * fVar2)
      ;
      fVar1 = fVar2 * fVar1 + fVar3;
      if (0.0 <= fVar3) {
        if (fVar3 <= 1.0) {
          fVar8 = (float10)fVar3;
        }
        else {
          fVar8 = (float10)1.0;
        }
      }
      else {
        fVar8 = (float10)0.0;
      }
      if (0.0 <= fVar1) {
        if (fVar1 <= 1.0) {
          fVar8 = ((float10)fVar1 + fVar8) * (float10)0.5;
        }
        else {
          fVar8 = ((float10)1.0 + fVar8) * (float10)0.5;
        }
      }
      else {
        fVar8 = ((float10)0.0 + fVar8) * (float10)0.5;
      }
    }
LAB_004ceefe:
    fVar9 = (float10)(float)(fVar8 * (float10)*unaff_EDI + (float10)*unaff_EBX) -
            (float10)(local_2c * *unaff_ESI + *param_1);
    fVar10 = (float10)(float)(fVar8 * (float10)unaff_EDI[1] + (float10)unaff_EBX[1]) -
             (float10)(local_2c * unaff_ESI[1] + param_1[1]);
    fVar8 = (fVar8 * (float10)unaff_EDI[2] + (float10)unaff_EBX[2]) -
            (float10)(local_2c * unaff_ESI[2] + param_1[2]);
    if (fVar9 * fVar9 + fVar10 * fVar10 + fVar8 * fVar8 <= (float10)param_2 * (float10)param_2) {
      return 1;
    }
  }
  else {
    fVar8 = (float10)vector3d_scalar_triple_product(&local_18);
    local_2c = (float)fVar8;
    fVar8 = (float10)vector3d_scalar_triple_product(&local_18);
    if ((local_2c < 0.0) || (1.0 < local_2c)) {
      bVar4 = true;
    }
    else {
      bVar4 = false;
    }
    if ((fVar8 < (float10)0.0) || ((float10)1.0 < fVar8)) {
      bVar6 = true;
      bVar5 = true;
    }
    else {
      bVar5 = false;
      bVar6 = false;
    }
    if (bVar4) {
      if (0.0 <= local_2c) {
        fVar1 = 1.0;
      }
      else {
        fVar1 = 0.0;
      }
      local_18 = fVar1 * *unaff_ESI + *param_1;
      local_14 = fVar1 * unaff_ESI[1] + param_1[1];
      local_10 = fVar1 * unaff_ESI[2] + param_1[2];
    }
    else if (!bVar5) goto LAB_004ceefe;
    if (((bVar4) && (cVar7 = ray_intersects_sphere_test(param_2), bVar5 = bVar6, cVar7 != '\0')) ||
       ((bVar5 && (cVar7 = ray_intersects_sphere_test(param_2), cVar7 != '\0')))) {
      return 1;
    }
  }
  return 0;
}
#endif
