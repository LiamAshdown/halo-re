// physics_scalar_approach_direction  (Ghidra: FUN_0050b4d0, still unnamed there; name chosen to
//   match this batch's other physics_scalar_* helpers)
// address 0x50b4d0, size 86 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: out/phase4/physics_functions.md summary ("Computes which direction, if any, a value
//   should move in order to approach a target, optionally accounting for wraparound");
//   physics_scalar_move_toward_target (0x50b2f0, this module) calls it twice, once before and
//   once after stepping the value, and compares the two results for equality to decide whether
//   the step overshot the target -- consistent with a signed "which way" helper.
// register convention: in_ECX -> range (physics_scalar_range *), used only for the wraparound
//   distance check. param_1/param_2/param_3 are Ghidra's own recognized stack parameters
//   (value, wrap, target).
//   // blam-cc: ECX -> range, stack -> value, wrap, target
// UNSURE: ownership of this whole physics_scalar_* family is unresolved; see
//   physics_scalar_advance_and_wrap.c.

#include "tags.h"
#include "math.h"
#include "physics.h"
#include "fn_physics.h"

extern double fabs(double x); // ABS is a single x87 FABS instruction

// Returns +1.0 / -1.0 for the direction *value* should move to reach *target*, or 0.0 when it
// is already there. When wrap is set and going directly would cross more than half the range's
// span, the direction is flipped so the caller wraps the short way around instead.
float physics_scalar_approach_direction(physics_scalar_range *range, float value, uint8_t wrap, float target)
{
    float delta = target - value;
    if (delta != 0.0f) {
        if (wrap != 0 && (range->upper - range->lower) * 0.5f < (float)fabs((double)delta)) {
            delta = -delta;
        }
        return (delta > 0.0f) ? 1.0f : -1.0f;
    }
    return delta; // 0.0f
}

#if 0
Original Ghidra decompilation (0x50b4d0):

float10 FUN_0050b4d0(float param_1,char param_2,float param_3)

{
  float *in_ECX;
  float10 fVar1;

  fVar1 = (float10)param_3 - (float10)param_1;
  if (fVar1 != (float10)0.0) {
    if ((param_2 != '\0') && (((float10)*in_ECX - (float10)in_ECX[1]) * (float10)0.5 < ABS(fVar1)))
    {
      fVar1 = -fVar1;
    }
    if ((float10)0.0 < fVar1) {
      return (float10)1.0;
    }
    fVar1 = (float10)-1.0;
  }
  return fVar1;
}
#endif
