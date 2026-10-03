// effect_random_uint16  (Ghidra: FUN_0044da40; named per out/phase2/results/effects_00.json,
// confidence 0.45)
// address 0x44da40, size 25 bytes
// name confidence: 0.45   rewrite confidence: 0.85
// evidence: advances the same LCG state as effect_random_int_between 0x44c800 and
// effect_random_scaled_range 0x44c840 (0x00719cd4) and returns the raw high 16 bits, with no
// min/max shaping -- the module's plain "next random word" primitive.
// register convention: none; no parameters.

#include "tags.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern random_seed effect_random_seed; // 0x00719cd4

uint32_t effect_random_uint16(void)
{
    effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
    return effect_random_seed >> k_random_value_shift;
}

#if 0
Original Ghidra decompilation (0x44da40):

uint FUN_0044da40(void)

{
  DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  return DAT_00719cd4 >> 0x10;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
