// game_engine_update_netgame_equipment  (Ghidra: game_engine_update_netgame_equipment, already
// named)
// address 0x45f9f0, size 598 bytes
// name confidence: 0.55   rewrite confidence: 0.25
// evidence: out/phase4/game_functions.md ("Per-tick update that respawns scenario
// netgame-equipment items once their configured respawn timer has elapsed"); types/tags.h
// ScenarioNetgameEquipment (0x90 bytes: spawn_time +0x0e, unknown_ffffffff +0x10 -- at runtime
// this holds the live spawned object's handle, not really -1 -- position +0x40, facing +0x4c,
// item_collection +0x50, whose TagDependency.tag_id sits at +0x5c), TagDependency,
// TagReflexive; types/items.h item_data (flags +0x1f4, held_game_time +0x204,
// _item_unknown_40_bit); types/objects.h object (position +0x5c, flags +0x10, definition_tag
// +0x00, network_role +0x04).
// register convention: `force respawn now` flag in AL (param_1, already an explicit stack/AL
// parameter in Ghidra's own signature).
// UNSURE: this function reuses one Ghidra-shown local (`iVar7`) for the do-while loop counter,
// a `__ftol()` scratch result and the current game tick in three different spans; split into
// separate, clearly-named locals here (the actual loop counter is `local_98` in the original,
// restored at the bottom of the loop body). The `__ftol()` call itself takes its real argument on
// the x87 stack, invisible to this decompilation -- see random_advance_draws.c for the same
// pattern. FUN_0045f720 (tag_reflexive_pick_weighted_random_index, this batch) is called with no
// visible argument; passed `item_collection_tag` here as the most plausible value still live at
// that point.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "game.h"

extern Scenario *global_scenario;    // 0x00746f8c
extern game_time_globals *game_time; // 0x006f1d6c
extern tag_instance *tag_instances;  // 0x0087bc14
extern data_array *object_headers;   // 0x008603b0

extern uint8_t netgame_equipment_game_type_matches(int16_t *types, int32_t count,
    int32_t current_engine_index); // 0x45f7c0, this batch; UNSURE real args at call site
extern int32_t tag_reflexive_pick_weighted_random_index(datum_index tag_id); // 0x45f720, this batch
extern void game_engine_dispatch_item_pickup_event(int32_t machine_id, int32_t param_1,
    int32_t param_2); // 0x45f850, this batch; UNSURE real args at call site
extern int32_t __ftol(void); // 0x6391b4, MSVC runtime; UNSURE: real argument is on the x87 stack

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void object_delete(datum_index object_index); // 0x4f5bd0, UNSURE exact signature
extern void object_placement_data_initialize(object_placement_data *placement,
    datum_index definition_tag, datum_index role); // 0x4f53a0, canonical form (src/items)
extern datum_index object_new_with_datum_role_control(object_placement_data *placement,
    uint32_t role); // 0x4f54b0
extern void object_list_membership_set(int32_t unknown); // 0x4f7450, not in this batch
extern void object_type_override_call_0x68(uint32_t object_index); // 0x4f4560, objects module; handle in ESI

extern double fcos(double radians); // a single x87 FCOS instruction (see src/game/vector3d_clamp_length.c for the same sqrt idiom)
extern double fsin(double radians); // a single x87 FSIN instruction
extern double sqrt(double x); // a single x87 FSQRT instruction, per src/items/item_accelerate.c

void game_engine_update_netgame_equipment(char force_respawn)
{
    int16_t loop_index;
    int32_t count = global_scenario->netgame_equipment.count;

    if (count <= 0) {
        return;
    }

    for (loop_index = 0; loop_index < count; loop_index++) {
        ScenarioNetgameEquipment *equipment =
            &((ScenarioNetgameEquipment *)global_scenario->netgame_equipment.pointer)[loop_index];
        datum_index item_collection_tag = *(datum_index *)&equipment->item_collection.tag_id;
            // TagID {index;id} is bit-identical to a datum_index; see src/objects/flag_new.c

        if (!netgame_equipment_game_type_matches((int16_t *)&equipment->type_0, 4, 0)) {
            // UNSURE: count (4) and current_engine_index (0) args not visible at this call site
            continue;
        }

        {
            int32_t respawn_interval = 900;
            int32_t extra = __ftol(); // UNSURE: see header note

            if (equipment->spawn_time != 0) {
                respawn_interval = equipment->spawn_time * 0x1e;
            } else if (item_collection_tag != (datum_index)0xffffffff) {
                int16_t permutation_count =
                    *(int16_t *)((uint8_t *)tag_instances[item_collection_tag & 0xffff].data + 0x0c);
                if (permutation_count != 0) {
                    respawn_interval = permutation_count * 0x1e;
                }
            }
            respawn_interval = respawn_interval + extra;

            {
                int32_t now = game_time->game_time;

                if (now % respawn_interval == 0 || force_respawn == 1) {
                    if (equipment->unknown_ffffffff != 0xffffffff) {
                        object *existing = object_try_and_get((datum_index)equipment->unknown_ffffffff, _object_mask_item);
                        if (existing != 0 && (((item_data *)((uint8_t *)existing + sizeof(object)))->flags & 0x40) != 0) {
                            float dx = existing->position.x - equipment->position.x;
                            float dy = existing->position.y - equipment->position.y;
                            float dz = existing->position.z - equipment->position.z;
                            float dist = (float)sqrt((double)(dx * dx + dy * dy + dz * dz));

                            if (dist <= 0.5f || (existing->flags & 0x20) == 0) {
                                ((item_data *)((uint8_t *)existing + sizeof(object)))->held_game_time =
                                    respawn_interval - 900 + now;
                                continue; // still resting where it was dropped, or too close to move; leave it
                            }
                            object_delete((datum_index)equipment->unknown_ffffffff);
                        }
                        equipment->unknown_ffffffff = 0xffffffff;
                    }

                    {
                        int32_t placement[6];   // UNSURE: true object_placement_data layout
                        uint32_t creation_data[6]; // matches Ghidra's 24-byte `local_88`
                        datum_index new_object;
                        int32_t picked_tag = tag_reflexive_pick_weighted_random_index(item_collection_tag);

                        object_placement_data_initialize((object_placement_data *)placement, k_datum_index_none,
                                             k_datum_index_none); // UNSURE: role elided by Ghidra

                        creation_data[0] = *(uint32_t *)&equipment->position.x;
                        creation_data[1] = *(uint32_t *)&equipment->position.y;
                        creation_data[2] = *(uint32_t *)&equipment->position.z;
                        creation_data[3] = 0;
                        *(float *)&creation_data[4] = (float)fcos(equipment->facing);
                        *(float *)&creation_data[5] = (float)fsin(equipment->facing);

                        new_object = object_new_with_datum_role_control((object_placement_data *)creation_data, 3);
                        if (new_object != (datum_index)0xffffffff) {
                            object *obj = ((object_header *)object_headers->data)[new_object & 0xffff].data;
                            item_data *item = (item_data *)((uint8_t *)obj + sizeof(object));

                            object_list_membership_set(0);
                            if (((uint8_t *)equipment)[0] & 1) {
                                obj->flags = obj->flags | 0x20;
                            }
                            obj->network_role = 0;
                            object_type_override_call_0x68(new_object); // handle in ESI, elided by Ghidra
                            game_engine_dispatch_item_pickup_event(obj->definition_tag, picked_tag, 0);
                                // UNSURE: real args not visible at this call site

                            item->held_game_time = item->held_game_time + respawn_interval - 900;

                            if (*(int32_t *)tag_instances[item_collection_tag & 0xffff].data == 1) {
                                item->flags = item->flags | 0x40;
                                equipment->unknown_ffffffff = new_object;
                            } else {
                                item->flags = item->flags & ~0x40u;
                            }
                        }
                    }
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x45f9f0), from tools/pack.py 0x45f9f0:

void game_engine_update_netgame_equipment(char param_1)

{
  undefined4 *puVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  int iVar5;
  char cVar6;
  int iVar7;
  int iVar8;
  undefined4 uVar9;
  uint uVar10;
  short sVar11;
  int iVar12;
  byte *pbVar13;
  float10 fVar14;
  int local_98;
  undefined1 local_88 [24];
  undefined4 local_70;
  undefined4 local_6c;
  undefined4 local_68;
  float local_54;
  float local_50;
  undefined4 local_4c;

  iVar5 = global_scenario;
  local_98 = 0;
  if (0 < *(int *)(global_scenario + 900)) {
    iVar7 = 0;
    do {
      pbVar13 = (byte *)(iVar7 * 0x90 + *(int *)(iVar5 + 0x388));
      cVar6 = FUN_0045f7c0(pbVar13 + 4);
      if (cVar6 != '\0') {
        iVar12 = 900;
        iVar7 = __ftol();
        sVar11 = *(short *)(pbVar13 + 0xe);
        if ((sVar11 != 0) ||
           ((*(uint *)(pbVar13 + 0x5c) != 0xffffffff &&
            (sVar11 = *(short *)(*(int *)((*(uint *)(pbVar13 + 0x5c) & 0xffff) * 0x20 + 0x14 +
                                         DAT_0087bc14) + 0xc), sVar11 != 0)))) {
          iVar12 = sVar11 * 0x1e;
        }
        iVar12 = iVar12 + iVar7;
        iVar7 = *(int *)(DAT_006f1d6c + 0xc);
        if ((iVar7 % iVar12 == 0) || (param_1 == '\x01')) {
          if (*(int *)(pbVar13 + 0x10) != -1) {
            iVar8 = object_try_and_get(0x1c);
            if ((iVar8 != 0) && ((*(byte *)(iVar8 + 500) & 0x40) != 0)) {
              fVar2 = *(float *)(iVar8 + 0x5c) - *(float *)(pbVar13 + 0x40);
              fVar3 = *(float *)(iVar8 + 0x60) - *(float *)(pbVar13 + 0x44);
              fVar4 = *(float *)(iVar8 + 100) - *(float *)(pbVar13 + 0x48);
              if ((SQRT(fVar2 * fVar2 + fVar4 * fVar4 + fVar3 * fVar3) <= 0.5) ||
                 ((*(byte *)(iVar8 + 0x10) & 0x20) == 0)) {
                *(int *)(iVar8 + 0x204) = iVar12 + -900 + iVar7;
                goto LAB_0045fc4c;
              }
              object_delete();
            }
            pbVar13[0x10] = 0xff;
            pbVar13[0x11] = 0xff;
            pbVar13[0x12] = 0xff;
            pbVar13[0x13] = 0xff;
          }
          uVar9 = FUN_0045f720();
          object_placement_data_initialize(uVar9,0xffffffff);
          fVar14 = (float10)fcos((float10)*(float *)(pbVar13 + 0x4c));
          local_70 = *(undefined4 *)(pbVar13 + 0x40);
          local_6c = *(undefined4 *)(pbVar13 + 0x44);
          local_68 = *(undefined4 *)(pbVar13 + 0x48);
          local_4c = 0;
          local_54 = (float)fVar14;
          fVar14 = (float10)fsin((float10)*(float *)(pbVar13 + 0x4c));
          local_50 = (float)fVar14;
          uVar10 = object_new_with_datum_role_control(local_88,3);
          if (uVar10 != 0xffffffff) {
            puVar1 = *(undefined4 **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar10 & 0xffff) * 0xc);
            FUN_004f7450(0);
            if ((*pbVar13 & 1) != 0) {
              puVar1[4] = puVar1[4] | 0x20;
            }
            puVar1[1] = 0;
            object_type_override_call_0x68(new_object); // handle in ESI, elided by Ghidra
            FUN_0045f850(*puVar1,local_98);
            iVar7 = DAT_0087bc14;
            puVar1[0x81] = puVar1[0x81] + iVar12 + -900;
            if (**(int **)((*(uint *)(pbVar13 + 0x5c) & 0xffff) * 0x20 + 0x14 + iVar7) == 1) {
              puVar1[0x7d] = puVar1[0x7d] | 0x40;
              *(uint *)(pbVar13 + 0x10) = uVar10;
            }
            else {
              puVar1[0x7d] = puVar1[0x7d] & 0xffffffbf;
            }
          }
        }
      }
LAB_0045fc4c:
      local_98 = local_98 + 1;
      iVar7 = (int)(short)local_98;
    } while (iVar7 < *(int *)(iVar5 + 900));
  }
  return;
}
#endif
