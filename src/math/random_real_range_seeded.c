// random_real_range_seeded  (Ghidra: random_real_range_seeded, already named)
// address 0x4cd170, size 57 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: out/phase4/math_types_notes.md random_seed section; identical LCG step to
//   random_real_range @0x401050 but reads/writes a caller-supplied seed pointer instead of the
//   global random_seed_global.
// register convention: seed pointer in ECX (in_ECX); min/max as the recognized stack
//   parameters (param_1, param_2).
//   // blam-cc: ECX -> seed, stack -> (min, max)

#include "tags.h"
#include "math.h"
#include "fn_math.h"

// Returns a pseudo-random float between min and max, advancing the caller-supplied RNG state
// (*seed) instead of the engine's global seed.
real random_real_range_seeded(random_seed *seed, real min, real max)
{
    *seed = *seed * k_random_multiplier + k_random_increment;
    return (max - min) * (real)(*seed >> k_random_value_shift) * 1.5259022e-05f + min;
}

#if 0
Original Ghidra decompilation (0x4cd170):

/* WARNING: Removing unreachable block (ram,0x004cd18d) */

float10 random_real_range_seeded(float param_1,float param_2)

{
  uint uVar1;
  uint *in_ECX;

  uVar1 = *in_ECX * 0x19660d + 0x3c6ef35f;
  *in_ECX = uVar1;
  return ((float10)param_2 - (float10)param_1) * (float10)(uVar1 >> 0x10) * (float10)1.5259022e-05 +
         (float10)param_1;
}
#endif
