// physics_scalar_step_to_target_clamped  (Ghidra: FUN_0050b460, still unnamed there; name
//   chosen to match this batch's other physics_scalar_* helpers)
// address 0x50b460, size 110 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (VERIFIED against objdump 0x50b460..0x50b4cd)
// evidence: out/phase4/physics_functions.md summary ("Attempts to move a clamped scalar value
//   toward a target using physics_clamp_value_to_spring_range, reporting whether the target was
//   reached"), which also supplies physics_clamp_value_to_spring_range's own name (0x50b370,
//   this module).
// register convention: in_ECX -> value (float *, read-modify-write). param_1/param_2 are
//   Ghidra's own recognized stack parameters (target, step), confirmed by the call into
//   physics_clamp_value_to_spring_range with param_2 (negated on the "above target" path) as
//   its step argument.
//   // blam-cc: ECX -> value, stack -> target, step
// UNSURE (major): this function also needs a rates (physics_scalar_rates *) to forward to
//   physics_clamp_value_to_spring_range, but never dereferences it itself, so Ghidra never
//   surfaces it as an in_EDX local -- the register is simply passed through untouched. This
//   rewrite adds it as a leading EDX parameter per the blam-cc ordering, matching
//   physics_clamp_value_to_spring_range's own rates parameter, since there is no other way for
//   the callee to receive it.
//   // blam-cc: EDX -> rates (inferred pass-through, not visible in this function's own body)
// UNSURE: return type is undefined4 in Ghidra with only the low byte meaningfully set (0 or 1);
//   declared uint8_t.

#include "tags.h"
#include "math.h"
#include "physics.h"
#include "fn_physics.h"


// Steps *value one spring tick toward target (up by step if below, down by step if above), then
// reports whether that step reached or passed target. Snaps *value exactly to target on the
// tick it is reached; leaves it at the stepped-and-clamped value otherwise.
uint8_t physics_scalar_step_to_target_clamped(physics_scalar_rates *rates, float *value, float target, float step)
{
    if (*value <= target) {
        if (*value < target) {
            physics_clamp_value_to_spring_range(value, rates, step);
            if (*value < target) {
                return 0; // still below target
            }
            *value = target;
            return 1;
        }
        // *value == target already: fall through, report reached
    } else {
        physics_clamp_value_to_spring_range(value, rates, -step);
        if (*value > target) {
            return 0; // still above target
        }
        *value = target;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x50b460):

undefined4 FUN_0050b460(float param_1,float param_2)

{
  float *in_ECX;
  float *extraout_ECX;
  float *extraout_ECX_00;

  if (*in_ECX <= param_1) {
    if (*in_ECX < param_1) {
      FUN_0050b370(param_2);
      if (*extraout_ECX_00 < param_1) {
        return 0;
      }
      *extraout_ECX_00 = param_1;
      return 1;
    }
  }
  else {
    FUN_0050b370(-param_2);
    if (*extraout_ECX < param_1 == (*extraout_ECX == param_1)) {
      return 0;
    }
    *extraout_ECX = param_1;
  }
  return 1;
}
#endif
