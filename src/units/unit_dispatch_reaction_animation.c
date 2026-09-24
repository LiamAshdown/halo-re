// unit_dispatch_reaction_animation  (Ghidra: unit_dispatch_reaction_animation)
// address 0x5614a0, size 41 bytes
// name confidence: 0.35 (phase2 candidate)   rewrite confidence: 0.15
// evidence: functions.md summary ("Dispatches to one of several per-unit reaction-animation
//   handler routines via a jump table indexed by a small reaction code").
// register convention: reaction code in the stack param.
//   // blam-cc: stack param_1 -> reaction_code
// UNSURE: Ghidra could not recover the jump table at 0x561604 ("Too many branches"), so neither
//   its entry count nor the handler addresses/signatures are known. The handlers are called
//   with zero visible arguments in the original, meaning they read whatever the caller left in
//   registers (most likely the unit index), which this rewrite cannot reconstruct without the
//   individual handler bodies. The table is declared here as an opaque array of no-argument
//   function pointers so the dispatch itself compiles; every entry's real signature is unknown.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

// unit_reaction_animation_handler is declared in types/units.h.
extern unit_reaction_animation_handler unit_reaction_animation_handlers[]; // 0x561604, PTR_LAB_00561604, UNSURE element count

void unit_dispatch_reaction_animation(int16_t reaction_code) // blam-cc: stack param_1 -> reaction_code
{
    unit_reaction_animation_handlers[reaction_code]();
}

#if 0
Original Ghidra decompilation (0x5614a0):

void FUN_005614a0(short param_1)

{
                    /* WARNING: Could not recover jumptable at 0x005614c2. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*(code *)(&PTR_LAB_00561604)[param_1])();
  return;
}
#endif
