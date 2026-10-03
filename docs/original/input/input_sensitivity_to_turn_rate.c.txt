// input_sensitivity_to_turn_rate  (Ghidra: FUN_0048c8e0; renamed per its behavior)
// address 0x48c8e0, size 71 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: objdump confirms the sole parameter is a normal cdecl stack float ([esp+4]); the
//   raw bytes at the three referenced constants are exactly 0.0010000000474974513f
//   (0x00672bf8), 100.0f (0x00672bc4), and 0.006283185910433531f (0x00672c14, float32(2*pi/1000)).
// register convention: value on the stack (cdecl float), no registers.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

// Converts a raw sensitivity value (clamped to 0.001 .. 100.0) into a radians-per-unit turn-rate
// scale factor by multiplying it by 2*pi/1000.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
float input_sensitivity_to_turn_rate(float sensitivity)
{
    if (sensitivity < 0.001f) {
        return 0.001f * 0.006283186f;
    }
    if (100.0f < sensitivity) {
        return 100.0f * 0.006283186f;
    }
    return sensitivity * 0.006283186f;
}

#if 0
Original Ghidra decompilation (0x48c8e0), from tools/pack.py 0x48c8e0:

float10 FUN_0048c8e0(float param_1)

{
  if (param_1 < 0.001) {
    return (float10)0.001 * (float10)0.006283186;
  }
  if (100.0 < param_1) {
    return (float10)100.0 * (float10)0.006283186;
  }
  return (float10)param_1 * (float10)0.006283186;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
