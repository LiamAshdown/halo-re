// ai_reference_set_combat_alert_flag  (Ghidra: ai_reference_set_combat_alert_flag; named for this rewrite)
// address 0x435af0, size 64 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: for every actor a packed ai reference names, calls actor_set_combat_alert_flag
// (0x421a40, already established elsewhere) with a caller-supplied flag this function
// itself never touches (a third inherited register, matching actor_set_combat_alert_flag's
// own EBX -> new_flag convention).
//   // blam-cc: EAX -> packed_reference, EBX -> new_flag

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator); // 0x432650, this batch
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0, this batch
extern void actor_set_combat_alert_flag(datum_index actor_index, uint8_t new_flag); // 0x421a40, already established

void ai_reference_set_combat_alert_flag(uint32_t packed_reference, uint8_t new_flag)
{
    if (packed_reference != (uint32_t)k_datum_index_none) {
        ai_reference_actor_iterator iterator;
        actor *a;

        ai_reference_actor_iterator_new(packed_reference, &iterator);
        a = ai_reference_actor_iterator_next(&iterator);
        while (a != 0) {
            actor_set_combat_alert_flag(iterator.actor_index, new_flag);
            a = ai_reference_actor_iterator_next(&iterator);
        }
    }
}

#if 0
Original Ghidra decompilation (0x435af0):

void FUN_00435af0(void)

{
  int in_EAX;
  int iVar1;

  if (in_EAX != -1) {
    FUN_00432650();
    iVar1 = FUN_004326d0();
    while (iVar1 != 0) {
      actor_set_combat_alert_flag();
      iVar1 = FUN_004326d0();
    }
  }
  return;
}
#endif
