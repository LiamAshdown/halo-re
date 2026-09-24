// random_range_real  (Ghidra: random_range_real, already named)
// address 0x444af0, size 55 bytes
// name confidence: 0.70   rewrite confidence: 0.85
// evidence: symbols/functions.txt / symbols/prototypes.txt (`float __cdecl
//   random_range_real(float minimum, float maximum);`); out/phase4/cache_types_notes.md
//   "random_range_real @0x444af0 belongs to the math/random module. It is the global LCG
//   (seed = seed * 0x19660d + 0x3c6ef35f) on 0x00719cd4"; types/math.h already documents
//   0x00719cd4 as `local_random_seed`, the "non-deterministic stream" companion to
//   `random_seed_global` (0x00719cd0, used by the differently-named sibling
//   src/math/random_real_range.c @0x401050). This is a distinct, real, independently callable
//   function -- not a duplicate compiled instance of 0x401050 -- because it drives the other
//   seed stream.
// register convention: __cdecl, both arguments on the stack (minimum, maximum); no
//   register-passed arguments. Confirmed by `python tools/pack.py 0x444af0`
//   (cc=__cdecl, 4 callers, signature float random_range_real(float minimum, float maximum)).
//   // blam-cc: stack -> (minimum, maximum)

#include "tags.h"
#include "math.h"

extern random_seed local_random_seed; // 0x00719cd4, the non-deterministic LCG stream (types/math.h)

// Returns a pseudo-random float linearly interpolated between minimum and maximum using the
// engine's non-deterministic (local_random_seed) LCG stream. Same formula as
// random_real_range @0x401050, which instead drives the deterministic random_seed_global stream.
real random_range_real(real minimum, real maximum)
{
    local_random_seed = local_random_seed * k_random_multiplier + k_random_increment;
    return (maximum - minimum) * (real)(local_random_seed >> k_random_value_shift) * 1.5259022e-05f + minimum;
}

#if 0
Original Ghidra decompilation (0x444af0):

float __cdecl random_range_real(float minimum,float maximum)

{
  DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  return (maximum - minimum) * (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 + minimum;
}
#endif
