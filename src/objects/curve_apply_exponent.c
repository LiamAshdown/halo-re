// curve_apply_exponent
// address 0x4fea50, size 33 bytes, zero recorded callers
// name confidence: 0.35 (still FUN_004fea50 in Ghidra; functions.md: "Returns a value unchanged
//   when its weight is exactly 1.0, otherwise evaluates it through a secondary
//   (interpolation/curve) helper")
// rewrite confidence: 0.7
// evidence: src/math/periodic_function_build_transition_table.c identifies 0x6283c0 as the
//   MSVC 7.1 CRT _CIpow (base in ST(1), exponent in ST(0)).
// register convention: two float stack parameters (Ghidra shows them cleanly).
// blam-cc: stack -> value, exponent

#include "tags.h"
#include "memory.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern double pow(double base, double exponent); // 0x6283c0, MSVC 7.1 CRT _CIpow

float curve_apply_exponent(float value, float exponent) // blam-cc: stack -> value, exponent
{
    if (exponent != 1.0f) {
        return (float)pow((double)value, (double)exponent);
    }
    return value;
}

#if 0
Original Ghidra decompilation (0x4fea50):

float10 FUN_004fea50(float param_1,float param_2)

{
  float10 fVar1;

  if (param_2 != 1.0) {
    fVar1 = (float10)FUN_006283c0();
    return fVar1;
  }
  return (float10)param_1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
