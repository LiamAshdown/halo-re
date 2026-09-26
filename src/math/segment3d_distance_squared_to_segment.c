// segment3d_distance_squared_to_segment  (Ghidra: segment3d_distance_squared_to_segment, already named)
// address 0x4cdef0, size 1187 bytes
// name confidence: 0.6   rewrite confidence: 0.35
// evidence: math_functions.md: "Computes the squared distance between the closest points of two
//   3D line segments, handling the parallel/degenerate cases." segment_a = a_start + s*a_dir for
//   s in [0,1], segment_b = b_start + t*b_dir for t in [0,1]; w0 = b_start - a_start;
//   n = a_dir x b_dir. When |n|^2 < 0.0001 the segments are (near-)parallel and s, t are found
//   by projecting each start point onto the other segment and averaging the two clamped
//   estimates; otherwise raw s, t solve the two-line system and are snapped to the nearest
//   endpoint (falling back to point3d_distance_squared_to_segment @0x4cde30 against the *other*
//   segment) whenever either falls outside [0,1].
//
// UNSURE, significantly: the non-parallel branch calls `vector3d_scalar_triple_product` (an
// out-of-module helper @0x44d8e0, per math_types_notes.md item 7) twice, both times showing
// only one visible argument (`&local_18`, i.e. &w0) -- the remaining arguments (almost
// certainly a_direction, b_direction and/or n, still live in registers from earlier in the
// function) are not recoverable from this decompile. The raw-s/raw-t computation below is
// reconstructed from the standard two-line closest-point formula (s, t solve
// [w0 x d2]&[w0 x d1] against n = d1 x d2, scaled by 1/|n|^2), which is mathematically
// consistent with the summary and with `n` already being on hand as `cross_ab`, but it is a
// reconstruction, not a literal transliteration -- particularly the sign of `w0` in each
// triple-product call could not be confirmed.

#include "tags.h"
#include "math.h"

extern double fabs(double x); // ABS is a single x87 FABS instruction
extern real vector3d_scalar_triple_product(const real_vector3d *a, const real_vector3d *b, const real_vector3d *c); // 0x44d8e0, a . (b x c); outside this module
extern real point3d_distance_squared_to_segment(real_point3d *segment_start, real_vector3d *segment_direction, real_point3d *point); // 0x4cde30

// FIXED (register inputs, objdump + difftest): the original never reads EAX; b_start arrive(s) on the stack (1 stack argument(s)).
// blam-cc: EBX -> a_start, ESI -> a_direction, EDI -> b_direction, stack -> b_start
real segment3d_distance_squared_to_segment(real_point3d *b_start, real_point3d *a_start, real_vector3d *a_direction, real_vector3d *b_direction)
{
    real_vector3d w0; // b_start - a_start
    real_vector3d cross_ab; // a_direction x b_direction (n)
    real s; // parameter along segment a, in [0,1] when both segments are used directly
    real t; // parameter along segment b
    real dx, dy, dz;

    w0.i = b_start->x - a_start->x;
    w0.j = b_start->y - a_start->y;
    w0.k = b_start->z - a_start->z;

    cross_ab.i = a_direction->j * b_direction->k - b_direction->j * a_direction->k;
    cross_ab.j = a_direction->k * b_direction->i - a_direction->i * b_direction->k;
    cross_ab.k = b_direction->j * a_direction->i - a_direction->j * b_direction->i;

    if (fabs((double)(cross_ab.i * cross_ab.i + cross_ab.j * cross_ab.j + cross_ab.k * cross_ab.k)) < 0.0001) {
        // (near-)parallel segments: no unique closest pair, so estimate s (along a) and t
        // (along b) independently from each start point's projection onto the other segment,
        // clamp each raw projection to its segment, and average the clamped projection with the
        // (still unclamped) combined estimate.
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
        // Not parallel: solve for s (along a) and t (along b) directly; see the header note on
        // vector3d_scalar_triple_product for the uncertainty in this reconstruction.
        real denom = cross_ab.i * cross_ab.i + cross_ab.j * cross_ab.j + cross_ab.k * cross_ab.k;
        real raw_s = vector3d_scalar_triple_product(&w0, b_direction, &cross_ab) / denom;
        real raw_t = vector3d_scalar_triple_product(&w0, a_direction, &cross_ab) / denom;
        int s_out_of_range = raw_s < 0.0f || 1.0f < raw_s;
        int t_out_of_range = raw_t < 0.0f || 1.0f < raw_t;

        if (s_out_of_range || t_out_of_range) {
            real dist_a = 3.4028235e+38f; // squared distance from the snapped point on a to segment b
            real dist_b = 3.4028235e+38f; // squared distance from the snapped point on b to segment a

            if (s_out_of_range) {
                real_point3d point_on_a;
                real snapped_s = (0.0f <= raw_s) ? 1.0f : 0.0f;
                point_on_a.x = snapped_s * a_direction->i + a_start->x;
                point_on_a.y = snapped_s * a_direction->j + a_start->y;
                point_on_a.z = snapped_s * a_direction->k + a_start->z;
                dist_a = point3d_distance_squared_to_segment(b_start, b_direction, &point_on_a);
            }
            if (t_out_of_range) {
                real_point3d point_on_b;
                real snapped_t = (0.0f <= raw_t) ? 1.0f : 0.0f;
                point_on_b.x = snapped_t * b_direction->i + b_start->x;
                point_on_b.y = snapped_t * b_direction->j + b_start->y;
                point_on_b.z = snapped_t * b_direction->k + b_start->z;
                dist_b = point3d_distance_squared_to_segment(a_start, a_direction, &point_on_b);
            }

            return (dist_a <= dist_b) ? dist_a : dist_b;
        }

        s = raw_s;
        t = raw_t;
    }

    dx = (t * b_direction->i + b_start->x) - (s * a_direction->i + a_start->x);
    dy = (t * b_direction->j + b_start->y) - (s * a_direction->j + a_start->y);
    dz = (t * b_direction->k + b_start->z) - (s * a_direction->k + a_start->z);
    return dx * dx + dy * dy + dz * dz;
}

#if 0
Original Ghidra decompilation (0x4cdef0):

float10 segment3d_distance_squared_to_segment(float *param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  bool bVar4;
  bool bVar5;
  float *unaff_EBX;
  float *unaff_ESI;
  float *unaff_EDI;
  float10 fVar6;
  float10 fVar7;
  float local_30;
  float local_2c;
  float local_28;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  local_18 = *param_1 - *unaff_EBX;
  local_14 = param_1[1] - unaff_EBX[1];
  local_10 = param_1[2] - unaff_EBX[2];
  local_c = unaff_ESI[1] * unaff_EDI[2] - unaff_EDI[1] * unaff_ESI[2];
  local_8 = unaff_ESI[2] * *unaff_EDI - *unaff_ESI * unaff_EDI[2];
  local_4 = unaff_EDI[1] * *unaff_ESI - unaff_ESI[1] * *unaff_EDI;
  if (ABS(local_c * local_c + local_8 * local_8 + local_4 * local_4) < 0.0001) {
    fVar1 = unaff_ESI[1] * unaff_EDI[1] + unaff_ESI[2] * unaff_EDI[2] + *unaff_ESI * *unaff_EDI;
    fVar2 = *unaff_ESI * *unaff_ESI + unaff_ESI[2] * unaff_ESI[2] + unaff_ESI[1] * unaff_ESI[1];
    if (fVar2 <= 0.0001) {
      fVar6 = (float10)0.0;
    }
    else {
      fVar2 = 1.0 / fVar2;
      fVar3 = (local_18 * *unaff_ESI + local_10 * unaff_ESI[2] + local_14 * unaff_ESI[1]) * fVar2;
      fVar2 = fVar2 * fVar1 + fVar3;
      if (0.0 <= fVar3) {
        if (fVar3 <= 1.0) {
          fVar6 = (float10)fVar3;
        }
        else {
          fVar6 = (float10)1.0;
        }
      }
      else {
        fVar6 = (float10)0.0;
      }
      if (0.0 <= fVar2) {
        if (fVar2 <= 1.0) {
          fVar6 = ((float10)fVar2 + fVar6) * (float10)0.5;
        }
        else {
          fVar6 = ((float10)1.0 + fVar6) * (float10)0.5;
        }
      }
      else {
        fVar6 = ((float10)0.0 + fVar6) * (float10)0.5;
      }
    }
    fVar2 = *unaff_EDI * *unaff_EDI + unaff_EDI[2] * unaff_EDI[2] + unaff_EDI[1] * unaff_EDI[1];
    if (fVar2 <= 0.0001) {
      local_30 = 0.0;
    }
    else {
      fVar2 = 1.0 / fVar2;
      fVar3 = -((local_10 * unaff_EDI[2] + local_18 * *unaff_EDI + local_14 * unaff_EDI[1]) * fVar2)
      ;
      fVar1 = fVar2 * fVar1 + fVar3;
      if (0.0 <= fVar3) {
        if (1.0 < fVar3) {
          fVar3 = 1.0;
        }
      }
      else {
        fVar3 = 0.0;
      }
      if (0.0 <= fVar1) {
        if (fVar1 <= 1.0) {
          local_30 = (fVar1 + fVar3) * 0.5;
        }
        else {
          local_30 = (fVar3 + 1.0) * 0.5;
        }
      }
      else {
        local_30 = (fVar3 + 0.0) * 0.5;
      }
    }
  }
  else {
    fVar6 = (float10)vector3d_scalar_triple_product(&local_18);
    fVar7 = (float10)vector3d_scalar_triple_product(&local_18);
    local_30 = (float)fVar7;
    fVar6 = (float10)(float)fVar6;
    if ((fVar6 < (float10)0.0) || ((float10)1.0 < fVar6)) {
      bVar4 = true;
    }
    else {
      bVar4 = false;
    }
    if ((local_30 < 0.0) || (bVar5 = false, 1.0 < local_30)) {
      bVar5 = true;
    }
    if ((bVar4) || (bVar5)) {
      local_28 = 3.4028235e+38;
      local_2c = 3.4028235e+38;
      if (bVar4) {
        if ((float10)0.0 <= fVar6) {
          fVar1 = 1.0;
        }
        else {
          fVar1 = 0.0;
        }
        local_18 = fVar1 * *unaff_ESI + *unaff_EBX;
        local_14 = fVar1 * unaff_ESI[1] + unaff_EBX[1];
        local_10 = fVar1 * unaff_ESI[2] + unaff_EBX[2];
        fVar6 = (float10)point3d_distance_squared_to_segment();
        local_28 = (float)fVar6;
      }
      if (bVar5) {
        if (0.0 <= local_30) {
          fVar1 = 1.0;
        }
        else {
          fVar1 = 0.0;
        }
        local_c = fVar1 * *unaff_EDI + *param_1;
        local_8 = fVar1 * unaff_EDI[1] + param_1[1];
        local_4 = fVar1 * unaff_EDI[2] + param_1[2];
        fVar6 = (float10)point3d_distance_squared_to_segment();
        local_2c = (float)fVar6;
      }
      if (local_28 <= local_2c) {
        return (float10)local_28;
      }
      return (float10)local_2c;
    }
  }
  fVar1 = (local_30 * *unaff_EDI + *param_1) -
          (float)(fVar6 * (float10)*unaff_ESI + (float10)*unaff_EBX);
  fVar2 = (local_30 * unaff_EDI[1] + param_1[1]) -
          (float)(fVar6 * (float10)unaff_ESI[1] + (float10)unaff_EBX[1]);
  fVar6 = (float10)(local_30 * unaff_EDI[2] + param_1[2]) -
          (fVar6 * (float10)unaff_ESI[2] + (float10)unaff_EBX[2]);
  return (float10)fVar1 * (float10)fVar1 + (float10)fVar2 * (float10)fVar2 + fVar6 * fVar6;
}
#endif
