// ai_reference_set_search_target_point  (Ghidra: ai_reference_set_search_target_point; named for this rewrite)
// address 0x434cc0, size 63 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: sets actor.unknown_1d4 to 1 ("point") and stores a caller-supplied reference
// value at actor.unknown_1d6 (the first dword of that 6-byte unknown block) for every actor
// a packed ai reference names, matching the phase-4 summary.
// register convention: Ghidra could not resolve the reference value at all (unaff_ESI);
// kept as an ordinary parameter.
//   // blam-cc: EAX -> packed_reference, ESI -> reference_value

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator); // 0x432650, this batch
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0, this batch

// blam-cc: EAX -> packed_reference, ESI -> reference_value
void ai_reference_set_search_target_point(uint32_t packed_reference, uint32_t reference_value)
{
    ai_reference_actor_iterator iterator;
    actor *a;

    ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        a->try_to_fight_type = 1;
        *(uint32_t *)(a->unknown_1d6 + 2) = reference_value;
        a = ai_reference_actor_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x434cc0):

void FUN_00434cc0(void)

{
  int iVar1;
  undefined4 unaff_ESI;

  FUN_00432650();
  iVar1 = FUN_004326d0();
  while (iVar1 != 0) {
    *(undefined2 *)(iVar1 + 0x1d4) = 1;
    *(undefined4 *)(iVar1 + 0x1d8) = unaff_ESI;
    iVar1 = FUN_004326d0();
  }
  return;
}
#endif
