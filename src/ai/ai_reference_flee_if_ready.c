// ai_reference_flee_if_ready  (Ghidra: ai_reference_flee_if_ready; named for this rewrite)
// address 0x434d90, size 91 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: for every actor a packed ai reference names, switches it into mode 0xb
// (types/ai.h _actor_mode_flee) via actor_set_mode (already established) if a readiness
// predicate (actor_squad_action_status_broadcast, outside this rewrite's range) is satisfied. Mirrors
// ai_unit_flee_if_ready (0x434df0, this batch), which calls the same predicate with an
// explicit second argument this call site does not visibly pass; guessed as 0 here.
// register convention: Ghidra fully resolved neither the packed reference (assumed EAX,
// per every sibling in this cluster) nor actor_squad_action_status_broadcast's second argument.
//   // blam-cc: EAX -> packed_reference

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator); // 0x432650, this batch
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0, this batch
extern char actor_squad_action_status_broadcast(datum_index actor_index, uint32_t param_2); // outside this rewrite's range, UNSURE signature
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data); // 0x40d8d0, this module

void ai_reference_flee_if_ready(uint32_t packed_reference)
{
    ai_reference_actor_iterator iterator;
    actor *a;

    ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        if (actor_squad_action_status_broadcast(iterator.actor_index, 0) != 0) { // UNSURE: second argument guessed, see file header
            uint8_t mode_data[0x84];
            actor_set_mode(iterator.actor_index, _actor_mode_flee, mode_data);
        }
        a = ai_reference_actor_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x434d90):

void FUN_00434d90(void)

{
  char cVar1;
  int iVar2;
  undefined4 local_8c;
  undefined1 local_84 [132];

  FUN_00432650();
  iVar2 = FUN_004326d0();
  while (iVar2 != 0) {
    cVar1 = FUN_00407140(local_8c);
    if (cVar1 != '\0') {
      actor_set_mode(local_8c,0xb,local_84);
    }
    iVar2 = FUN_004326d0();
  }
  return;
}
#endif
