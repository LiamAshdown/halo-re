// ai_release_actors_filtered  (Ghidra: ai_release_actors_filtered, renamed)
// address 0x42ab00, size 203 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (REWRITTEN from objdump 0x42ab00..0x42abca)
// evidence: types/ai.h actor.next_in_encounter(0x2c)/platoon_index(0x3c)/squad_index(0x3a).
//   Calls actor_delete_or_release_unit (0x4288e0, already rewritten in this module),
//   ai_reference_actor_iterator_init_cursor ("head of the unassigned actor list" per types/ai.h ai_globals+0x08,
//   already declared elsewhere in this module) and actor_iterator_next (dropped
//   iterator-state argument recovered in src/ai/ai_mark_recognized_objects_for_reaction.c).
//   Phase-4 summary: "Releases all active actors, or only those matching a given
//   encounter/squad filter, as part of AI cleanup."
//   UNSURE: this is one of the least-confident rewrites in this pass. encounter_index (EAX)
//   is compared against -1 to pick between "release everything" (via actor_iterator_next,
//   whose actor_iterator_new(0) setup call is not established here and is modeled as producing the
//   same iterator this module's other actor_iterator_next callers build) and "release only
//   actors in one platoon/squad" (via the unassigned-actor linked list, filtered by
//   unaff_EDI as platoon_index and param_1 as squad_index). is_dead (the flag passed to
//   actor_delete_or_release_unit) is assumed 0 in both paths, matching that this function's
//   own decompile never sets AL to anything else. None of this was independently confirmed
//   with objdump.
// register convention: EAX -> encounter_index, EDI -> platoon_index, stack -> squad_index.
//   // blam-cc: EAX -> encounter_index, EDI -> platoon_index, stack -> squad_index, BL -> is_dead

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern ai_globals *ai_globals_ptr; // 0x00880354
extern data_array *actor_data;     // 0x00880360
extern data_array *encounter_data; // 0x008802c8

extern actor *actor_iterator_next(actor_iterator_state *iterator); // 0x436a70
extern void actor_iterator_new(actor_iterator_state *out_iterator, uint8_t active_only); // 0x436a30, EAX, stack
extern void ai_reference_actor_iterator_init_cursor(int32_t encounter_index, datum_index *cursor); // 0x4369f0, EAX, ECX
extern void actor_delete_or_release_unit(datum_index actor_index, uint8_t is_dead); // 0x4288e0

// blam-cc: EAX -> encounter_index, EDI -> platoon_index, stack -> squad_index
// REWRITTEN from objdump. Encounter none walks every actor (actor_iterator_new(&iterator, 0), so inactive actors
//   too); otherwise it walks the encounter's member list (ai_reference_actor_iterator_init_cursor, cursor[2] is
//   the first actor, +0x2c the next) filtered by platoon (+0x3c vs EDI) and squad (+0x3a vs the stack argument),
//   -1 meaning any. Each match goes to actor_delete_or_release_unit(actor, BL). The draft built the iterator
//   with active = 1, called the cursor setup without its encounter/cursor, and forced is_dead to 0.
// blam-cc: EAX -> encounter_index, EDI -> platoon_index, stack -> squad_index, BL -> is_dead
void ai_release_actors_filtered(datum_index encounter_index, int32_t platoon_index, int32_t squad_index, uint8_t is_dead)
{
    if (ai_globals_ptr->actors_valid == 0) {
        return;
    }

    if (encounter_index == (datum_index)k_datum_index_none) {
        actor_iterator_state iterator;

        actor_iterator_new(&iterator, 0);
        while (actor_iterator_next(&iterator) != 0) {
            actor_delete_or_release_unit(iterator.actor_index, is_dead);
        }
    } else {
        datum_index cursor[3];
        datum_index actor_index;

        ai_reference_actor_iterator_init_cursor((int32_t)encounter_index, cursor);
        actor_index = cursor[2];
        while (ai_globals_ptr->actors_valid != 0 && actor_index != (datum_index)k_datum_index_none) {
            actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
            datum_index next = a->next_in_encounter;

            if ((platoon_index == -1 || (int32_t)a->platoon_index == platoon_index) &&
                (squad_index == -1 || (int32_t)a->squad_index == squad_index)) {
                actor_delete_or_release_unit(actor_index, is_dead);
            }
            actor_index = next;
        }
    }
}

#if 0
Original Ghidra decompilation (0x42ab00):

void FUN_0042ab00(int param_1)

{
  int in_EAX;
  int iVar1;
  uint uVar2;
  int unaff_EDI;
  uint local_20;
  undefined4 local_8;

  if (*(char *)(DAT_00880354 + 1) != '\0') {
    if (in_EAX == -1) {
      FUN_00436a30(0);
      iVar1 = actor_iterator_next();
      if (iVar1 != 0) {
        do {
          actor_delete_or_release_unit(local_8);
          iVar1 = actor_iterator_next();
        } while (iVar1 != 0);
        return;
      }
    }
    else {
      FUN_004369f0();
      while ((uVar2 = local_20, *(char *)(DAT_00880354 + 1) != '\0' && (uVar2 != 0xffffffff))) {
        iVar1 = (uVar2 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
        local_20 = *(uint *)(iVar1 + 0x2c);
        if (((unaff_EDI == -1) || (*(short *)(iVar1 + 0x3c) == unaff_EDI)) &&
           ((param_1 == -1 || (*(short *)(iVar1 + 0x3a) == param_1)))) {
          actor_delete_or_release_unit(uVar2);
        }
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
