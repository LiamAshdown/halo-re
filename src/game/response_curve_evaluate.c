// response_curve_evaluate  (Ghidra: FUN_00470fb0; renamed, no established name)
// address 0x470fb0, size 179 bytes
// name confidence: 0.3   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Evaluates a piecewise-linear response curve stored as
// a float table at a given fractional index, matching the input's sign").
// register convention: the float table pointer is Ghidra's `unaff_EDI`.
//   // blam-cc: EDI -> table, stack -> table_count, x
// UNSURE: partially reconstructed against objdump (--start-address=0x470fb0
// --stop-address=0x471063 bin/halo.exe) to recover `x`'s scaling (`|x| * (table_count - 1)`) and
// the two-stage clamp (against a magic double constant at 0x00672c08, then against
// table_count - 1), which Ghidra's decompile does not show explicitly (`extraout_ST0`). The exact
// order/branching of the two clamp comparisons in the real FPU code is not fully disentangled;
// what is preserved exactly is Ghidra's own final formula -- a standard table lerp between the
// floor and ceiling indices of the scaled position, sign-flipped when `x` is negative.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern double response_curve_scale_limit; // 0x00672c08, UNSURE exact meaning; a fixed clamp ceiling

extern double fabs(double x);

// blam-cc: EDI -> table, stack -> table_count, x
// Evaluates the `table_count`-entry piecewise-linear table at fractional position
// |x| * (table_count - 1) (clamped to at most table_count - 1 and to response_curve_scale_limit),
// linearly interpolating between the two nearest entries, and negates the result if x < 0.
real response_curve_evaluate(int16_t table_count, real x, real *table)
{
    int32_t max_index = table_count - 1;
    double scaled = fabs((double)x) * (double)max_index;
    int32_t lower, upper;
    real result;

    if (scaled >= response_curve_scale_limit) {
        scaled = response_curve_scale_limit;
    }
    if (scaled > (double)max_index) {
        scaled = (double)max_index;
    }

    lower = (int32_t)scaled;
    if (lower < 0) {
        lower = 0;
    } else if (lower > max_index) {
        lower = max_index;
    }
    upper = lower + 1;
    if (upper > max_index) {
        upper = max_index;
    }

    result = (real)(scaled - (double)lower) * (table[upper] - table[lower]) + table[lower];
    if (x < 0.0f) {
        result = -result;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x470fb0), from tools/pack.py 0x470fb0:

float10 FUN_00470fb0(short param_1,float param_2)

{
  int iVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  int unaff_EDI;
  float10 extraout_ST0;
  float10 fVar5;

  iVar1 = param_1 + -1;
  sVar2 = __ftol();
  if (sVar2 < 0) {
    sVar2 = 0;
  }
  else if (iVar1 < sVar2) {
    sVar2 = (short)iVar1;
  }
  iVar3 = (int)sVar2;
  iVar4 = iVar3 + 1;
  if (iVar1 < iVar3 + 1) {
    iVar4 = iVar1;
  }
  fVar5 = (extraout_ST0 - (float10)iVar3) *
          ((float10)*(float *)(unaff_EDI + (short)iVar4 * 4) -
          (float10)*(float *)(unaff_EDI + iVar3 * 4)) + (float10)*(float *)(unaff_EDI + iVar3 * 4);
  if (param_2 < 0.0) {
    fVar5 = -fVar5;
  }
  return fVar5;
}
#endif
