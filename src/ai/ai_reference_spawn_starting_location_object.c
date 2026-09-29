// ai_reference_spawn_starting_location_object  (Ghidra: ai_reference_spawn_starting_location_object; named for this rewrite)
// address 0x4328c0, size 383 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: resolves a packed ai reference (squad or platoon kind) to one ScenarioSquad
// within the named encounter -- for a platoon reference, the first squad whose
// ScenarioSquad.platoon matches -- then reads that squad's actor_type (+0x20) as an index
// into Scenario.actor_palette (a TagDependency array, size 0x10, tag_id at +0xc), resolves
// the ActorVariant tag's actor_definition (ActorVariant+0x10, the same field
// actor_new/ai_types_notes.md already established), and spawns+attaches an actor via
// actor_new_and_attach_to_unit (0x426ac0, already rewritten), passing the squad's
// initial_state/return_state (ScenarioSquad+0x24/+0x26) through, before re-running the
// dirty-squad morale sweep (0x435f00, this batch). The two truncated-to-char bit tests
// resolve cleanly against named bitfields: Actor.flags bit 26 is "swarm" (types/ai.h's own
// evidence for actor.swarm), and ScenarioEncounterFlags bit 4 is "initially_braindead"
// (types/tags.h's own bitfield comment) -- both match actor_new_and_attach_to_unit's
// reuse_existing and start_active roles.
// register convention: Ghidra fully resolved both parameters.
//   // blam-cc: stack -> unit_index, packed_reference (order not independently confirmed
//   with objdump; param_1 flows straight into actor_new_and_attach_to_unit's unit_index)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

extern ai_globals *ai_globals_ptr;   // 0x00880354
extern Scenario *global_scenario;    // 0x00746f8c
extern tag_instance *tag_instances;  // 0x0087bc14

extern datum_index actor_new_and_attach_to_unit(char reuse_existing, datum_index unit_index,
    datum_index actor_variant_tag, uint32_t encounter_or_none, int16_t squad_index, char ignore_squad,
    datum_index exclude_actor, char start_active, uint16_t unknown_60, int16_t follow_mode,
    uint16_t command_list_index, uint8_t unknown_68); // 0x426ac0, already rewritten
extern void encounters_recompute_dirty(void); // 0x435f00, this batch

// blam-cc: stack -> unit_index, packed_reference
// Resolves a packed ai reference (a squad, or a platoon -- taking its first squad) to a
// ScenarioSquad, and spawns+attaches a new actor of that squad's actor type onto unit_index,
// carrying over the squad's initial/return state. Does nothing if the AI globals, the
// reference or the resolved squad/actor-variant/actor tag chain is not valid.
void ai_reference_spawn_starting_location_object(datum_index unit_index, uint32_t packed_reference)
{
    uint32_t encounter_index;
    ScenarioEncounter *encounter_definition;
    uint32_t squad_index;
    uint32_t kind;

    if (ai_globals_ptr->actors_valid == 0 || packed_reference == (uint32_t)k_datum_index_none ||
        unit_index == (datum_index)k_datum_index_none) {
        return;
    }

    encounter_index = packed_reference & 0xffff;
    if ((int32_t)encounter_index >= global_scenario->encounters.count) {
        return;
    }
    encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)[encounter_index];

    squad_index = 0;
    kind = packed_reference >> 0x1e;
    if (kind == 2) {
        uint32_t requested = (packed_reference >> 0x10) & 0xff;
        squad_index = requested; // requested is always 0..255, so the original's "< 0" guard never fires
    } else if (kind == 1) {
        uint32_t target_platoon = (packed_reference >> 0x10) & 0xff;
        uint32_t candidate = 0;
        if (encounter_definition->squads.count > 0) {
            ScenarioSquad *squads = (ScenarioSquad *)encounter_definition->squads.pointer;
            do {
                if (squads[candidate].platoon == target_platoon) {
                    squad_index = candidate;
                    break;
                }
                candidate = candidate + 1;
            } while (candidate < (uint32_t)encounter_definition->squads.count);
            // if no squad's platoon matches, squad_index is left at its initial value of 0,
            // matching the original's fallthrough (it never re-tests uVar7 on a miss)
        }
    }

    if ((int32_t)squad_index < encounter_definition->squads.count) {
        ScenarioSquad *squad = &((ScenarioSquad *)encounter_definition->squads.pointer)[squad_index];
        int16_t actor_palette_index = (int16_t)squad->actor_type;

        if (actor_palette_index != -1) {
            TagDependency *actor_palette_entry =
                &((TagDependency *)global_scenario->actor_palette.pointer)[actor_palette_index];
            datum_index actor_variant_tag = *(datum_index *)&actor_palette_entry->tag_id;

            if (actor_variant_tag != (datum_index)k_datum_index_none) {
                uint8_t *actor_variant_data =
                    (uint8_t *)tag_instances[actor_variant_tag & 0xffff].data;
                datum_index actor_definition_tag = *(datum_index *)(actor_variant_data + 0x10);

                if (actor_definition_tag != (datum_index)k_datum_index_none) {
                    uint8_t *actor_tag_data = (uint8_t *)tag_instances[actor_definition_tag & 0xffff].data;
                    uint32_t actor_tag_flags = *(uint32_t *)actor_tag_data;
                    char reuse_existing = (char)((actor_tag_flags >> 0x1a) & 1); // Actor.flags bit 26, "swarm"
                    char start_active =
                        (char)((encounter_definition->flags >> 4) & 1); // ScenarioEncounterFlags bit 4, "initially_braindead"

                    actor_new_and_attach_to_unit(reuse_existing, unit_index, actor_variant_tag, encounter_index,
                        (int16_t)squad_index, 0, (datum_index)k_datum_index_none, start_active,
                        (uint16_t)squad->initial_state, (int16_t)squad->return_state, 0xffff, 0);
                    encounters_recompute_dirty();
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4328c0):

void FUN_004328c0(int param_1,uint param_2)

{
  short sVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  short *psVar5;
  int iVar6;
  uint uVar7;
  uint uVar8;

  if ((((*(char *)(DAT_00880354 + 1) != '\0') && (param_1 != -1)) && (param_2 != 0xffffffff)) &&
     (uVar3 = param_2 & 0xffff, (int)uVar3 < *(int *)(global_scenario + 0x42c))) {
    iVar4 = uVar3 * 0xb0 + *(int *)(global_scenario + 0x430);
    uVar7 = 0;
    if (param_2 >> 0x1e == 2) {
      uVar8 = param_2 >> 0x10 & 0xff;
LAB_00432978:
      uVar7 = uVar8;
      if ((int)uVar8 < 0) {
        return;
      }
    }
    else if (param_2 >> 0x1e == 1) {
      uVar8 = 0;
      if (0 < *(int *)(iVar4 + 0x80)) {
        psVar5 = (short *)(*(int *)(iVar4 + 0x84) + 0x22);
        do {
          if ((int)*psVar5 == (param_2 >> 0x10 & 0xff)) goto LAB_00432978;
          uVar8 = uVar8 + 1;
          psVar5 = psVar5 + 0x74;
        } while ((int)uVar8 < *(int *)(iVar4 + 0x80));
      }
    }
    if ((int)uVar7 < *(int *)(iVar4 + 0x80)) {
      sVar1 = *(short *)(uVar7 * 0xe8 + 0x20 + *(int *)(iVar4 + 0x84));
      iVar6 = uVar7 * 0xe8 + *(int *)(iVar4 + 0x84);
      if (((sVar1 != -1) &&
          (uVar8 = *(uint *)(sVar1 * 0x10 + *(int *)(global_scenario + 0x424) + 0xc),
          uVar8 != 0xffffffff)) &&
         (uVar2 = *(uint *)(*(int *)((uVar8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x10),
         uVar2 != 0xffffffff)) {
        actor_new_and_attach_to_unit
                  (**(uint **)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) >> 0x1a & 0xffffff01,
                   param_1,uVar8,uVar3,uVar7,0,0xffffffff,*(uint *)(iVar4 + 0x20) >> 4 & 0xffffff01,
                   (int)*(short *)(iVar6 + 0x24),*(undefined2 *)(iVar6 + 0x26),0xffffffff,0);
        FUN_00435f00();
        return;
      }
    }
  }
  return;
}
#endif
