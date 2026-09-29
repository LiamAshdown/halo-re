// ai_reference_set_search_target_area  (Ghidra: ai_reference_set_search_target_area; named for this rewrite)
// address 0x434d00, size 57 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: sets actor.unknown_1d4 to 2 ("area") for every actor a packed ai reference
// names, matching the phase-4 summary; no associated payload, unlike
// ai_reference_set_search_target_point (0x434cc0, this batch).
//   // blam-cc: EAX -> packed_reference

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator); // 0x432650, this batch
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0, this batch

void ai_reference_set_search_target_area(uint32_t packed_reference)
{
    ai_reference_actor_iterator iterator;
    actor *a;

    ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        a->try_to_fight_type = 2;
        a = ai_reference_actor_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x434d00):

void FUN_00434d00(void)

{
  int iVar1;

  FUN_00432650();
  iVar1 = FUN_004326d0();
  while (iVar1 != 0) {
    *(undefined2 *)(iVar1 + 0x1d4) = 2;
    iVar1 = FUN_004326d0();
  }
  return;
}
#endif
