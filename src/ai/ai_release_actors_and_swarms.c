// ai_release_actors_and_swarms  (Ghidra: ai_release_actors_and_swarms, already named)
// address 0x428ea0, size 152 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: types/ai.h ai_globals.stagger_threshold(0x04)/stagger_highest(0x06)/
//   stagger_claimed(0x03); actor.awareness_level(0x6a); offset 0xb falls inside
//   actor.unknown_0a / actor.swarm_pending (0x0a-0x0b). Calls actor_delete_or_release_unit (0x4288e0) and
//   actor_update_activation_state (0x429160), both already rewritten in this module, and
//   actor_iterator_next, whose dropped iterator-state argument is recovered in
//   src/ai/ai_mark_recognized_objects_for_reaction.c (reused here).
//   UNSURE: is_dead's derivation mirrors ai_release_inactive_swarms's identical pattern; not
//   independently re-verified with objdump for this call site.
// register convention: plain __cdecl, no parameters.
//   // blam-cc: (no arguments)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"
#include <stdint.h>

extern ai_globals *ai_globals_ptr; // 0x00880354
extern data_array *actor_data;     // 0x00880360
extern data_array *encounter_data; // 0x008802c8


// Iterates every active actor as part of AI global cleanup, tearing down/resetting swarm
// actors and deleting the remainder.
void ai_release_actors_and_swarms(void)
{
    actor_iterator_state iterator;
    actor *a;
    uint8_t is_dead = 0;

    ai_globals_ptr->stagger_threshold = ai_globals_ptr->stagger_highest;
    ai_globals_ptr->stagger_highest = 0;
    ai_globals_ptr->stagger_claimed = 0;

    // 0x428f0a: the release call always gets AL = 0

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

        if (a->swarm_pending == 0) {
            if (a->awareness_level > 0) {
                actor_update_activation_state(actor_index);
            }
        } else {
            actor_delete_or_release_unit(actor_index, is_dead);
        }
        a = actor_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x428ea0):

void ai_release_actors_and_swarms(void)

{
  int iVar1;
  undefined4 local_8;

  iVar1 = DAT_00880354;
  *(undefined2 *)(DAT_00880354 + 4) = *(undefined2 *)(DAT_00880354 + 6);
  *(undefined2 *)(iVar1 + 6) = 0;
  *(undefined1 *)(iVar1 + 3) = 0;
  if (*(char *)(iVar1 + 1) != '\0') {
    local_8 = 0xffffffff;
  }
  iVar1 = actor_iterator_next();
  while (iVar1 != 0) {
    if (*(char *)(iVar1 + 0xb) == '\0') {
      if (0 < *(short *)(iVar1 + 0x6a)) {
        actor_update_activation_state();
      }
    }
    else {
      actor_delete_or_release_unit(local_8);
    }
    iVar1 = actor_iterator_next();
  }
  return;
}
#endif
