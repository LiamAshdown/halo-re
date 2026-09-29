// encounter_deactivate  (Ghidra: squad_deactivate; renamed for this rewrite)
// address 0x437870, size 204 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: the exact inverse of encounter_activate @0x437710 - it clears
//   encounter.units_active (+0x0d), walks the same encounter member list, and for every
//   still-active member calls actor_clear_perceived_props / actor_delete_swarm / actor_set_units_active,
//   clears actor.active and stamps actor+0x0c with the current tick. It also flushes the
//   encounter's "ai pursuit" ring through squad_recent_object_list_clear @0x436c10, which
//   hangs off encounter.first_pursuit (types/ai.h).
// register convention: EAX -> encounter_index (Ghidra's in_EAX), no stack arguments.
//   // blam-cc: EAX -> encounter_index
//
// UNSURE: actor_clear_perceived_props, actor_delete_swarm (0x4280b0) and actor_set_units_active (0x427860)
// are all shown by Ghidra without arguments; they are written here as taking the actor they
// act on, which is what the surrounding code establishes.
//
// Note the redundant double test in the original: the member is checked for actor.active
// twice against the same byte, once through each of the two aliases Ghidra built for the
// actor pointer. Preserved as one test here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "ai.h"

extern data_array *encounter_data;   // 0x008802c8
extern ai_globals *ai_globals_ptr;   // 0x00880354
extern data_array *actor_data;       // 0x00880360
extern game_time_globals *game_time; // 0x006f1d6c

extern void squad_recent_object_list_clear(datum_index encounter_index); // 0x436c10
extern void actor_clear_perceived_props(datum_index actor_index);      // 0x427e00, not yet rewritten
extern void actor_delete_swarm(datum_index actor_index);       // 0x4280b0, not yet rewritten
extern void actor_set_units_active(datum_index actor_index, uint8_t dormant); // 0x427860, blam-cc: EAX, BL

// blam-cc: EAX -> encounter_index
// Puts an encounter back to sleep: clears units_active, drops its recently-seen-object ring
// and, for every member that was still active, tears down its swarm, re-syncs its units and
// stamps the deactivation tick.
void encounter_deactivate(datum_index encounter_index)
{
    encounter *enc;
    actor *a;
    datum_index actor_index;
    datum_index current;

    enc = &((encounter *)encounter_data->data)[encounter_index & 0xffff];
    enc->units_active = 0;
    squad_recent_object_list_clear(encounter_index);

    actor_index = (datum_index)k_datum_index_none;
    if (ai_globals_ptr->actors_valid != 0) {
        if (encounter_index == (datum_index)k_datum_index_none) {
            actor_index = ai_globals_ptr->first_encounterless_actor;
        } else {
            actor_index = enc->first_actor;
        }
    }

    while (ai_globals_ptr->actors_valid != 0 && actor_index != (datum_index)k_datum_index_none) {
        current = actor_index;
        a = &((actor *)actor_data->data)[current & 0xffff];
        actor_index = a->next_in_encounter;
        if (a->active != 0) {
            actor_clear_perceived_props(current);
            actor_delete_swarm(current);
            actor_set_units_active(current, 1);
            a->active = 0;
            a->unknown_0c = (datum_index)game_time->game_time;
        }
    }
}

#if 0
Original Ghidra decompilation (0x437870):

void squad_deactivate(void)

{
  uint in_EAX;
  int iVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint local_4;

  iVar1 = DAT_008802c8;
  iVar3 = (in_EAX & 0xffff) * 0x6c;
  *(undefined1 *)(iVar3 + 0xd + *(int *)(DAT_008802c8 + 0x34)) = 0;
  squad_recent_object_list_clear();
  iVar2 = DAT_00880354;
  if (*(char *)(DAT_00880354 + 1) != '\0') {
    if (in_EAX == 0xffffffff) {
      local_4 = *(uint *)(DAT_00880354 + 8);
    }
    else {
      local_4 = *(uint *)(iVar3 + 0x14 + *(int *)(iVar1 + 0x34));
    }
  }
  while ((uVar4 = local_4, *(char *)(iVar2 + 1) != '\0' && (uVar4 != 0xffffffff))) {
    iVar1 = *(int *)(DAT_00880360 + 0x34);
    iVar3 = (uVar4 & 0xffff) * 0x724;
    local_4 = *(uint *)(iVar3 + 0x2c + iVar1);
    if ((*(char *)(iVar3 + iVar1 + 8) != '\0') &&
       (iVar1 = (uVar4 & 0xffff) * 0x724 + iVar1, *(char *)(iVar1 + 8) != '\0')) {
      FUN_00427e00(uVar4);
      actor_delete_swarm();
      actor_set_units_active();
      iVar2 = DAT_00880354;
      iVar3 = DAT_006f1d6c;
      *(undefined1 *)(iVar1 + 8) = 0;
      *(undefined4 *)(iVar1 + 0xc) = *(undefined4 *)(iVar3 + 0xc);
    }
  }
  return;
}
#endif
