// physics_clamp_value_to_spring_range  (Ghidra: FUN_0050b370; name from
//   out/phase4/physics_functions.md's own summary of its caller, 0x50b460: "Attempts to move a
//   clamped scalar value toward a target using physics_clamp_value_to_spring_range")
// address 0x50b370, size 227 bytes
// VERIFIED against disassembly 0x50b370..0x50b453 (2026-09-30). the sign tests are single fcomp / test ah idioms; the
//   negative branch always re-clamps against the lower bound, the positive branch returns early when within the upper
//   bound
// name confidence: 0.35   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x50b370..0x50b452; rate offsets probed)
// evidence: out/phase4/physics_functions.md summary of this address ("Clamps a scalar
//   spring-related value against direction-dependent upper and lower limits scaled by a rate
//   parameter"); types/physics.h physics_scalar_rates field order (maximum_positive,
//   maximum_negative, acceleration_positive, acceleration_negative), which matches in_EDX[0..3]
//   here exactly (in_EDX[2]/[3] are the two acceleration scales used first, in_EDX[0]/[1] the
//   two maximum scales used last).
// register convention: in_ECX -> value (float *, read-modify-write), in_EDX -> rates
//   (physics_scalar_rates *). param_1 is Ghidra's own recognized stack parameter (a signed step
//   magnitude: its sign picks the positive or negative branch, its absolute value scales every
//   limit).
//   // blam-cc: ECX -> value, EDX -> rates, stack -> step

#include "tags.h"
#include "math.h"
#include "physics.h"

extern double fabs(double x); // ABS is a single x87 FABS instruction

// Advances *value one spring step in the direction of step's sign (using the matching
// acceleration_positive/acceleration_negative scale, itself scaled by |step|, as a spring
// toward the corresponding zero-crossing), then clamps the result to
// +/-(|step| * maximum_positive/maximum_negative).
void physics_clamp_value_to_spring_range(float *value, physics_scalar_rates *rates, float step)
{
    float magnitude = (float)fabs((double)step);
    float accel_positive = magnitude * rates->acceleration_positive;
    float accel_negative = magnitude * rates->acceleration_negative;

    if (step <= 0.0f) {
        if (0.0f <= step) {
            return; // step == 0.0f exactly: no-op
        }

        float current = *value;
        float stepped;
        if (*value < accel_negative) {
            if (current > 0.0f) {
                stepped = (current / accel_negative - 1.0f) * accel_positive;
            } else {
                stepped = current - accel_positive;
            }
        } else {
            stepped = current - accel_negative;
        }
        *value = stepped;

        float lower_bound = -(magnitude * rates->maximum_negative);
        if (lower_bound <= *value) {
            lower_bound = *value;
        }
        *value = lower_bound;
        return;
    }

    float stepped;
    if (-accel_negative < *value) {
        if (*value < 0.0f) {
            stepped = (*value / accel_negative + 1.0f) * accel_positive;
        } else {
            stepped = accel_positive + *value;
        }
    } else {
        stepped = accel_negative + *value;
    }
    *value = stepped;

    float upper_bound = magnitude * rates->maximum_positive;
    if (*value <= upper_bound) {
        return; // already within the upper bound
    }
    *value = upper_bound;
}

#if 0
Original Ghidra decompilation (0x50b370):

void FUN_0050b370(float param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float *in_ECX;
  float *in_EDX;

  fVar2 = ABS(param_1);
  fVar4 = fVar2 * in_EDX[2];
  fVar3 = fVar2 * in_EDX[3];
  if (param_1 <= 0.0) {
    if (0.0 <= param_1) {
      return;
    }
    fVar1 = *in_ECX;
    if (*in_ECX < fVar3) {
      if (fVar1 < 0.0 == (fVar1 == 0.0)) {
        fVar1 = (*in_ECX / fVar3 - 1.0) * fVar4;
      }
      else {
        fVar1 = *in_ECX - fVar4;
      }
    }
    else {
      fVar1 = fVar1 - fVar3;
    }
    *in_ECX = fVar1;
    fVar2 = -(fVar2 * in_EDX[1]);
    if (fVar2 <= *in_ECX) {
      fVar2 = *in_ECX;
    }
  }
  else {
    if (-fVar3 < *in_ECX) {
      if (*in_ECX < 0.0) {
        fVar3 = (*in_ECX / fVar3 + 1.0) * fVar4;
      }
      else {
        fVar3 = fVar4 + *in_ECX;
      }
    }
    else {
      fVar3 = fVar3 + *in_ECX;
    }
    *in_ECX = fVar3;
    fVar2 = fVar2 * *in_EDX;
    if (*in_ECX <= fVar2) {
      *in_ECX = *in_ECX;
      return;
    }
  }
  *in_ECX = fVar2;
  return;
}
#endif
