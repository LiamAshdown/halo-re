// ai_reference_clear_search_target  (Ghidra: ai_reference_clear_search_target; named for this rewrite)
// address 0x434c80, size 57 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: sets actor.unknown_1d4 (types/ai.h) to 0 for every actor a packed ai reference
// names; matches the phase-4 summary ("search-target type, enum value 0 = none").
//   // blam-cc: EAX -> packed_reference

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator); // 0x432650, this batch
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0, this batch

void ai_reference_clear_search_target(uint32_t packed_reference)
{
    ai_reference_actor_iterator iterator;
    actor *a;

    ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        a->unknown_1d4 = 0;
        a = ai_reference_actor_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x434c80):

void FUN_00434c80(void)

{
  int iVar1;

  FUN_00432650();
  iVar1 = FUN_004326d0();
  while (iVar1 != 0) {
    *(undefined2 *)(iVar1 + 0x1d4) = 0;
    iVar1 = FUN_004326d0();
  }
  return;
}
#endif
