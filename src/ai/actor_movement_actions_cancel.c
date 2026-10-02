// actor_movement_actions_cancel  (Ghidra: actor_movement_action_cancel, already named --
//   see the NAME COLLISION note below for why this file uses a different, plural name)
// address 0x417a30, size 37 bytes
// name confidence: 0.55 (out/functions.json marks this name "IMPORTED" for this exact address,
//   i.e. it is Ghidra's own established name, not a phase-4 guess)   rewrite confidence: 0.85
// evidence: types/ai.h actor_movement_action.cancelled (0x02) cites "actor_movement_action_cancel
//   sets it"; actor.queued_movement (0x400) / actor.active_movement (0x46c), so cancelled sits
//   at 0x402 and 0x46e respectively -- exactly the two byte offsets this function writes.
// register convention: actor index in EAX (in_EAX, unresolved register read; Ghidra shows the
//   function taking no recognized parameters). Verified against objdump -d -M intel: the
//   function's first instructions use EAX directly with no preceding assignment, confirming it
//   is a genuine incoming register argument.
//   // blam-cc: EAX -> actor_index
//
// NAME COLLISION (found while writing this file, not fixed here -- out of this rewrite's
// scope): out/functions.json's name_source field marks 0x417a30 "IMPORTED" (Ghidra's own name)
// and 0x428650 "DEFAULT" (still FUN_00428650) for "actor_movement_action_cancel". An earlier
// session nonetheless wrote src/ai/actor_movement_action_cancel.c for 0x428650 (101 bytes, a
// completely different function -- it clears actor.firing_position_index and, in modes 3/4,
// actor.secondary_action/active_movement.extra, then invokes a per-mode-definition function
// pointer), reasoning from types/ai.h's un-addressed citation alone. That file is left
// untouched here since fixing it means re-auditing every one of its own callers (some already
// correctly cite 0x417a30 for this symbol, e.g. actor_consider_combat_mode.c and
// actor_squad_action_execute.c; others cite 0x428650 while calling the same symbol name, e.g.
// actor_reset_squad_link_for_type_change.c, ai_actor_link_to_unassigned_list.c,
// ai_object_list_detach_actors_from_encounters.c and ai_reference_units_exit_vehicles.c
// [sic, ai_reference_detach_actors_from_encounters.c] -- a cross-file cleanup beyond this
// rewrite's three target addresses). This file is named actor_movement_actions_cancel (plural,
// since it cancels both the queued and the active movement action) to avoid silently
// overwriting that unrelated, already-written function.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360

// blam-cc: EAX -> actor_index
// Marks both the actor's queued and its currently active movement action as cancelled.
void actor_movement_actions_cancel(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    self->queued_movement.cancelled = 1;
    self->active_movement.cancelled = 1;
}

#if 0
Original Ghidra decompilation (0x417a30):

void actor_movement_action_cancel(void)

{
  uint in_EAX;
  int iVar1;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  *(undefined1 *)(iVar1 + 0x402) = 1;
  *(undefined1 *)(iVar1 + 0x46e) = 1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
