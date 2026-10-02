// input_clamp_unit_float  (Ghidra: FUN_0048c8a0; renamed per its behavior)
// address 0x48c8a0, size 53 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: objdump confirms the sole parameter is a normal cdecl stack float ([esp+4]) and the
//   two comparison constants are exactly 0.0 (0x00672ac0) and 1.0 (0x00672ac4).
// register convention: value on the stack (cdecl float), no registers.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

// Clamps a floating-point input value to the 0..1 range.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
float input_clamp_unit_float(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (1.0f < value) {
        return 1.0f;
    }
    return value;
}

#if 0
Original Ghidra decompilation (0x48c8a0), from tools/pack.py 0x48c8a0:

float10 FUN_0048c8a0(float param_1)

{
  if (param_1 < 0.0) {
    return (float10)0.0;
  }
  if (1.0 < param_1) {
    return (float10)1.0;
  }
  return (float10)param_1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
