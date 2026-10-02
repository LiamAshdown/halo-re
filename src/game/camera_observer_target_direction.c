// camera_observer_target_direction  (Ghidra: FUN_00459cc0; renamed per symbols/review_queue.txt)
// address 0x459cc0, size 272 bytes
// name confidence: 0.3   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x459cc0..0x459dcf)
// evidence: symbols/review_queue.txt 0x459cc0 "computes a normalized direction vector between
//   two points ..., then a clamped dot-product angle via FUN_00628140 (acos-style)"; the
//   clamp-then-acos collapse follows the same pattern already used and documented in
//   src/math/vector3d_angle_between_4cd5e0.c.
// register convention: candidate/closest point in EAX (in_EAX), observer facing vector in ECX
//   (in_ECX), reference/observer position in ESI (unaff_ESI); object handle, an unused second
//   value, and the three output pointers are the recognized stack parameters (param_1..param_5).
//   // blam-cc: EAX -> candidate_point, ECX -> facing, ESI -> reference_position,
//   //          stack -> object, unused, out_direction, out_distance, out_angle
//
// UNSURE: `FUN_0045a280()` is called here with zero visible arguments, but its own file
// (vector3d_closest_point_on_segment.c) needs five. Only `object` (this function's param_1) and
// `reference_position` (unaff_ESI) have a confident source here; `aux_vector` is modelled as the
// facing vector (the only other vector this function has in scope) and the nudge clamp bound
// could not be recovered at all, so it is passed as 0.0, which degenerates the nudge refinement
// to a no-op (out_closest is left at the raw ray projection) rather than guessing a number.
// `candidate_point` (in_EAX) is therefore expected to already equal `&out_closest` from that
// call -- i.e. this function assumes its caller aliases in_EAX with a local it owns.

#include "tags.h"
#include "math.h"
#include "memory.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void vector3d_closest_point_on_segment(datum_index unit_index, real_vector3d *aux_vector,
    real_point3d *reference_point, real_point3d *out_closest); // 0x45a280, ECX unit, EBX aux, stack (reference, out)
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, vector in ECX
extern char camera_observer_target_is_valid(datum_index exclude_object,
    real_point3d *observer_position, real_point3d *target_position,
    datum_index target_object); // this batch, 0x459dd0
extern double acos(double x); // 0x628140, CRT/compiler helper; operand on the x87 stack

// Computes the closest point on `object`'s look ray to `reference_position`, then the direction,
// distance and angle (against `facing`) from `reference_position` to that point. Returns 0 when
// `object` fails camera_observer_target_is_valid's line-of-sight check.
uint32_t camera_observer_target_direction(real_point3d *candidate_point, real_vector3d *facing,
                                           real_point3d *reference_position, datum_index object,
                                           datum_index exclude_object,
                                           real_vector3d *out_direction, real *out_distance, real *out_angle)
    // blam-cc: EAX -> candidate_point, ECX -> facing, ESI -> reference_position,
    //          stack -> object, exclude_object, out_direction, out_distance, out_angle
    // CORRECTED (phase 4 review): the second stack parameter is NOT unused -- 0x459ce3 loads it
    // into EAX immediately before calling 0x459dd0, which reads EAX as its exclude_object.
{
    real dot;

    vector3d_closest_point_on_segment(object, facing, reference_position, candidate_point); // 0x459cc9..0x459cd9: 2 stack args
    if (camera_observer_target_is_valid(exclude_object, reference_position,
                                        candidate_point, object) != 0) {
        out_direction->i = candidate_point->x - reference_position->x;
        out_direction->j = candidate_point->y - reference_position->y;
        out_direction->k = candidate_point->z - reference_position->z;
        *out_distance = vector3d_normalize_with_length(out_direction);
        if (*out_distance != 0.0f) {
            dot = facing->i * out_direction->i + facing->j * out_direction->j + facing->k * out_direction->k;
            if (dot < -1.0f) {
                dot = -1.0f;
            } else if (1.0f < dot) {
                dot = 1.0f;
            }
            *out_angle = (real)acos((double)dot);
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x459cc0), from tools/pack.py 0x459cc0:

undefined4
FUN_00459cc0(undefined4 param_1,undefined4 param_2,float *param_3,float *param_4,float *param_5)

{
  float fVar1;
  char cVar2;
  float *in_EAX;
  float *in_ECX;
  float *unaff_ESI;
  float10 fVar3;
  float10 extraout_ST0;
  float10 extraout_ST0_00;
  float10 extraout_ST0_01;

  FUN_0045a280();
  cVar2 = FUN_00459dd0(param_1);
  if (cVar2 != '\0') {
    *param_3 = *in_EAX - *unaff_ESI;
    param_3[1] = in_EAX[1] - unaff_ESI[1];
    param_3[2] = in_EAX[2] - unaff_ESI[2];
    fVar3 = (float10)vector3d_normalize_with_length();
    *param_4 = (float)fVar3;
    if (fVar3 != (float10)0.0) {
      fVar1 = *in_ECX * *param_3 + in_ECX[1] * param_3[1] + in_ECX[2] * param_3[2];
      if (fVar1 < -1.0) {
        FUN_00628140();
        *param_5 = (float)extraout_ST0;
        return 1;
      }
      if (1.0 < fVar1) {
        FUN_00628140();
        *param_5 = (float)extraout_ST0_00;
        return 1;
      }
      FUN_00628140();
      *param_5 = (float)extraout_ST0_01;
      return 1;
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
