// ai_reference_invoke_squad_callback_406f80  (Ghidra: ai_reference_invoke_squad_callback_406f80; named for this rewrite)
// address 0x434e60, size 102 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: for every actor a packed ai reference names, calls
// actor_squad_for_each_member (actor_swarm_for_each_component, outside this rewrite's range) with a fixed
// callback (LAB_00406f80) and no reset. Matches the phase-4 summary ("registers a fixed
// callback against every squad member").
//   // blam-cc: EAX -> packed_reference

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator); // 0x432650, this batch
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0, this batch
extern void actor_swarm_for_each_component(datum_index actor_index, int32_t reset, void *callback, int32_t callback_arg); // 0x407040, outside this rewrite's range, UNSURE signature
extern void LAB_00406f80(void); // 0x406f80, outside this rewrite's range, UNSURE signature

void ai_reference_invoke_squad_callback_406f80(uint32_t packed_reference)
{
    ai_reference_actor_iterator iterator;
    actor *a;

    ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        actor_swarm_for_each_component(iterator.actor_index, 0, (void *)LAB_00406f80, 0);
        a = ai_reference_actor_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x434e60):

void FUN_00434e60(void)

{
  int iVar1;
  undefined4 local_8;

  FUN_00432650();
  iVar1 = FUN_004326d0();
  while (iVar1 != 0) {
    FUN_00407040(local_8,0,&LAB_00406f80,0);
    iVar1 = FUN_004326d0();
  }
  return;
}
#endif
