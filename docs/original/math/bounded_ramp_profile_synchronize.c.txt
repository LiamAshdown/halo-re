// bounded_ramp_profile_synchronize  (Ghidra: FUN_00564840; renamed for this rewrite)
// address 0x564840, size 329 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: out/phase4/units_types_notes.md "0x564580, 0x564840, 0x564990 -- a bounded
//   acceleration/velocity ramp solver and its evaluator." Sits between the two: given two
//   profiles already built by bounded_ramp_profile_build (0x564580), each with its own total
//   duration (phase1 + phase2 + phase3), it stretches the SHORTER profile's cruise phase so both
//   finish at the same time -- used by the caller (0x564ae0) to keep an azimuth axis and an
//   elevation axis arriving together. Only runs when both profiles are still in their "not
//   coasting" state (byte 0 == 0); a profile already in the dead zone is left untouched.
// register convention: none recognized by Ghidra -- profile_a in ECX (in_ECX), profile_b in EDX
//   (in_EDX), rate (max_acceleration) as the one stack parameter.
//   // blam-cc: ECX -> profile_a, EDX -> profile_b, stack -> max_acceleration
// UNSURE: the exact rounding/re-solve at the end (a fresh quadratic against the longer profile's
//   own phase1_acceleration) is transcribed as decompiled; not independently re-derived.

#include "tags.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double sqrt(double x); // x87 FSQRT
extern double fabs(double x); // x87 FABS

// See src/math/bounded_ramp_profile_build.c for the profile buffer layout (TYPES-GAP, 0x20 bytes).

// If both profiles still have a live ramp (not already in the dead zone), extends whichever one
// finishes sooner so both profiles run for the same total duration.
void bounded_ramp_profile_synchronize(bounded_ramp_profile *profile_a, bounded_ramp_profile *profile_b,
                                      real max_acceleration)
{
    real duration_a, duration_b, extra;
    bounded_ramp_profile *shorter;
    real cruise_velocity, adjusted_duration, term, radicand;

    if (profile_a->within_dead_zone != 0 || profile_b->within_dead_zone != 0) {
        return;
    }

    duration_a = profile_a->phase3_duration + profile_a->phase2_duration + profile_a->phase1_duration;
    duration_b = profile_b->phase3_duration + profile_b->phase2_duration + profile_b->phase1_duration;

    // 0x56486f..0x5648c1, written as the x87 tests read (`> 0` / `<`), so a NaN takes the
    // "not this profile" branch exactly as the original does.
    if (profile_a->phase1_duration > 0.0f && duration_a < duration_b) {
        extra = duration_b - duration_a;
        shorter = profile_a;
    } else {
        if (!(profile_b->phase1_duration > 0.0f && duration_b < duration_a)) {
            return;
        }
        extra = duration_a - duration_b;
        shorter = profile_b;
    }

    if (shorter == 0) {
        return;
    }

    term = (extra + shorter->phase2_duration) * max_acceleration;
    radicand = term * term -
        -extra * (real)fabs((double)(shorter->phase1_duration * shorter->phase1_acceleration +
                                      shorter->start_velocity)) * max_acceleration * 4.0f;
    adjusted_duration = ((real)sqrt((double)radicand) - term) / (max_acceleration + max_acceleration);

    // min(+0x10, +0x1c) as 0x56490b..0x56491d computes it: +0x10 unless it is greater
    cruise_velocity = shorter->phase1_duration > shorter->phase3_duration ?
        shorter->phase3_duration : shorter->phase1_duration;
    if (cruise_velocity < adjusted_duration) {
        adjusted_duration = cruise_velocity;
    }

    if (0.0f < adjusted_duration) {
        real new_start_velocity = (shorter->phase1_duration - adjusted_duration) * shorter->phase1_acceleration +
            shorter->start_velocity;
        shorter->phase1_duration = shorter->phase1_duration - adjusted_duration;
        shorter->phase3_duration = shorter->phase3_duration - adjusted_duration;
        shorter->phase2_duration =
            ((new_start_velocity + new_start_velocity + adjusted_duration * shorter->phase1_acceleration) *
             adjusted_duration) / new_start_velocity;
    }
}

#if 0
Original Ghidra decompilation (0x564840):

void FUN_00564840(float param_1)

{
  float fVar1;
  float fVar2;
  char *in_ECX;
  char *in_EDX;

  if ((*in_ECX == '\0') && (*in_EDX == '\0')) {
    fVar2 = *(float *)(in_ECX + 0x1c) + *(float *)(in_ECX + 0x14) + *(float *)(in_ECX + 0x10);
    fVar1 = *(float *)(in_EDX + 0x1c) + *(float *)(in_EDX + 0x14) + *(float *)(in_EDX + 0x10);
    if ((*(float *)(in_ECX + 0x10) <= 0.0) || (fVar1 <= fVar2)) {
      if (*(float *)(in_EDX + 0x10) <= 0.0) {
        return;
      }
      if (fVar2 <= fVar1) {
        return;
      }
      fVar1 = fVar2 - fVar1;
    }
    else {
      fVar1 = fVar1 - fVar2;
      in_EDX = in_ECX;
    }
    if (in_EDX != (char *)0x0) {
      fVar2 = (fVar1 + *(float *)(in_EDX + 0x14)) * param_1;
      fVar2 = (SQRT(fVar2 * fVar2 -
                    -fVar1 * ABS(*(float *)(in_EDX + 0x10) * *(float *)(in_EDX + 0xc) +
                                 *(float *)(in_EDX + 8)) * param_1 * 4.0) - fVar2) /
              (param_1 + param_1);
      if (*(float *)(in_EDX + 0x10) <= *(float *)(in_EDX + 0x1c)) {
        fVar1 = *(float *)(in_EDX + 0x10);
      }
      else {
        fVar1 = *(float *)(in_EDX + 0x1c);
      }
      if (fVar1 < fVar2) {
        if (*(float *)(in_EDX + 0x10) <= *(float *)(in_EDX + 0x1c)) {
          fVar2 = *(float *)(in_EDX + 0x10);
        }
        else {
          fVar2 = *(float *)(in_EDX + 0x1c);
        }
      }
      if (0.0 < fVar2) {
        fVar1 = (*(float *)(in_EDX + 0x10) - fVar2) * *(float *)(in_EDX + 0xc) +
                *(float *)(in_EDX + 8);
        *(float *)(in_EDX + 0x10) = *(float *)(in_EDX + 0x10) - fVar2;
        *(float *)(in_EDX + 0x1c) = *(float *)(in_EDX + 0x1c) - fVar2;
        *(float *)(in_EDX + 0x14) =
             ((fVar1 + fVar1 + fVar2 * *(float *)(in_EDX + 0xc)) * fVar2) / fVar1;
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
