// vector3d_closest_point_on_segment  (Ghidra: FUN_0045a280; renamed per symbols/review_queue.txt)
// address 0x45a280, size 533 bytes
// name confidence: 0.45   rewrite confidence: 0.85
// evidence: disassembly (objdump -d -M intel --start-address=0x45a280 --stop-address=0x45a4a0
//   bin/halo.exe) proves the call at the top is unit_get_look_origin_and_direction (0x55a390,
//   already rewritten at src/units/unit_get_look_origin_and_direction.c): the "direction" and
//   "origin" locals Ghidra names are exactly that function's ESI/EDI outputs. The target's own
//   look ray (origin, direction) is the "segment" this function projects onto; the point being
//   tested is `reference_point`, and the closest point along that ray (clamped to [origin,
//   origin+direction]) is written to `out_closest`.
// register convention: target unit index in ECX (in_ECX, forwarded to unit_get_look_origin_
//   and_direction), a third vector in EBX (unaff_EBX) whose real identity was not pinned down
//   (see UNSURE below), reference point and output point as the two recognized stack parameters
//   (param_1, param_2).
//   // blam-cc: ECX -> unit_index, EBX -> aux_vector, stack -> reference_point, out_closest
//
// UNSURE: `aux_vector` (EBX) is read as a plain 3-float vector by both the degeneracy check
// (cross(direction, aux_vector)) and the parametric projection, but at the one call site this
// batch can see (camera_observer_target_score, 0x459b10) the live EBX value at that point is
// that function's own `cone` argument (a 4-float observer_target_cone, not a vector) -- the
// same buffer is evidently read under two different shapes by this subsystem. Modelled here as
// a plain real_vector3d* so the two interpretations both compile against the same bytes.
// UNSURE: the disassembly shows a trailing adjustment -- clamp_length is applied to a small
// "nudge" vector (the projection of (out_closest - reference_point) onto aux_vector, negated and
// added back to the raw delta), not to `out_closest` itself as Ghidra's own decompile renders
// it; the rewrite below follows the disassembly. The clamp's maximum-length bound (loaded from a
// stack slot this function's own two recognized parameters don't account for) could not be
// traced to its origin; modelled as an extra explicit parameter rather than an invented
// constant, so the compiled behavior depends on whatever the caller supplies.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void unit_get_look_origin_and_direction(uint32_t object_index, uint32_t *out_status,
    real_vector3d *out_direction, real_point3d *out_origin); // 0x55a390, units module
extern void vector3d_clamp_length(real_vector3d *v, real max_length); // this batch, 0x459300

// Projects `reference_point` onto the target unit's look ray (its eye origin extended along its
// looking direction), clamping the projection to the ray's forward half, then nudges the result
// back along `aux_vector` by the (clamp-limited) component of the raw delta along that axis.
// REWRITTEN 2026-09-27 (static loop) from objdump 0x45a280..0x45a494: only TWO stack arguments (reference_point,
// out_closest). The nudge clamp is the float unit_get_look_origin_and_direction writes through its second argument
// (the target tag's autoaim width, +0x458) -- the binary passes the address of its own out_closest slot, having saved
// out_closest in EBP. The draft took an extra clamp parameter, which the callers filled with 0.0 / a cone distance.
void vector3d_closest_point_on_segment(datum_index unit_index, real_vector3d *aux_vector,
                                        real_point3d *reference_point, real_point3d *out_closest)
    // blam-cc: ECX -> unit_index, EBX -> aux_vector, stack -> reference_point, out_closest
{
    real_vector3d direction;
    real_point3d origin;
    uint32_t autoaim_width_bits; // float bits of the target tag's autoaim width
    real_vector3d cross;
    real cross_length_squared;
    real t;
    real_vector3d delta;
    real_vector3d nudge;
    real dot;

    unit_get_look_origin_and_direction(unit_index, &autoaim_width_bits, &direction, &origin);

    cross.i = direction.j * aux_vector->k - direction.k * aux_vector->j;
    cross.j = direction.k * aux_vector->i - direction.i * aux_vector->k;
    cross.k = direction.i * aux_vector->j - direction.j * aux_vector->i;
    cross_length_squared = cross.i * cross.i + cross.j * cross.j + cross.k * cross.k;

    if (cross_length_squared <= 0.0f) {
        *out_closest = origin;
    } else {
        t = (((reference_point->y - origin.y) * aux_vector->k - (reference_point->z - origin.z) * aux_vector->j) * cross.i +
             ((reference_point->z - origin.z) * aux_vector->i - (reference_point->x - origin.x) * aux_vector->k) * cross.j +
             ((reference_point->x - origin.x) * aux_vector->j - (reference_point->y - origin.y) * aux_vector->i) * cross.k) /
            cross_length_squared;
        if (t < 0.0f) {
            t = 0.0f;
        } else if (1.0f < t) {
            t = 1.0f;
        }
        out_closest->x = direction.i * t + origin.x;
        out_closest->y = direction.j * t + origin.y;
        out_closest->z = direction.k * t + origin.z;
    }

    delta.i = out_closest->x - reference_point->x;
    delta.j = out_closest->y - reference_point->y;
    delta.k = out_closest->z - reference_point->z;
    dot = -(delta.j * aux_vector->j + delta.k * aux_vector->k + delta.i * aux_vector->i);

    nudge.i = dot * aux_vector->i + delta.i;
    nudge.j = dot * aux_vector->j + delta.j;
    nudge.k = dot * aux_vector->k + delta.k;
    vector3d_clamp_length(&nudge, *(real *)&autoaim_width_bits);

    out_closest->x = out_closest->x - nudge.i;
    out_closest->y = out_closest->y - nudge.j;
    out_closest->z = out_closest->z - nudge.k;
}

#if 0
Original Ghidra decompilation (0x45a280), from tools/pack.py 0x45a280:

void FUN_0045a280(float *param_1,float *param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float *unaff_EBX;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;

  FUN_0055a390();
  fVar1 = local_2c * unaff_EBX[2] - local_28 * unaff_EBX[1];
  fVar2 = local_28 * *unaff_EBX - local_30 * unaff_EBX[2];
  fVar3 = local_30 * unaff_EBX[1] - local_2c * *unaff_EBX;
  fVar4 = fVar1 * fVar1 + fVar2 * fVar2 + fVar3 * fVar3;
  if (fVar4 <= 0.0) {
    *param_2 = local_24;
    param_2[1] = local_20;
    param_2[2] = local_1c;
  }
  else {
    fVar4 = (((param_1[1] - local_20) * unaff_EBX[2] - (param_1[2] - local_1c) * unaff_EBX[1]) *
             fVar1 + ((param_1[2] - local_1c) * *unaff_EBX - (*param_1 - local_24) * unaff_EBX[2]) *
                     fVar2 + ((*param_1 - local_24) * unaff_EBX[1] -
                             (param_1[1] - local_20) * *unaff_EBX) * fVar3) / fVar4;
    if (0.0 <= fVar4) {
      if (1.0 < fVar4) {
        fVar4 = 1.0;
      }
    }
    else {
      fVar4 = 0.0;
    }
    *param_2 = local_30 * fVar4 + local_24;
    param_2[1] = local_2c * fVar4 + local_20;
    param_2[2] = fVar4 * local_28 + local_1c;
  }
  fVar1 = *param_2;
  fVar2 = *param_1;
  fVar3 = param_2[1];
  fVar4 = param_1[1];
  fVar5 = param_2[2];
  fVar6 = param_1[2];
  fVar10 = -((fVar3 - fVar4) * unaff_EBX[1] +
            (fVar5 - fVar6) * unaff_EBX[2] + (fVar1 - fVar2) * *unaff_EBX);
  fVar7 = *unaff_EBX;
  fVar8 = unaff_EBX[1];
  fVar9 = unaff_EBX[2];
  vector3d_clamp_length(param_2);
  *param_2 = *param_2 - (fVar10 * fVar7 + (fVar1 - fVar2));
  param_2[1] = param_2[1] - (fVar10 * fVar8 + (fVar3 - fVar4));
  param_2[2] = param_2[2] - (fVar10 * fVar9 + (fVar5 - fVar6));
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
