// ai_reference_detach_actors_from_encounters  (Ghidra: ai_reference_detach_actors_from_encounters; named for this rewrite)
// address 0x4351c0, size 153 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: it drives the packed-ai-reference actor iterator pair
//   (ai_reference_actor_iterator_new 0x432650 / _next 0x4326d0, both already rewritten),
//   calls encounter_remove_actor (0x436620) on every actor it visits, then relinks the actor
//   onto ai_globals.unknown_08 (the unassigned-actor list types/ai.h documents) and finally
//   flushes the dirty encounters. ai_object_list_detach_actors_from_encounters @0x435260 is
//   the object-list twin of this function and does exactly the same thing.
// register convention: EAX -> packed_reference (Ghidra's in_EAX, tested against -1 before
//   the iterator is built).
//   // blam-cc: EAX -> packed_reference
//
// UNSURE:
//  - actor_movement_action_cancel is called once before and once after the relink; it is not rewritten
//    anywhere in this module, and Ghidra shows it with no arguments. It is written here as
//    taking the actor index, which is the only live value at both call sites.
//  - actor + 0x10 is set to 0x5a when actor.active is set and 0 otherwise. 0x5a is the same
//    "re-evaluation countdown" constant encounters_update_activation uses on the same field.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern ai_globals *ai_globals_ptr; // 0x00880354
extern data_array *actor_data;     // 0x00880360

extern void ai_reference_actor_iterator_new(uint32_t packed_reference,
    ai_reference_actor_iterator *out_iterator);                             // 0x432650
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0
extern void actor_movement_action_cancel(datum_index actor_index);   // 0x428650, not yet rewritten
extern void encounter_remove_actor(datum_index actor_index, uint8_t skip_counters); // 0x436620, blam-cc: EAX -> actor_index
extern void encounters_recompute_dirty(void);                               // 0x435f00

// blam-cc: EAX -> packed_reference
// Pulls every actor a packed ai reference names out of its encounter and parks it on the
// global unassigned-actor list.
void ai_reference_detach_actors_from_encounters(uint32_t packed_reference)
{
    ai_reference_actor_iterator iterator;
    actor *a;
    actor *self;

    if (packed_reference == 0xffffffff) {
        return;
    }

    ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        actor_movement_action_cancel(iterator.actor_index);
        encounter_remove_actor(iterator.actor_index, 0);
        if (ai_globals_ptr->actors_valid != 0) {
            self = &((actor *)actor_data->data)[iterator.actor_index & 0xffff];
            self->next_in_encounter = ai_globals_ptr->first_encounterless_actor;
            ai_globals_ptr->first_encounterless_actor = iterator.actor_index;
            self->unknown_09 = 1;
            *(uint16_t *)&self->unknown_10[0] =
                (uint16_t)(-(uint16_t)(self->active != 0) & 0x5a);
            actor_movement_action_cancel(iterator.actor_index);
        }
        a = ai_reference_actor_iterator_next(&iterator);
    }
    encounters_recompute_dirty();
}

#if 0
Original Ghidra decompilation (0x4351c0):

void FUN_004351c0(void)

{
  int in_EAX;
  int iVar1;
  int iVar2;
  uint local_8;

  if (in_EAX != -1) {
    FUN_00432650();
    iVar1 = FUN_004326d0();
    while (iVar1 != 0) {
      FUN_00428650();
      squad_remove_actor(0);
      iVar1 = DAT_00880354;
      if (*(char *)(DAT_00880354 + 1) != '\0') {
        iVar2 = (local_8 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
        *(undefined4 *)(iVar2 + 0x2c) = *(undefined4 *)(DAT_00880354 + 8);
        *(uint *)(iVar1 + 8) = local_8;
        *(undefined1 *)(iVar2 + 9) = 1;
        *(ushort *)(iVar2 + 0x10) = -(ushort)(*(char *)(iVar2 + 8) != '\0') & 0x5a;
        FUN_00428650();
      }
      iVar1 = FUN_004326d0();
    }
    FUN_00435f00();
  }
  return;
}
#endif
