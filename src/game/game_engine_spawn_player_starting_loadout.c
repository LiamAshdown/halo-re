// game_engine_spawn_player_starting_loadout  (Ghidra: game_engine_spawn_player_starting_loadout,
// already named)
// address 0x4611f0, size 450 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("Spawns a player's starting weapon/equipment loadout
// from the scenario's starting-profile table and applies its ammo-reset rules"); types/tags.h
// Scenario::starting_equipment (TagReflexive at +0x390, confirmed against the header's own
// "player_starting_profile @ +0x348" anchor by counting the five 0xc-byte reflexives between
// them), ScenarioStartingEquipment (size 0xcc: flags(4) + type_0..type_3(4x int16) +
// pad[48] + item_collection_1..6(TagDependency, 0x10 each) + pad[48] -- item_collection_1's own
// tag_id field lands at +0x48, matching this function's own loop start exactly);
// netgame_equipment_game_type_matches (0x45f7c0) and tag_reflexive_pick_weighted_random_index
// (0x45f720), both already named in this batch; src/objects/object_new_with_datum_role_control.c
// (the `object_type_definitions[object_tag->object_type]` idiom for the role-0-vs-3 check).
// register convention: no register-passed arguments Ghidra recovers; all three are this
// function's own stack parameters (param_1 is read nowhere in the body).
// UNSURE: object_placement_data_initialize's own placement-buffer argument (EAX/register) and
// object_type_definition::network_delta_message_type's real meaning are unrecoverable here, exactly as already
// flagged in the sibling netgame-equipment spawner (game_engine_update_netgame_equipment.c).
// reconciled: R38 object_type_definition +0x0a/+0x0c/+0x0e/+0x10 -> scenario_placement_offset/scenario_palette_offset/scenario_placement_size/network_delta_message_type (int32, -1 = none)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"

extern Scenario *global_scenario;                  // 0x00746f8c
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern int16_t network_game_mode;                   // 0x00719720
extern tag_instance *tag_instances;                 // 0x0087bc14
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc
extern data_array *object_headers;                  // 0x008603b0

extern uint8_t netgame_equipment_game_type_matches(int16_t *types, int32_t count,
    int32_t current_engine_index); // 0x45f7c0, this batch
extern int32_t tag_reflexive_pick_weighted_random_index(datum_index tag_id); // 0x45f720, this batch
extern void object_placement_data_initialize(object_placement_data *placement,
    datum_index definition_tag, datum_index role); // 0x4f53a0
extern datum_index object_new_with_datum_role_control(object_placement_data *placement,
    uint32_t role); // 0x4f54b0
extern void object_delete_recursive(datum_index object_index, uint8_t recurse_siblings); // 0x4f59d0
extern void object_delete_unparented(datum_index object_index); // 0x4f5aa0
extern uint8_t unit_has_weapon_of_type(datum_index a, datum_index b); // 0x56d610, this batch's
    // sibling FUN_0056d610; UNSURE name/signature, not in this batch's own address range
extern void unit_pickup_weapon(uint32_t flags); // 0x56d400, not in this batch

// UNSURE: `starting_equipment_index` (param_1 in Ghidra) is never read in this body.
// Finds the first Scenario::starting_equipment entry whose game-type list matches the active
// game engine, then spawns up to five items -- one per item_collection slot -- each a weighted
// random pick from that collection's own reflexive, alternating role 3/0 for
// object_new_with_datum_role_control the same way the sibling netgame-equipment spawner does.
// Finally applies the entry's grenade-suppression flags to the caller's in/out frag/plasma
// counts (`*frag_count`/`*plasma_count`).
void game_engine_spawn_player_starting_loadout(uint32_t starting_equipment_index, int32_t *frag_count,
    int32_t *plasma_count)
{
    int32_t count = (int32_t)global_scenario->starting_equipment.count;
    int32_t i;
    ScenarioStartingEquipment *equipment;

    if (count <= 0) {
        return;
    }

    equipment = (ScenarioStartingEquipment *)global_scenario->starting_equipment.pointer;
    i = 0;
    for (;;) {
        if (netgame_equipment_game_type_matches(&equipment->type_0, 4,
                current_game_engine != 0 ? current_game_engine->index : 0)) {
            break;
        }
        i = i + 1;
        equipment = equipment + 1;
        if (count <= i) {
            return;
        }
    }

    {
        TagDependency *item_collections = &equipment->item_collection_1;
        uint8_t first_spawn = 1;
        int32_t slot;

        for (slot = 0; slot < 5; slot++) {
            if (*(int32_t *)&item_collections[slot].tag_id != -1) {
                object_placement_data placement;
                datum_index picked_tag = (datum_index)tag_reflexive_pick_weighted_random_index(
                    *(datum_index *)&item_collections[slot].tag_id);
                uint32_t role = 3;
                datum_index new_object;

                object_placement_data_initialize(&placement, picked_tag, (datum_index)0xffffffff);

                if (network_game_mode == 2) {
                    tag_instance *tag_inst = &tag_instances[picked_tag & 0xffff];
                    Object *object_tag = (Object *)tag_inst->data;
                    if (object_type_definitions[object_tag->object_type]->network_delta_message_type != -1) {
                        role = 0;
                    }
                }

                new_object = object_new_with_datum_role_control(&placement, role);
                if (new_object != (datum_index)0xffffffff) {
                    if (!first_spawn) {
                        datum_index previous = (datum_index)((object_header *)object_headers->data)[new_object & 0xffff].data;
                        // UNSURE: Ghidra reads this as *(int*)(object_headers[new_object]+4), i.e.
                        // the object's own network_role/type-ish field per objects.h object+0x04;
                        // modeled with unit_has_weapon_of_type's own established signature instead.
                        if (unit_has_weapon_of_type(new_object, previous) != 0) {
                            int32_t category = *(int32_t *)((uint8_t *)previous + 4);
                            if (category == 0) {
                                object_delete_unparented(new_object);
                            } else if (category != 3) {
                                goto next_slot;
                            }
                            object_delete_recursive(new_object, 0);
                            goto next_slot;
                        }
                    }
                    unit_pickup_weapon(0 - first_spawn & 2);
                    first_spawn = 0;
                }
            }
        next_slot:;
        }
    }

    if ((equipment->flags & 1) != 0) {
        *frag_count = 0;
        *plasma_count = 0;
    }
    if ((equipment->flags & 2) != 0) {
        *plasma_count = *plasma_count + *frag_count;
        *frag_count = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4611f0), from tools/pack.py 0x4611f0:

void game_engine_spawn_player_starting_loadout(undefined4 param_1,int *param_2,int *param_3)

{
  int iVar1;
  bool bVar2;
  char cVar3;
  undefined4 uVar4;
  uint uVar5;
  byte *pbVar6;
  int iVar7;
  byte *local_90;
  int local_8c;
  uint local_88 [34];

  iVar1 = *(int *)(global_scenario + 0x390);
  iVar7 = 0;
  if (0 < iVar1) {
    local_90 = *(byte **)(global_scenario + 0x394);
    pbVar6 = local_90 + 4;
    while (cVar3 = FUN_0045f7c0(pbVar6), cVar3 == '\0') {
      iVar7 = iVar7 + 1;
      local_90 = local_90 + 0xcc;
      pbVar6 = pbVar6 + 0xcc;
      if (iVar1 <= iVar7) {
        return;
      }
    }
    bVar2 = true;
    pbVar6 = local_90 + 0x48;
    local_8c = 5;
    do {
      if (*(int *)pbVar6 != -1) {
        uVar4 = FUN_0045f720();
        object_placement_data_initialize(uVar4,0xffffffff);
        uVar4 = 3;
        if ((DAT_00719720 == 2) &&
           (*(int *)((&PTR_PTR_0069bfdc)
                     [**(short **)((local_88[0] & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)] + 0x10) !=
            -1)) {
          uVar4 = 0;
        }
        uVar5 = object_new_with_datum_role_control(local_88,uVar4);
        if (uVar5 != 0xffffffff) {
          if (!bVar2) {
            iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar5 & 0xffff) * 0xc);
            cVar3 = FUN_0056d610();
            if (cVar3 != '\0') {
              iVar1 = *(int *)(iVar1 + 4);
              if (iVar1 == 0) {
                object_delete_unparented();
              }
              else if (iVar1 != 3) goto LAB_00461362;
              object_delete_recursive(uVar5,0);
              goto LAB_00461362;
            }
          }
          FUN_0056d400(-bVar2 & 2);
          bVar2 = false;
        }
      }
LAB_00461362:
      pbVar6 = pbVar6 + 0x10;
      local_8c = local_8c + -1;
    } while (local_8c != 0);
    if ((*local_90 & 1) != 0) {
      *param_2 = 0;
      *param_3 = 0;
    }
    if ((*local_90 & 2) != 0) {
      *param_3 = *param_3 + *param_2;
      *param_2 = 0;
    }
  }
  return;
}
#endif
