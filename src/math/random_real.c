// random_real  (Ghidra: random_real, already named)
// address 0x4019f0, size 41 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: out/phase4/math_functions.md; same LCG as random_real_range @0x401050.
// register convention: __cdecl, no arguments.

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern random_seed random_seed_global; // 0x00719cd0

// Returns a pseudo-random float in the range [0,1) using the engine's global LCG seed.
real random_real(void)
{
    random_seed_global = random_seed_global * k_random_multiplier + k_random_increment;
    return (real)(random_seed_global >> k_random_value_shift) * 1.5259022e-05f;
}

#if 0
Original Ghidra decompilation (0x4019f0):

float __cdecl random_real(void)

{
  DAT_00719cd0 = DAT_00719cd0 * 0x19660d + 0x3c6ef35f;
  return (float)(DAT_00719cd0 >> 0x10) * 1.5259022e-05;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
