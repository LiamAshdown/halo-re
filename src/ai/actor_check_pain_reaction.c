// actor_check_pain_reaction  (Ghidra: actor_check_pain_reaction, renamed)
// address 0x40de20, size 75 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: phase-4 summary "checks a pain/reaction condition and transitions the actor
// into the associated mode if satisfied."
// register convention: no parameters visible in the decompiled C; actor_build_order_grenade_or_melee and
// actor_set_mode are both called with zero visible arguments, so the actor_index this
// function operates on is presumably a register the caller already set (as with the other
// zero-argument helpers in this module).
// UNSURE: actor_index register and actor_set_mode's mode/mode_data arguments are not
// visible in the decompiled C at all; needs the disassembly review pass.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern int32_t actor_build_order_grenade_or_melee(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_build_order_grenade_or_melee at 0x403630
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data); // 0x40d8d0, this module

// UNSURE: actor_index register not determined from the decompilation
uint8_t actor_check_pain_reaction(datum_index actor_index)
{
    if (actor_build_order_grenade_or_melee(actor_index) != 0) {
        actor_set_mode(actor_index, 0, (void *)0); // UNSURE: mode/mode_data not visible
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x40de20):

undefined4 FUN_0040de20(void)

{
  char cVar1;

  cVar1 = FUN_00403630();
  if (cVar1 != '\0') {
    actor_set_mode();
    return 1;
  }
  return 0;
}
#endif
