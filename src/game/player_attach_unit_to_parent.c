// player_attach_unit_to_parent  (Ghidra: FUN_00475c60; named per this rewrite)
// address 0x475c60, size 1096 bytes
// name confidence: 0.3   rewrite confidence: 0.25
// evidence: out/phase4/game_functions.md ("Attaches a player's unit as a child of a target
//   parent object at a given local offset, updating its transform, light attachments, and
//   bounding radius"); shares almost all of its field mapping with
//   src/game/game_engine_reattach_player_unit_unused.c (this batch, 0x475270, callers=0), which
//   is clearly an earlier/superseded version of this same operation -- see that file's header
//   for the underlying types/units.h and types/objects.h evidence, not repeated here.
// objdump -d -M intel --start-address=0x475c60 --stop-address=0x475e40 bin/halo.exe confirms
// the stack-parameter convention (matching FUN_00475270 exactly) and that
// object_set_position_and_orientation is called with two literal 0 (NULL) arguments here, not
// the freshly computed position delta -- see UNSURE below.
// register convention: stack -> player_index, target_object, local_offset.
//
// UNSURE: a position delta (seat-local transform position minus the unit's current
// object.position, offset by object+0x5c/0x60/0x64) is computed into locals that are never read
// again anywhere in the disassembly reachable from this function; transcribed as dead
// computation exactly as Ghidra shows it, since "fixing" it would be inventing behavior. Also
// UNSURE, as in FUN_00475270: the node-local-transform math, the Vehicle/Unit tag field at
// +0x34/+0xbc, and the weapon-marker offset at unit+0x1ea.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include <string.h>

extern data_array *player_data; // 0x0087a480
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern int16_t network_game_mode; // 0x00719720
extern game_time_globals *game_time; // 0x006f1d6c
extern uint8_t *network_client; // 0x0071c2d8

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void object_get_node_local_transform(datum_index object_index, int32_t node_index,
                                             void *out_transform, int32_t unknown); // 0x4f6080, UNSURE args
extern void unit_try_set_animation_state(datum_index unit_handle, int32_t state); // 0x565f90
extern void object_snap_to_parent_marker_and_detach(datum_index object_index); // 0x4f6610, not in this batch
extern void object_set_position_and_orientation(datum_index object_index,
    real_vector3d *forward, real_vector3d *up, real_point3d *position); // 0x4f51c0, canonical
    // 4-parameter form (cheat_teleport_to_camera.c / game_engine_update_teleporter.c);
    // UNSURE at the call site below, where Ghidra shows fewer arguments.
extern void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x4cc0d0
extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table,
                                              int32_t invoke_callback); // 0x4f9a20
extern void unit_recompute_seat_occupants(void); // 0x56ce30, units module, not in this batch
extern void unit_pick_and_ready_next_weapon(void); // 0x56d6a0, units module, not in this batch
extern void unit_update_animation_state_machine(datum_index unit_handle); // 0x565420
extern void unit_reset_orientation_and_find_position(datum_index unit_handle); // 0x55add0, units module, not in this batch
extern void object_recalculate_bounding_radius_recursive(datum_index object_index); // 0x4f82b0
extern uint8_t unit_all_seats_unoccupied(void); // 0x566910, units module, not in this batch
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, memory
    // module's canonical form; blam-cc: EDX -> handle, ESI -> array. UNSURE at the call site
    // below: Ghidra elides both register arguments there.
extern void player_update_history_free_all(void *queue); // 0x4e6f20
extern void unit_dispatch_scripted_event_9(uint8_t flag); // 0x56c370, units module, not in this batch
extern uint8_t player_find_placement_position(uint32_t player_index, uint32_t target_object, void *local_offset); // this batch

// UNSURE, best-effort (see header): attaches player_index's unit as a child of target_object's
// current parent chain unless already there or this machine is a network client, recomputing
// its transform from the parent's seat marker, resetting its embedded seat/animation
// bookkeeping, propagating light attachments, recalculating its bounding radius, and clearing
// the driving player's update-history queues while hosting. Always finishes by forwarding to
// FUN_004757b0 with the same three parameters and returning its result.
uint8_t player_attach_unit_to_parent(uint32_t player_index, uint32_t target_object, void *local_offset)
{
    player *plr;
    datum_index unit_handle;
    object *target_obj;
    uint8_t result;

    plr = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    unit_handle = plr->unit;
    target_obj = object_try_and_get((datum_index)target_object, _object_mask_biped);
    if (target_obj == (object *)0) {
        return 0;
    }

    if (target_obj->parent_object != (datum_index)-1 && network_game_mode != 1) {
        object *unit_obj = ((object_header *)object_data->data)[unit_handle & 0xffff].data;
        unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
        datum_index driver = unit->driver_unit_index;

        if (driver != (datum_index)-1 && *((int16_t *)((uint8_t *)unit_obj + 0x2f0)) != -1) {
            object *driver_obj = ((object_header *)object_data->data)[driver & 0xffff].data;
            unit_data *driver_unit = (unit_data *)((uint8_t *)driver_obj + k_unit_data_offset);
            Unit *driver_tag = (Unit *)tag_instances[driver_obj->definition_tag & 0xffff].data;
            real_matrix4x3 local_transform;
            real_matrix4x3 result_transform;
            Unit *unit_tag;
            Vehicle *unit_as_vehicle_tag;
            real_point3d new_position;

            object_get_node_local_transform(
                driver, (int32_t)((uint8_t *)driver_tag->seats.pointer + 0x24 +
                                   *((int16_t *)((uint8_t *)unit_obj + 0x2f0)) * 0x11c),
                &local_transform, 1);
            // CORRECTED by review. This is NOT a dead computation: objdump 0x475e25..0x475e4e
            // builds a real_point3d at [ebp-0x38] out of these three sums and hands it to
            // object_set_position_and_orientation in EDI. Note the operator -- the original is
            // "fld [ebp-0x2c] ; fadd [ebx+0x5c]", an ADD onto the unit's own position, not the
            // subtraction the first pass wrote.
            new_position.x = local_transform.position.x + unit_obj->position.x;
            new_position.y = local_transform.position.y + unit_obj->position.y;

            unit_tag = (Unit *)tag_instances[unit_obj->definition_tag & 0xffff].data;
            unit_as_vehicle_tag = (Vehicle *)tag_instances[
                ((TagID *)((uint8_t *)unit_tag + 0x34))->index & 0xffff].data; // UNSURE offset
            {
                uint8_t *unknown_block = (uint8_t *)unit_as_vehicle_tag + 0xbc;
                new_position.z = (local_transform.position.z + unit_obj->position.z) -
                                   *(float *)(unknown_block + 8); // UNSURE exact field

                if (driver_unit->driver_unit_index == unit_handle &&
                    *((int8_t *)driver_obj + 0x2a3) != '%' && unit_obj->parent_object != (datum_index)-1) {
                    unit_try_set_animation_state(unit_obj->parent_object, 0x25);
                }

                unit->last_parent_object_index = driver;
                unit->last_seat_change_tick = game_time->game_time;
                if (unit->driver_unit_index == unit_handle) {
                    unit->driver_unit_index = (datum_index)-1;
                }
                if (unit->gunner_unit_index == unit_handle) {
                    unit->gunner_unit_index = (datum_index)-1;
                }

                object_snap_to_parent_marker_and_detach(unit_handle);
                // objdump 0x475e2e..0x475e4e: "push 0 ; push 0 ; push esi" plus
                // "lea edi,[ebp-0x38]", i.e. (object_index, forward=NULL, up=NULL, position).
                object_set_position_and_orientation(unit_handle, 0, 0, &new_position);

                matrix4x3_multiply(&local_transform, (real_matrix4x3 *)(unknown_block + 0xac),
                                    &result_transform);
                unit_obj->forward = result_transform.forward;
                unit_obj->up = result_transform.up;

                {
                    uint8_t *unit_tag_data = (uint8_t *)tag_instances[unit_obj->definition_tag & 0xffff].data;
                    if (*(int32_t *)(unit_tag_data + 0x34) != -1) {
                        if ((unit_obj->flags & 1) != 0) {
                            object_for_each_light_attachment(0, 1, 0); // UNSURE arg shapes
                        }
                        if (*(int32_t *)(unit_tag_data + 0x34) != -1) {
                            object_header *unit_header = &((object_header *)object_data->data)[unit_handle & 0xffff];
                            unit_obj->flags = unit_obj->flags & ~1u;
                            unit_header->flags = unit_header->flags | 2;
                        }
                    }
                }

                *((int16_t *)((uint8_t *)unit_obj + 0x2f0)) = -1; // vehicle_seat_index = -1
                *((uint8_t *)unit_obj + 0x2a7) = 2; // UNSURE
                if (driver_unit->driver_unit_index == unit_handle) {
                    driver_unit->driver_unit_index = (datum_index)-1;
                }
                if (driver_unit->gunner_unit_index == unit_handle) {
                    driver_unit->gunner_unit_index = (datum_index)-1;
                }
            }

            unit_recompute_seat_occupants();
            unit_pick_and_ready_next_weapon();
            unit_update_animation_state_machine(unit_handle);

            {
                uint8_t *marker_ptr = (uint8_t *)unit_obj + *((int16_t *)((uint8_t *)unit_obj + 0x1ea)) + 0x10;
                memcpy(marker_ptr, &result_transform.forward, sizeof(result_transform.forward));
            }

            if (unit_obj->type == _object_type_biped) {
                unit_reset_orientation_and_find_position(unit_handle);
            }
            object_recalculate_bounding_radius_recursive(unit_handle);

            if (unit_all_seats_unoccupied() == 1) {
                object *local_obj = object_try_and_get((datum_index)-1, _object_mask_vehicle); // UNSURE arg
                if (local_obj != (object *)0) {
                    *(int32_t *)((uint8_t *)local_obj + 0x5ac) = game_time->game_time; // UNSURE offset
                }
            }

            if (network_game_mode == 1) {
                void *datum = datum_get(unit_handle, object_data); // UNSURE: both arguments are
                        // register-elided by Ghidra; modeled as the object lookup the
                        // surrounding code performs.
                if (datum != (void *)0 && *(int16_t *)((uint8_t *)datum + 2) == -1) {
                    circular_queue *cq1 = (circular_queue *)((uint8_t *)datum + 0x170);
                    circular_queue *cq2 = (circular_queue *)((uint8_t *)datum + 0x1d0);
                    cq1->read_index = 0;
                    cq1->write_index = 0;
                    cq2->read_index = 0;
                    cq2->write_index = 0;
                }
            }
        }

        if (unit_obj->network_role == 0) {
            unit_dispatch_scripted_event_9(1);
        }

        if (network_game_mode == 1) {
            datum_index controlling_player = unit->controlling_player;
            if (controlling_player != (datum_index)-1) {
                int16_t index = (int16_t)controlling_player;
                if (index >= 0 && index < player_data->maximum_count) {
                    player *cp = (player *)((uint8_t *)player_data->data + (uint32_t)(uint16_t)index * player_data->size);
                    int16_t salt = (int16_t)(controlling_player >> 16);
                    if (cp->identifier != 0 && (salt == 0 || cp->identifier == salt) &&
                        cp->local_player_index == -1 && network_client != (uint8_t *)0) {
                        player_update_history_free_all(*(void **)(network_client + 0xf48));
                    }
                }
            }
        }
    }

    result = player_find_placement_position(player_index, target_object, local_offset);
    return result;
}

#if 0
Original Ghidra decompilation (0x475c60), from tools/pack.py 0x475c60:

undefined4 FUN_00475c60(uint param_1,undefined4 param_2,undefined4 param_3)

{
  byte *pbVar1;
  undefined4 *puVar2;
  uint uVar3;
  uint *puVar4;
  int iVar5;
  uint *puVar6;
  uint uVar7;
  char cVar8;
  int iVar9;
  undefined4 uVar10;
  short sVar11;
  short sVar12;
  undefined1 local_e4 [96];
  float local_84;
  float local_80;
  float local_7c;
  undefined1 local_74 [4];
  uint local_70;
  uint local_6c;
  uint local_68;
  uint local_58;
  uint local_54;
  uint local_50;
  float local_3c;
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  undefined4 local_24;
  undefined4 local_20;
  float local_1c;
  uint *local_18;
  int local_14;
  uint local_10;
  uint local_c;
  undefined4 local_8;

  uVar3 = *(uint *)((param_1 & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34));
  local_10 = uVar3;
  iVar9 = object_try_and_get(1);
  if (iVar9 == 0) {
    return 0;
  }
  if ((*(int *)(iVar9 + 0x11c) != -1) && (DAT_00719720 != 1)) {
    local_8 = (uVar3 & 0xffff) * 0xc;
    puVar4 = *(uint **)(local_8 + 8 + *(int *)(DAT_008603b0 + 0x34));
    local_c = puVar4[0x47];
    if ((local_c != 0xffffffff) && ((short)puVar4[0xbc] != -1)) {
      local_18 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_c & 0xffff) * 0xc);
      iVar9 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_8);
      iVar9 = *(short *)(iVar9 + 0x1f2) + iVar9;
      object_get_node_local_transform
                (local_c,*(int *)(*(int *)((*local_18 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                                 0x2e8) + 0x24 + (short)puVar4[0xbc] * 0x11c,local_e4,1);
      local_30 = *(float *)(iVar9 + 0x28) - local_84;
      local_2c = *(float *)(iVar9 + 0x2c) - local_80;
      iVar5 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)
                                          + 0x34) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0xbc);
      local_14 = iVar5 + 0x68;
      local_24 = *(undefined4 *)(iVar5 + 0x28);
      local_28 = *(float *)(iVar9 + 0x30) - local_7c;
      local_20 = *(undefined4 *)(iVar5 + 0x2c);
      local_1c = *(float *)(iVar5 + 0x30);
      if (((local_18[0xc9] == uVar3) && (*(char *)((int)local_18 + 0x2a3) != '%')) &&
         (puVar4[0x47] != 0xffffffff)) {
        unit_try_set_animation_state(puVar4[0x47],0x25);
      }
      iVar9 = DAT_006f1d6c;
      puVar4[0xcb] = local_c;
      puVar4[0xcc] = *(uint *)(iVar9 + 0xc);
      if (puVar4[0xc9] == uVar3) {
        puVar4[0xc9] = 0xffffffff;
      }
      if (puVar4[0xca] == uVar3) {
        puVar4[0xca] = 0xffffffff;
      }
      FUN_004f6610(uVar3);
      local_3c = local_30 + (float)puVar4[0x17];
      local_38 = local_2c + (float)puVar4[0x18];
      local_34 = (local_28 + (float)puVar4[0x19]) - local_1c;
      object_set_position_and_orientation(uVar3,0,0);
      uVar7 = local_8;
      iVar9 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_8);
      (*(code *)PTR_matrix4x3_multiply_00696664)
                (*(short *)(iVar9 + 0x1f2) + iVar9,local_14,local_74);
      puVar4[0x1d] = local_70;
      puVar4[0x1e] = local_6c;
      puVar4[0x1f] = local_68;
      puVar4[0x20] = local_58;
      puVar4[0x21] = local_54;
      iVar9 = DAT_008603b0;
      puVar4[0x22] = local_50;
      puVar6 = *(uint **)(*(int *)(iVar9 + 0x34) + 8 + uVar7);
      local_14 = *(int *)((*puVar6 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      if (*(int *)(local_14 + 0x34) != -1) {
        if ((puVar6[4] & 1) != 0) {
          object_for_each_light_attachment(0,1);
        }
        if (*(int *)(local_14 + 0x34) != -1) {
          iVar9 = *(int *)(DAT_008603b0 + 0x34);
          puVar6[4] = puVar6[4] & 0xfffffffe;
          pbVar1 = (byte *)(iVar9 + local_8 + 2);
          *pbVar1 = *pbVar1 | 2;
        }
      }
      *(undefined2 *)(puVar4 + 0xbc) = 0xffff;
      *(undefined1 *)((int)puVar4 + 0x2a7) = 2;
      if (local_18[0xc9] == uVar3) {
        local_18[0xc9] = 0xffffffff;
      }
      if (local_18[0xca] == uVar3) {
        local_18[0xca] = 0xffffffff;
      }
      FUN_0056ce30();
      FUN_0056d6a0();
      uVar3 = local_10;
      local_8 = (uint)CONCAT12(0x14,(undefined2)local_8);
      FUN_00565420(local_10);
      puVar2 = (undefined4 *)(*(short *)((int)puVar4 + 0x1ea) + 0x10 + (int)puVar4);
      *puVar2 = local_24;
      puVar2[1] = local_20;
      puVar2[2] = local_1c;
      if ((short)puVar4[0x2d] == 0) {
        FUN_0055add0(uVar3);
      }
      object_recalculate_bounding_radius_recursive(uVar3);
      cVar8 = FUN_00566910();
      if ((cVar8 == '\x01') && (iVar9 = object_try_and_get(2), iVar9 != 0)) {
        *(undefined4 *)(iVar9 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
      }
      if (((DAT_00719720 == 1) && (iVar9 = datum_get(), iVar9 != 0)) &&
         (*(short *)(iVar9 + 2) == -1)) {
        *(undefined4 *)(iVar9 + 0x180) = 0;
        *(undefined4 *)(iVar9 + 0x17c) = 0;
        *(undefined4 *)(iVar9 + 0x1e0) = 0;
        *(undefined4 *)(iVar9 + 0x1dc) = 0;
      }
    }
    if (puVar4[1] == 0) {
      FUN_0056c370(1);
    }
    if ((((DAT_00719720 == 1) && (uVar3 = puVar4[0x86], uVar3 != 0xffffffff)) &&
        (sVar12 = (short)uVar3, -1 < sVar12)) && (sVar12 < *(short *)(DAT_0087a480 + 0x20))) {
      iVar9 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar12;
      sVar12 = *(short *)(iVar9 + *(int *)(DAT_0087a480 + 0x34));
      if ((((sVar12 != 0) && ((sVar11 = (short)(uVar3 >> 0x10), sVar11 == 0 || (sVar12 == sVar11))))
          && (*(short *)(iVar9 + *(int *)(DAT_0087a480 + 0x34) + 2) != -1)) && (DAT_0071c2d8 != 0))
      {
        player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
      }
    }
  }
  uVar10 = FUN_004757b0(param_1,param_2,param_3);
  return uVar10;
}
#endif
