// actor_reset_squad_link_for_type_change  (Ghidra: actor_reset_squad_link_for_type_change, renamed)
// address 0x4290f0, size 98 bytes
// name confidence: 0.3   rewrite confidence: 0.9 (VERIFIED against objdump)
// evidence: types/ai.h actor.unknown_09/encounter_index(0x34). Calls
//   actor_movement_action_cancel (0x428650, already rewritten in this module) and
//   squad_remove_actor/encounter_add_actor/ai_actor_link_to_unassigned_list/ai_actor_unlink_from_unassigned_list, none established elsewhere in
//   this repo (all outside this rewrite's range).
//   UNSURE: this is one of the least-confident rewrites in this pass; encounter_index (EBX)
//   and the exact arguments each unestablished callee needs were not independently
//   re-verified with objdump.
// register convention: EAX -> actor_index, EBX -> encounter_index.
//   // blam-cc: EAX -> actor_index, EBX -> encounter_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *actor_data; // 0x00880360


    // UNSURE: the squad index arrives in DX and Ghidra did not attribute it to this call
    // site, so the actor's current squad_index is passed; encounter_add_actor writes it
    // straight back into the same field.

// blam-cc: EAX -> actor_index, EBX -> encounter_index
// FIXED (verified against 0x4290f0..0x429151): the squad is a stack argument (it becomes encounter_add_actor's DX)
//   and encounter_add_actor keeps the actor's team (push 1).
void actor_reset_squad_link_for_type_change(datum_index actor_index, datum_index encounter_index, int16_t squad_index)
    // blam-cc: EAX -> actor_index, EBX -> encounter_index, stack -> squad_index
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    actor_movement_action_cancel(actor_index);

    if (self->unknown_09 == 0) {
        if (self->encounter_index != (datum_index)k_datum_index_none) {
            encounter_remove_actor(actor_index, 0);
        }
    } else {
        ai_actor_unlink_from_unassigned_list(actor_index);
    }

    if (encounter_index == (datum_index)k_datum_index_none) {
        ai_actor_link_to_unassigned_list(actor_index);
        return;
    }
    encounter_add_actor(squad_index, actor_index, encounter_index, 1); // 0x42913f..0x429147
}

#if 0
Original Ghidra decompilation (0x4290f0):

void FUN_004290f0(void)

{
  uint in_EAX;
  int unaff_EBX;
  int iVar1;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  FUN_00428650();
  if (*(char *)(iVar1 + 9) == '\0') {
    if (*(int *)(iVar1 + 0x34) != -1) {
      squad_remove_actor(0);
    }
  }
  else {
    FUN_00436990();
  }
  if (unaff_EBX == -1) {
    FUN_00436940();
    return;
  }
  FUN_00436770();
  return;
}
#endif
