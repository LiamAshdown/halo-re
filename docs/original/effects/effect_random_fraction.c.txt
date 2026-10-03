// effect_random_fraction  (Ghidra: effect_random_fraction, already named)
// address 0x4505b0, size 41 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: same LCG state and shift as effect_random_int_between 0x44c800 and
// effect_random_uint16 0x44da40 (0x00719cd4).
// register convention: no parameters.

#include "tags.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern random_seed effect_random_seed; // 0x00719cd4

// Returns the next pseudo-random fraction in [0, 1).
real effect_random_fraction(void)
{
    effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
    return (real)(effect_random_seed >> k_random_value_shift) * 1.5259022e-05f;
}

#if 0
Original Ghidra decompilation (0x4505b0):

float10 effect_random_fraction(void)

{
  DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  return (float10)(DAT_00719cd4 >> 0x10) * (float10)1.5259022e-05;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
