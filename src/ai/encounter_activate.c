// encounter_activate  (Ghidra: squad_activate; renamed for this rewrite)
// address 0x437710, size 263 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: it reads ScenarioEncounter.precomputed_bsp_index (+0x7e) and refuses to run
//   unless the encounter belongs to the BSP currently loaded (0x0069e8d8), walks
//   encounter.first_actor / actor.next_in_encounter, and finally sets encounter.units_active
//   (+0x0d) and the activation timestamp at +0x10. It is the encounter, not the squad, that
//   carries all three fields (out/phase4/ai_types_notes.md misattribution table).
// register convention: ECX -> encounter_index. Ghidra's in_ECX is the only argument, and the
//   member walk uses the "encounter_index == none means the unassigned list" convention
//   shared with encounter_deactivate and encounter_recompute_morale.
//   // blam-cc: ECX -> encounter_index
//
// UNSURE: actor_create_swarm (0x427f40) and actor_set_units_active (0x427860) are shown by
// Ghidra with no arguments; both are written here as taking the actor they act on.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "ai.h"

extern data_array *encounter_data;   // 0x008802c8
extern Scenario *global_scenario;    // 0x00746f8c
extern int16_t global_structure_bsp_index;    // 0x0069e8d8
extern ai_globals *ai_global_data;   // 0x00880354
extern data_array *actor_data;       // 0x00880360
extern game_time_globals *game_time; // 0x006f1d6c

extern void actor_set_units_active(datum_index actor_index); // 0x427860, not yet rewritten
extern void actor_create_swarm(datum_index actor_index);     // 0x427f40, not yet rewritten

// blam-cc: ECX -> encounter_index
// Brings an encounter to life: every member that is not already active gets its swarm built
// (swarm actors that fail to get one are only marked pending), is flagged active and, unless
// it is already alert, has its units woken. Returns the encounter's units_active flag.
uint8_t encounter_activate(datum_index encounter_index)
{
    encounter *enc;
    ScenarioEncounter *definition;
    actor *a;
    datum_index actor_index;
    datum_index current;

    enc = &((encounter *)encounter_data->data)[encounter_index & 0xffff];
    definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)
        [encounter_index & 0xffff];

    if ((int16_t)definition->precomputed_bsp_index != -1 &&
        (int16_t)definition->precomputed_bsp_index != global_structure_bsp_index) {
        return enc->units_active;
    }

    if (enc->units_active == 0) {
        actor_index = (datum_index)k_datum_index_none;
        if (ai_global_data->actors_valid != 0) {
            if (encounter_index == (datum_index)k_datum_index_none) {
                actor_index = ai_global_data->unknown_08;
            } else {
                actor_index = enc->first_actor;
            }
        }
        while (ai_global_data->actors_valid != 0 &&
               actor_index != (datum_index)k_datum_index_none) {
            current = actor_index;
            a = &((actor *)actor_data->data)[current & 0xffff];
            actor_index = a->next_in_encounter;
            if (a->active != 1) {
                if (a->swarm == 0 || (actor_create_swarm(current),
                                      a->swarm_index != (datum_index)k_datum_index_none)) {
                    a->active = 1;
                    if (a->awareness_level == 0) {
                        actor_set_units_active(current);
                    }
                } else {
                    a->swarm_pending = 1;
                }
            }
        }
    }

    enc->activation_tick = game_time->game_time;
    enc->units_active = 1;
    return enc->units_active;
}

#if 0
Original Ghidra decompilation (0x437710):

undefined4 squad_activate(void)

{
  int iVar1;
  short sVar2;
  int iVar3;
  undefined4 uVar4;
  uint uVar5;
  uint in_ECX;
  uint local_4;

  iVar1 = (in_ECX & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
  iVar3 = (in_ECX & 0xffff) * 0xb0 + *(int *)(global_scenario + 0x430);
  sVar2 = *(short *)(iVar3 + 0x7e);
  uVar4 = CONCAT22((short)((uint)iVar3 >> 0x10),sVar2);
  if ((sVar2 == -1) || (sVar2 == DAT_0069e8d8)) {
    if (*(char *)(iVar1 + 0xd) == '\0') {
      if (*(char *)(DAT_00880354 + 1) != '\0') {
        if (in_ECX == 0xffffffff) {
          local_4 = *(uint *)(DAT_00880354 + 8);
        }
        else {
          local_4 = *(uint *)(iVar1 + 0x14);
        }
      }
      while ((*(char *)(DAT_00880354 + 1) != '\0' && (local_4 != 0xffffffff))) {
        uVar5 = local_4 & 0xffff;
        local_4 = *(uint *)(uVar5 * 0x724 + 0x2c + *(int *)(DAT_00880360 + 0x34));
        iVar3 = uVar5 * 0x724 + *(int *)(DAT_00880360 + 0x34);
        if (*(char *)(iVar3 + 8) != '\x01') {
          if ((*(char *)(iVar3 + 6) == '\0') || (actor_create_swarm(), *(int *)(iVar3 + 0x28) != -1)
             ) {
            *(undefined1 *)(iVar3 + 8) = 1;
            if (*(short *)(iVar3 + 0x6a) == 0) {
              actor_set_units_active();
            }
          }
          else {
            *(undefined1 *)(iVar3 + 0xb) = 1;
          }
        }
      }
    }
    uVar4 = *(undefined4 *)(DAT_006f1d6c + 0xc);
    *(undefined4 *)(iVar1 + 0x10) = uVar4;
    *(undefined1 *)(iVar1 + 0xd) = 1;
  }
  return CONCAT31((int3)((uint)uVar4 >> 8),*(undefined1 *)(iVar1 + 0xd));
}
#endif
