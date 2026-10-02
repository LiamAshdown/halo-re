// angle_delta_wrapped  (Ghidra: angle_delta_wrapped, already named)
// address 0x470d10, size 47 bytes, cc=__cdecl
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/game_functions.md ("Computes the shortest signed angular difference
// between two angles, wrapped to the range (-pi, pi]").
// The second guard, Ghidra's `fVar1 < -pi != (fVar1 == -pi)`, is the FPU unordered-compare idiom:
// for ordered operands, `(a < b) != (a == b)` is true exactly when `a <= b` (see
// game_engine_tick.c for the sibling `(a < b) == (a == b)` -> `a > b` idiom this project already
// documents); applied here it reduces to `fVar1 <= -pi`, which is the natural symmetric partner
// of the first branch's `pi <= fVar1` and is what is written below.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

// Returns (to - from), wrapped into (-pi, pi].
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
float angle_delta_wrapped(float from, float to)
{
    float delta = to - from;

    if (3.1415927f <= delta) {
        delta = delta - 6.2831855f;
    }
    if (delta <= -3.1415927f) {
        delta = delta + 6.2831855f;
    }
    return delta;
}

#if 0
Original Ghidra decompilation (0x470d10), from tools/pack.py 0x470d10:

float __cdecl angle_delta_wrapped(float from,float to)

{
  float fVar1;

  fVar1 = to - from;
  if (3.1415927 <= fVar1) {
    fVar1 = fVar1 - 6.2831855;
  }
  if (fVar1 < -3.1415927 != (fVar1 == -3.1415927)) {
    fVar1 = fVar1 + 6.2831855;
  }
  return fVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
