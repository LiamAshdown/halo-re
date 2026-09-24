// ai_reference_actor_iterator_next  (Ghidra: ai_reference_actor_iterator_next; named for this rewrite)
// address 0x4326d0, size 97 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: types/ai.h already documents this as the partner of 0x432650 ("next; EDX ->
// iterator"). Chains actor.next_in_encounter (+0x2c) exactly like the global actor iterator
// (actor_iterator_next @0x436a70, this batch) and the encounter member list itself
// (encounter.first_actor, squad_remove_actor/encounter_add_actor's next_in_encounter chain),
// and filters by actor.squad_index (+0x3a) / actor.platoon_index (+0x3c) against the
// iterator's squad_filter/platoon_filter fields (+0x04/+0x08, see
// ai_reference_actor_iterator_new).
// register convention: matches the existing header comment: EDX -> iterator.
//   // blam-cc: EDX -> iterator

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

extern data_array *actor_data;      // 0x00880360
extern ai_globals *ai_global_data;  // 0x00880354

// blam-cc: EDX -> iterator
// Advances the iterator to the next actor in its chain (the encounter's member list, or the
// global unassigned-actor list when the iterator has no encounter) whose squad_index and
// platoon_index match the iterator's filters (a filter of -1 accepts any value), and
// returns a pointer to that actor, or 0 once the chain or the AI globals are exhausted.
actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator)
{
    int32_t *squad_filter = (int32_t *)(iterator->unknown_00 + 0x04);
    int32_t *platoon_filter = (int32_t *)(iterator->unknown_00 + 0x08);
    actor *base = (actor *)actor_data->data;

    for (;;) {
        datum_index next;
        actor *candidate;

        if (ai_global_data->actors_valid == 0) {
            return 0;
        }

        next = *(datum_index *)iterator->unknown_14;
        iterator->actor_index = next;
        if (next == (datum_index)k_datum_index_none) {
            return 0;
        }

        candidate = &base[next & 0xffff];
        *(datum_index *)iterator->unknown_14 = candidate->next_in_encounter;

        if (*squad_filter == -1 || *squad_filter == candidate->squad_index) {
            if (*platoon_filter == -1 || *platoon_filter == candidate->platoon_index) {
                return candidate;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4326d0):

int FUN_004326d0(void)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int in_EDX;

  iVar3 = DAT_00880360;
  iVar2 = DAT_00880354;
  do {
    do {
      if (*(char *)(iVar2 + 1) == '\0') {
        return 0;
      }
      uVar1 = *(uint *)(in_EDX + 0x14);
      *(uint *)(in_EDX + 0x10) = uVar1;
      if (uVar1 == 0xffffffff) {
        return 0;
      }
      iVar4 = (uVar1 & 0xffff) * 0x724 + *(int *)(iVar3 + 0x34);
      *(undefined4 *)(in_EDX + 0x14) = *(undefined4 *)(iVar4 + 0x2c);
    } while ((*(int *)(in_EDX + 4) != -1) && (*(int *)(in_EDX + 4) != (int)*(short *)(iVar4 + 0x3a))
            );
    if (*(int *)(in_EDX + 8) == -1) {
      return iVar4;
    }
  } while (*(int *)(in_EDX + 8) != (int)*(short *)(iVar4 + 0x3c));
  return iVar4;
}
#endif
