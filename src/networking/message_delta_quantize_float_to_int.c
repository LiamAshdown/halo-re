// message_delta_quantize_float_to_int  (Ghidra: message_delta_quantize_float_to_int, already named)
// address 0x4ea480, size 80 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md ("Quantizes a float value within [param_2,param_3]
// into a clamped integer index with unaff_ESI representing the maximum level.").
// register convention: ESI -> max_level (unaff_ESI in the decompile); stack -> value, minimum,
// maximum.
//   // blam-cc: ESI -> max_level, stack -> value, minimum, maximum
// UNSURE: FUN_00623e40's exact effect (a CRT helper taking the scaled double on the FPU stack
// before __ftol truncates it); modeled as a plain pass-through round-to-nearest, matching the
// "+ 0.5 then truncate" shape of the surrounding arithmetic. The final
// `-(uint)(uVar2 != 0) & uVar2` idiom is semantically a no-op (it always evaluates to uVar2) and
// is simplified accordingly rather than transcribed bit for bit.

#include "tags.h"
#include "memory.h"
#include <math.h>

extern double FUN_00623e40(double value); // 0x623e40, CRT helper; UNSURE: exact effect

// Maps value from [minimum, maximum] onto an integer index in [0, max_level], rounding to the
// nearest level and clamping the result to max_level.
uint32_t message_delta_quantize_float_to_int(uint32_t max_level, real value, real minimum,
    real maximum)
    // blam-cc: ESI -> max_level, stack -> value, minimum, maximum
{
    real level_count_as_float;
    double scaled;
    uint32_t result;

    level_count_as_float = (real)(int32_t)max_level;
    if ((int32_t)max_level < 0) {
        level_count_as_float = level_count_as_float + 4.2949673e+09f;
    }
    scaled = FUN_00623e40((double)(level_count_as_float * ((value - minimum) / (maximum - minimum)) + 0.5));
    result = (uint32_t)(int32_t)scaled;
    if (max_level < result) {
        result = max_level;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4ea480), from tools/pack.py 0x4ea480:

uint message_delta_quantize_float_to_int(float param_1,float param_2,float param_3)

{
  float fVar1;
  uint uVar2;
  uint unaff_ESI;

  fVar1 = (float)(int)unaff_ESI;
  if ((int)unaff_ESI < 0) {
    fVar1 = fVar1 + 4.2949673e+09;
  }
  FUN_00623e40((double)(fVar1 * ((param_1 - param_2) / (param_3 - param_2)) + 0.5));
  uVar2 = __ftol();
  if (unaff_ESI < uVar2) {
    uVar2 = unaff_ESI;
  }
  return -(uint)(uVar2 != 0) & uVar2;
}
#endif
