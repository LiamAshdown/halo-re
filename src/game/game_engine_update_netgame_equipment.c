// game_engine_update_netgame_equipment  (Ghidra: game_engine_update_netgame_equipment, already
// named)
// address 0x45f9f0, size 653 bytes (0x45f9f0..0x45fc7c; Ghidra's metadata said 598 and catalogued the
//   loop-increment tail as the bogus function 0x45fc50 "hud_render_scoreboard_ingame" -- the
//   `index < netgame_equipment.count` test that closes the per-entry loop below; re-checked against
//   objdump 0x45fbd0..0x45fc7d by orphan pass 4)
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
// VERIFIED against disassembly 0x45f9f0..0x45fc7c (2026-09-30). Fixed: the __ftol operand is
//   (float)loop_index / (float)count * 300.0 (0x45fa66..0x45fa79); the game type test gets the engine index
//   (engine +4, or -1); object_placement_data_initialize gets the picked item tag; the position/forward vectors
//   are written into the SAME placement block that is passed to object_new_with_datum_role_control (the draft built a
//   separate array); object_list_membership_set gets the new object in ECX; the pickup dispatch gets
//   (new object, definition tag, loop index).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern Scenario *global_scenario;    // 0x00746f8c
extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_time_globals *game_time; // 0x006f1d6c
extern tag_instance *tag_instances;  // 0x0087bc14
extern data_array *object_data;   // 0x008603b0

extern uint8_t netgame_equipment_game_type_matches(int16_t *types, int32_t count,
    int32_t current_engine_index); // 0x45f7c0, this batch; UNSURE real args at call site
extern int32_t tag_reflexive_pick_weighted_random_index(datum_index tag_id); // 0x45f720, this batch
extern void game_engine_dispatch_item_pickup_event(int32_t machine_id, int32_t picked_tag,
    int32_t param_2); // 0x45f850, this batch; UNSURE real args at call site

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void object_delete(datum_index object_index); // 0x4f5bd0, UNSURE exact signature
extern void object_placement_data_initialize(object_placement_data *placement,
    datum_index definition_tag, datum_index role); // 0x4f53a0, canonical form (src/items)
extern datum_index object_new_with_datum_role_control(object_placement_data *placement,
    uint32_t role); // 0x4f54b0
extern void object_list_membership_set(uint32_t object_index, char add); // 0x4f7450, blam-cc: ECX -> object_index, stack -> add
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

        if (!netgame_equipment_game_type_matches((int16_t *)&equipment->type_0, 4,
                current_game_engine != 0 ? current_game_engine->index : -1)) { // 0x45fa39..0x45fa56: esi = engine index or -1, edx = 4
            continue;
        }

        {
            int32_t respawn_interval = 900;
            // 0x45fa66: fild loop_index / (float)count * 300.0 -> __ftol. Staggers each entry's respawn phase across
            //   the 10 s (300 tick) window by its position in the list.
            int32_t extra = (int32_t)((float)loop_index / (float)count * 300.0f);

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
                        object_placement_data placement;
                        datum_index new_object;
                        int32_t picked_tag = tag_reflexive_pick_weighted_random_index(item_collection_tag);

                        // 0x45fb50: push -1 (role); push picked (definition); eax = &placement
                        object_placement_data_initialize(&placement, (datum_index)picked_tag, k_datum_index_none);

                        // 0x45fb5c..0x45fb89: position (+0x18), forward = (cos, sin, 0) (+0x34)
                        placement.position.x = equipment->position.x;
                        placement.position.y = equipment->position.y;
                        placement.position.z = equipment->position.z;
                        placement.forward.i = (float)fcos(equipment->facing);
                        placement.forward.j = (float)fsin(equipment->facing);
                        placement.forward.k = 0.0f;

                        new_object = object_new_with_datum_role_control(&placement, 3);
                        if (new_object != (datum_index)0xffffffff) {
                            object *obj = ((object_header *)object_data->data)[new_object & 0xffff].data;
                            item_data *item = (item_data *)((uint8_t *)obj + sizeof(object));

                            object_list_membership_set(new_object, 0); // 0x45fbba: push 0; ecx = the new object
                            if (((uint8_t *)equipment)[0] & 1) {
                                obj->flags = obj->flags | 0x20;
                            }
                            obj->network_role = 0;
                            object_type_override_call_0x68(new_object); // handle in ESI, elided by Ghidra
                            // 0x45fbdc..0x45fbe7: ecx = new object, push loop index, push the object's definition tag
                            game_engine_dispatch_item_pickup_event(new_object, obj->definition_tag, loop_index);

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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
