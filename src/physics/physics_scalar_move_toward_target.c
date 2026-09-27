// physics_scalar_move_toward_target  (Ghidra: FUN_0050b2f0, still unnamed there; name chosen to
//   match this batch's other physics_scalar_* helpers)
// address 0x50b2f0, size 127 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (VERIFIED against objdump 0x50b2f0..0x50b36e)
// evidence: out/phase4/physics_functions.md summary ("Moves a physics scalar toward a target
//   value at a fixed rate, snapping exactly to the target once reached"); calls
//   physics_scalar_approach_direction (0x50b4d0, this module) both before and after stepping,
//   and physics_scalar_advance_and_wrap (0x50b290, this module) to take the step itself.
// register convention: in_EDX -> value (float *, read-modify-write). param_1/param_2/param_3
//   are Ghidra's own recognized stack parameters (wrap, target, rate) -- confirmed by the call
//   into physics_scalar_approach_direction, whose own (value, wrap, target) parameter order
//   this function's param_1/param_2 match exactly.
//   // blam-cc: EDX -> value, stack -> wrap, target, rate
// UNSURE (major): this function also needs a range (physics_scalar_range *) to forward to both
//   callees, but never dereferences it itself, so Ghidra never surfaces it as an in_ECX local at
//   all -- the register is simply passed through untouched. This rewrite adds it as a leading
//   ECX parameter per the blam-cc ordering, matching physics_scalar_approach_direction's and
//   physics_scalar_advance_and_wrap's own range parameter, since there is no other way for
//   either callee to receive it.
//   // blam-cc: ESI -> range (0x50b302 / 0x50b340 copy it into ECX for physics_scalar_approach_direction)
// UNSURE: return type is undefined4 in Ghidra with only the low byte meaningfully set (0 or 1);
//   declared uint8_t.

#include "tags.h"
#include "math.h"
#include "physics.h"

extern float physics_scalar_approach_direction(physics_scalar_range *range, float value, uint8_t wrap, float target); // 0x50b4d0
extern void physics_scalar_advance_and_wrap(physics_scalar_range *range, float *value, uint8_t wrap, float delta); // 0x50b290

// Steps *value one tick toward target at the given rate, honoring range's wraparound the same
// way physics_scalar_advance_and_wrap does. Returns 1 and snaps *value exactly to target once
// the step reaches or passes it (or it was already there); returns 0 while still en route.
uint8_t physics_scalar_move_toward_target(physics_scalar_range *range, float *value, uint8_t wrap, float target, float rate)
{
    float direction = physics_scalar_approach_direction(range, *value, wrap, target);
    if (direction != 0.0f) {
        physics_scalar_advance_and_wrap(range, value, wrap, direction * rate);
        if (physics_scalar_approach_direction(range, *value, wrap, target) == direction) {
            return 0; // still approaching, hasn't reached or passed the target yet
        }
    }
    *value = target;
    return 1;
}

#if 0
Original Ghidra decompilation (0x50b2f0):

undefined4 FUN_0050b2f0(undefined4 param_1,undefined4 param_2,float param_3)

{
  float fVar1;
  undefined2 extraout_var;
  undefined4 uVar2;
  undefined2 extraout_var_00;
  undefined4 *in_EDX;
  undefined4 *extraout_EDX;
  undefined4 *extraout_EDX_00;
  undefined4 *extraout_EDX_01;
  undefined4 *puVar3;
  float10 fVar4;
  float10 fVar5;

  fVar4 = (float10)FUN_0050b4d0(*in_EDX,param_1,param_2);
  fVar1 = (float)fVar4;
  uVar2 = CONCAT22(extraout_var,
                   (ushort)(fVar1 < 0.0) << 8 | (ushort)NAN(fVar1) << 10 |
                   (ushort)(fVar1 == 0.0) << 0xe);
  puVar3 = extraout_EDX;
  if (fVar1 != 0.0) {
    FUN_0050b290(fVar1 * param_3);
    fVar4 = (float10)FUN_0050b4d0(*extraout_EDX_00,param_1,param_2);
    fVar5 = (float10)fVar1;
    uVar2 = CONCAT22(extraout_var_00,
                     (ushort)(fVar5 < fVar4) << 8 | (ushort)(NAN(fVar5) || NAN(fVar4)) << 10 |
                     (ushort)(fVar5 == fVar4) << 0xe);
    puVar3 = extraout_EDX_01;
    if (fVar5 == fVar4) {
      return uVar2;
    }
  }
  *puVar3 = param_2;
  return CONCAT31((int3)((uint)uVar2 >> 8),1);
}
#endif
