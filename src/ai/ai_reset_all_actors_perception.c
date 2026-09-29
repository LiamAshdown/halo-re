// ai_reset_all_actors_perception  (Ghidra: ai_reset_all_actors_perception, already named)
// address 0x429080, size 105 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: calls actor_dispatch_perception_reset (0x429000, already rewritten in this
//   module) and actor_iterator_next, whose dropped iterator-state argument is recovered in
//   src/ai/ai_mark_recognized_objects_for_reaction.c (reused here).
// register convention: plain __cdecl, no parameters.
//   // blam-cc: (no arguments)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"
#include <stdint.h>

extern data_array *actor_data;     // 0x00880360
extern data_array *encounter_data; // 0x008802c8


// Iterates all active actors and re-runs their perception-reset dispatcher.
void ai_reset_all_actors_perception(void)
{
    actor_iterator_state iterator;
    actor *a;

    iterator.filter_array = encounter_data;
    iterator.unknown_04 = 0;
    iterator.cursor = -1;
    iterator.signature = (uint32_t)(uintptr_t)encounter_data ^ 0x69746572;
    iterator.unknown_10 = 0;
    iterator.active = 1;
    iterator.actor_index = -1;
    iterator.unknown_18 = -1;

    a = actor_iterator_next(&iterator);
    while (a != 0) {
        datum_index actor_index = iterator.actor_index /* the full handle, salt included */;
        actor_dispatch_perception_reset(actor_index);
        a = actor_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x429080):

void ai_reset_all_actors_perception(void)

{
  int iVar1;

  iVar1 = actor_iterator_next();
  while (iVar1 != 0) {
    FUN_00429000();
    iVar1 = actor_iterator_next();
  }
  return;
}
#endif
