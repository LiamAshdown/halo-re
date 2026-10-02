// random_real_range  (Ghidra: random_real_range, already named)
// address 0x401050, size 55 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: out/phase4/math_functions.md; types/math.h random_seed section (LCG constants,
//   value = (seed >> 16) * a 1/65536-ish scale). The scale literal read from .rdata at
//   0x00672b84 (file offset 0x272b84) is the float32 bit pattern 0x37800080, which is
//   1.0f/65535.0f rounded to float32, not 1.0f/65536.0f -- kept as the literal from the
//   decompile since it is what the binary actually multiplies by.
// register convention: __cdecl, both arguments on the stack (min, max); no register-passed
//   arguments.

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern random_seed random_seed_global; // 0x00719cd0

// Returns a pseudo-random float linearly interpolated between min and max using the engine's
// global LCG seed.
real random_real_range(real min, real max)
{
    random_seed_global = random_seed_global * k_random_multiplier + k_random_increment;
    return (max - min) * (real)(random_seed_global >> k_random_value_shift) * 1.5259022e-05f + min;
}

#if 0
Original Ghidra decompilation (0x401050):

float __cdecl random_real_range(float min,float max)

{
  DAT_00719cd0 = DAT_00719cd0 * 0x19660d + 0x3c6ef35f;
  return (max - min) * (float)(DAT_00719cd0 >> 0x10) * 1.5259022e-05 + min;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
