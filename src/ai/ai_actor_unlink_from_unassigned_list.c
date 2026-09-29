// ai_actor_unlink_from_unassigned_list  (Ghidra: ai_actor_unlink_from_unassigned_list; named for this rewrite)
// address 0x436990, size 92 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: removes an actor from the global unassigned-actor list (the counterpart of
// ai_actor_link_to_unassigned_list, 0x436940, this batch): walks ai_globals.unknown_08's
// chain (actor.next_in_encounter) until it finds the link pointing at this actor, splices it
// out, then clears actor.unknown_09/unknown_0a/next_in_encounter.
// register convention: Ghidra could not resolve the parameter at all.
//   // blam-cc: EDI -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern ai_globals *ai_globals_ptr; // 0x00880354
extern data_array *actor_data;     // 0x00880360

void ai_actor_unlink_from_unassigned_list(datum_index actor_index)
{
    if (ai_globals_ptr->actors_valid != 0) {
        actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
        datum_index *link = &ai_globals_ptr->first_encounterless_actor;

        while (*link != actor_index) {
            actor *node = &((actor *)actor_data->data)[*link & 0xffff];
            link = &node->next_in_encounter;
        }

        *link = a->next_in_encounter;
        a->unknown_09 = 0;
        a->next_in_encounter = (datum_index)k_datum_index_none;
        a->unknown_0a = 0;
    }
}

#if 0
Original Ghidra decompilation (0x436990):

void FUN_00436990(void)

{
  uint *puVar1;
  uint uVar2;
  int iVar3;
  uint unaff_EDI;

  if (*(char *)(DAT_00880354 + 1) != '\0') {
    puVar1 = (uint *)(DAT_00880354 + 8);
    iVar3 = (unaff_EDI & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
    uVar2 = *puVar1;
    while (uVar2 != unaff_EDI) {
      puVar1 = (uint *)((uVar2 & 0xffff) * 0x724 + 0x2c + *(int *)(DAT_00880360 + 0x34));
      uVar2 = *puVar1;
    }
    *puVar1 = *(uint *)(iVar3 + 0x2c);
    *(undefined1 *)(iVar3 + 9) = 0;
    *(undefined4 *)(iVar3 + 0x2c) = 0xffffffff;
    *(undefined1 *)(iVar3 + 10) = 0;
  }
  return;
}
#endif
