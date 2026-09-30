// control_axis_sign  (Ghidra: FUN_00471070; renamed, no established name)
// address 0x471070, size 57 bytes
// name confidence: 0.3   rewrite confidence: 0.7
// evidence: out/phase4/game_functions.md ("Returns the sign of a control-stick axis value
// outside a small deadzone, or zero within it").

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"

// Returns 1.0 if value > 0.05, -1.0 if value < -0.05, otherwise 0.0.
real control_axis_sign(real value)
{
    real result = 0.0f;

    if (value > 0.05f) {
        result = 1.0f;
    }
    if (value < -0.05f) {
        result = -1.0f;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x471070), from tools/pack.py 0x471070:

float10 FUN_00471070(float param_1)

{
  float10 fVar1;

  fVar1 = (float10)0.0;
  if (0.05 < param_1) {
    fVar1 = (float10)1.0;
  }
  if (param_1 < -0.05) {
    fVar1 = (float10)-1.0;
  }
  return fVar1;
}
#endif
