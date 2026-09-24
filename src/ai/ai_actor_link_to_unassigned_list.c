// ai_actor_link_to_unassigned_list  (Ghidra: ai_actor_link_to_unassigned_list; named for this rewrite)
// address 0x436940, size 76 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: prepends an actor onto the global unassigned-actor list headed by
// ai_globals.unknown_08 (types/ai.h: "also the head of the unassigned actor list"), chained
// through actor.next_in_encounter (already established); sets actor.unknown_09 (marking it
// linked) and actor.unknown_10 from actor.active, then calls actor_movement_action_cancel (outside this
// rewrite's range). Matches the phase-4 summary.
// register convention: Ghidra could not resolve the parameter at all.
//   // blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern ai_globals *ai_global_data; // 0x00880354
extern data_array *actor_data;     // 0x00880360

extern void actor_movement_action_cancel(void); // 0x428650, outside this rewrite's range, UNSURE signature

void ai_actor_link_to_unassigned_list(datum_index actor_index)
{
    if (ai_global_data->actors_valid != 0) {
        actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];

        a->next_in_encounter = ai_global_data->unknown_08;
        ai_global_data->unknown_08 = actor_index;
        a->unknown_09 = 1;
        *(int16_t *)a->unknown_10 = (a->active != 0) ? 0x5a : 0;

        actor_movement_action_cancel();
    }
}

#if 0
Original Ghidra decompilation (0x436940):

void FUN_00436940(void)

{
  int iVar1;
  uint in_EAX;
  int iVar2;

  iVar1 = DAT_00880354;
  if (*(char *)(DAT_00880354 + 1) != '\0') {
    iVar2 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
    *(undefined4 *)(iVar2 + 0x2c) = *(undefined4 *)(DAT_00880354 + 8);
    *(uint *)(iVar1 + 8) = in_EAX;
    *(undefined1 *)(iVar2 + 9) = 1;
    *(ushort *)(iVar2 + 0x10) = -(ushort)(*(char *)(iVar2 + 8) != '\0') & 0x5a;
    FUN_00428650();
  }
  return;
}
#endif
