// value_step_toward_target  (Ghidra: FUN_00470d40; renamed, no established name)
// address 0x470d40, size 64 bytes
// name confidence: 0.35   rewrite confidence: 0.6
// evidence: out/phase4/game_functions.md ("Advances a value pointed to by ECX toward a target by
// at most a given maximum step per call").
// register convention: the value pointer is Ghidra's `in_ECX`; `target` and `max_step` are this
// function's own recognized stack parameters.
//   // blam-cc: ECX -> value, stack -> target, max_step

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

// blam-cc: ECX -> value, stack -> target, max_step
// Moves *value toward target by at most max_step (in either direction) and writes the result
// back through value.
void value_step_toward_target(float *value, float target, float max_step)
{
    float delta = target - *value;

    if (delta > max_step) {
        delta = max_step;
    } else if (delta < -max_step) {
        delta = -max_step;
    }
    *value = *value + delta;
}

#if 0
Original Ghidra decompilation (0x470d40), from tools/pack.py 0x470d40:

void FUN_00470d40(float param_1,float param_2)

{
  float fVar1;
  float *in_ECX;

  param_1 = param_1 - *in_ECX;
  fVar1 = -param_2;
  if ((-param_2 <= param_1) && (fVar1 = param_1, param_2 < param_1)) {
    *in_ECX = param_2 + *in_ECX;
    return;
  }
  *in_ECX = fVar1 + *in_ECX;
  return;
}
#endif
