// effect_random_int_between  (Ghidra: FUN_0044c800; named per out/phase2/results/effects_00.json,
// confidence 0.35)
// address 0x44c800, size 49 bytes
// name confidence: 0.35   rewrite confidence: 0.7
// evidence: out/phase2/results/effects_00.json 0x44c800 -- "Advances the local LCG state
// DAT_00719cd4 then scales (param_1 - in_ECX) by the 16-bit fraction and adds in_ECX back, the
// classic 'random integer in [min,max)' pattern reused throughout this file." Matches the LCG
// constants and shift documented for random_seed in types/math.h (k_random_multiplier,
// k_random_increment, k_random_value_shift) exactly, applied to this module's own seed at
// 0x00719cd4 (aliased effect_random_seed here; the same address is also called
// effect_random_seed by types/math.h and widget_random_seed by src/objects/*.c -- one shared
// non-deterministic stream, named per module).
// register convention: minimum bound in ECX (in_ECX), maximum bound on the stack (param_1).
//   // blam-cc: ECX -> minimum, stack -> maximum

#include "tags.h"
#include "math.h"

extern random_seed effect_random_seed; // 0x00719cd4

// Returns a pseudo-random integer in [minimum, maximum).
int effect_random_int_between(int16_t minimum, int16_t maximum)
{
    effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
    return (int)(((int)maximum - (int)minimum) * (int)(effect_random_seed >> k_random_value_shift) >> 16) +
           minimum;
}

#if 0
Original Ghidra decompilation (0x44c800):

int FUN_0044c800(short param_1)

{
  int in_ECX;
  
  DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  return (((int)param_1 - (int)(short)in_ECX) * (DAT_00719cd4 >> 0x10) >> 0x10) + in_ECX;
}
#endif
