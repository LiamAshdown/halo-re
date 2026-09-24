// actor_dispatch_look_handler_by_posture  (Ghidra: actor_dispatch_look_handler_by_posture; named from out/phase2/results/ai_02.json)
// address 0x41bb30, size 133 bytes
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: out/phase2/results/ai_02.json -- early-outs unless unaff_BX is 0 or 1, calls
//   actor_get_actor_definition, then jumps through a function-pointer table (PTR_LAB_0041be00)
//   indexed by a stack parameter.
// register convention: EAX carries the actor_index through to actor_get_actor_definition (not
//   captured by Ghidra as unaff_EAX, since it is the recognized register that function already
//   expects live); EBX (unaff_BX) is a small gate code; the jump-table index is a stack
//   parameter Ghidra placed at frame offset 0x18.
//   // blam-cc: EAX -> actor_index, EBX -> posture, stack -> handler_index
//
// UNSURE: the jump table at 0x0041be00 and its handler procedures sit in the 0x1a0-byte gap
// between this function and actor_target_get_priority_class (0x41be10); Ghidra did not create
// function boundaries there (the table's targets are only reached indirectly, the same
// situation types/ai.h documents for the firing-position rule tables), so the table's element
// count, the handler signature and what actor_get_actor_definition's result feeds into the
// handler call are all unrecovered. The call is left as a bare indirect call through the raw
// table pointer, matching what Ghidra itself could resolve.

// KNOWN DEFECT, flagged by the module review and NOT fixed here: the arity below is wrong.
// 0x41bb30 opens with sub esp,0x24 / push ebp / push esi / push edi and then reads its
// first stack argument at [esp+0x34], i.e. entry_esp+4, after testing BX; every call site
// pushes six dwords and cleans up with add esp,0x18 (0x41f0e3, 0x41f2d0, 0x41ce5e,
// 0x41d10e, 0x41d230). So the real shape is EBX plus SIX stack arguments, which is what
// src/ai/actor_danger_update_reaction.c and src/ai/actor_target_update_tracking_speed.c
// both declare, and the three-parameter form below cannot be right. The body needs a full
// re-derivation from the disassembly; until then treat this file as unverified and the
// name actor_dispatch_look_handler_by_posture as unconfirmed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

extern void * actor_get_actor_definition(datum_index actor_index); // 0x40fa70

// UNSURE signature and element count: see note above.
extern uint32_t (*actor_look_handler_table[])(void); // 0x0041be00 PTR_LAB_0041be00

// blam-cc: EAX -> actor_index, EBX -> posture, stack -> handler_index
// Dispatches to one of several per-posture look handler functions via a jump table, gated on
// the actor's current stance (posture must be 0 or 1).
uint32_t actor_dispatch_look_handler_by_posture(datum_index actor_index, int16_t posture, int16_t handler_index)
{
    if (posture != 0 && posture != 1) {
        return 0;
    }
    // UNSURE: the return value is discarded by the original; the actor definition pointer is
    // presumably left live in EAX for the indirect call below, which Ghidra could not resolve
    // through the jump table.
    actor_get_actor_definition(actor_index);
    return actor_look_handler_table[handler_index]();
}

#if 0
Original Ghidra decompilation (0x41bb30):

undefined4 FUN_0041bb30(void)

{
  undefined4 uVar1;
  short unaff_BX;
  short in_stack_00000018;

  if ((unaff_BX != 0) && (unaff_BX != 1)) {
    return 0;
  }
  actor_get_actor_definition();
                    /* WARNING: Could not recover jumptable at 0x0041bba4. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  uVar1 = (*(code *)(&PTR_LAB_0041be00)[in_stack_00000018])();
  return uVar1;
}
#endif
