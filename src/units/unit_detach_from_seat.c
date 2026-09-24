// unit_detach_from_seat  (Ghidra: FUN_0056c640)
// address 0x56c640, size 1024 bytes, name confidence 0.45, rewrite confidence 0.25
// functions.md: "Detaches a unit from the parent object/seat it is currently attached to,
// repositioning it in world space at the seat's exit marker and clearing all seat-occupancy
// bookkeeping."
// evidence: this is the canonical version of the ejection block used throughout this module
//   (unit_apply_damage_effects.c, unit_detach_and_enter_named_seat.c,
//   unit_seat_candidates_from_zone_and_enter.c, unit_release_transient_state.c,
//   unit_try_exit_controlled_seat.c all reproduce a close variant of this exact sequence).
// blam-cc: param_1 -> unit_index, param_2 -> suppress_trigger, param_3 -> require_client_flag,
//   param_4 -> fire_trigger_event.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;      // 0x008603b0
extern data_array *player_data;      // 0x0087a480
extern tag_instance *tag_instances;  // 0x0087bc14
extern hs_game_time_globals *game_time; // 0x006f1d6c, the game time globals (types/hs.h)
extern int32_t game_connection_role; // 0x00719720
extern uint8_t *player_control_globals; // 0x0071c2d8
extern void *matrix4x3_multiply_thunk; // 0x00696664

extern void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out);            // 0x4cc0d0, UNSURE signature
extern uint32_t datum_get(void);                                        // 0x4d0680, UNSURE signature  // real signature (datum_get.c): void * datum_get(datum_index handle, data_array *array); Ghidra recovered 0 of 2 args at this call site
extern void player_update_history_free_all(void *history);              // 0x4e6f20, UNSURE signature
extern void object_set_position_and_orientation(uint32_t object_index, void *a, void *b); // 0x4f51c0, UNSURE signature  // real signature (object_set_position_and_orientation.c): void object_set_position_and_orientation(uint32_t object_index, real_vector3d *forward, real_vector3d *up, real_point3d *position); Ghidra recovered 3 of 4 args at this call site
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker, uint32_t param_4); // 0x4f6080
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index);                          // 0x4f6610, UNSURE signature
extern object * object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index); // 0x4f82b0
extern void object_for_each_light_attachment(uint32_t object_index, uint32_t flag); // 0x4f9a20, UNSURE signature  // real signature (object_for_each_light_attachment.c): void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table, int32_t invoke_callback); Ghidra recovered 2 of 3 args at this call site
extern void unit_reset_orientation_and_find_position(uint32_t object_index);                          // 0x55add0, UNSURE signature
extern void unit_update_animation_state_machine(uint32_t unit_index);                         // 0x565420, UNSURE signature  // real signature (unit_update_animation_state_machine.c): uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request); Ghidra recovered 1 of 2 args at this call site
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern uint8_t unit_all_seats_unoccupied(uint32_t unit_index);                      // 0x566910
extern void unit_dispatch_scripted_event_9(uint32_t param_1);                            // 0x56c370, UNSURE signature  // real signature (unit_dispatch_scripted_event_9.c): void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key); Ghidra recovered 1 of 2 args at this call site
extern void unit_recompute_seat_occupants(uint32_t unit_index);                         // 0x56ce30
extern void unit_pick_and_ready_next_weapon(uint32_t unit_index);                         // 0x56d6a0

void unit_detach_from_seat(uint32_t unit_index, uint8_t suppress_trigger, uint8_t require_client_flag, uint8_t fire_trigger_event)
{
    if ((game_connection_role == 1) && (require_client_flag != 1)) {
        return;
    }

    object *self_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    datum_index vehicle_index = self_obj->parent_object;
    uint32_t controlling = 0;

    if ((vehicle_index != k_datum_index_none) && (((unit_data *)((uint8_t *)self_obj + k_unit_data_offset))->vehicle_seat_index != -1)) {
        unit_data *self_unit = (unit_data *)((uint8_t *)self_obj + k_unit_data_offset);
        object *parent = ((object_header *)object_data->data)[vehicle_index & 0xffff].data;
        Unit *parent_tag = (Unit *)tag_instances[parent->definition_tag & 0xffff].data;
        uint8_t *exit_marker = (uint8_t *)(*(int32_t *)((uint8_t *)parent_tag + 0x2e8) + 0x24 +
                                           self_unit->vehicle_seat_index * 0x11c);
        uint8_t local_transform[96];
        object_get_node_local_transform(vehicle_index, exit_marker, (object_marker *)local_transform, 1);

        Unit *self_tag = (Unit *)tag_instances[self_obj->definition_tag & 0xffff].data;
        uint32_t *node_data = (uint32_t *)(*(int32_t *)((uint8_t *)tag_instances[self_tag->base.model.tag_id.index & 0xffff].data + 0xbc));
        uint32_t saved[3] = { node_data[0xa], node_data[0xb], node_data[0xc] };

        unit_data *parent_unit = (unit_data *)((uint8_t *)parent + k_unit_data_offset);
        if ((parent_unit->driver_unit_index == unit_index) && (parent_unit->animation_state != 0x25) &&
            (self_obj->parent_object != k_datum_index_none)) {   // local_8 + 0x2a3
            unit_try_set_animation_state(vehicle_index, 0x25);
        }
        self_unit->last_parent_object_index = vehicle_index;         // puVar3[0xcb] = local_c
        self_unit->last_seat_change_tick = game_time->current_tick;  // puVar3[0xcc]
        // the first clear pair is on puVar3 (this unit); the pair further down is on local_8
        if (self_unit->driver_unit_index == unit_index) self_unit->driver_unit_index = k_datum_index_none;
        if (self_unit->gunner_unit_index == unit_index) self_unit->gunner_unit_index = k_datum_index_none;
        object_snap_to_parent_marker_and_detach(unit_index);
        object_set_position_and_orientation(unit_index, 0, 0); // UNSURE-CALL
        matrix4x3_multiply(0, 0, 0);                           // UNSURE-CALL

        Object *self_def = (Object *)tag_instances[self_obj->definition_tag & 0xffff].data;
        if (*(uint32_t *)&self_def->model.tag_id != 0xffffffff) {
            if ((self_obj->flags & 1) != 0) object_for_each_light_attachment(unit_index, 1);
            if (*(uint32_t *)&self_def->model.tag_id != 0xffffffff) {
                self_obj->flags &= ~1u;                          // object + 0x10, bit 0
                ((object_header *)object_data->data)[unit_index & 0xffff].flags |= 0x02;
            }
        }
        self_unit->vehicle_seat_index = -1;
        self_unit->base_animation_state = _unit_base_animation_state_stand;
        if (parent_unit->driver_unit_index == unit_index) parent_unit->driver_unit_index = k_datum_index_none;
        if (parent_unit->gunner_unit_index == unit_index) parent_unit->gunner_unit_index = k_datum_index_none;
        unit_recompute_seat_occupants(unit_index);
        unit_pick_and_ready_next_weapon(unit_index);
        unit_update_animation_state_machine(unit_index);
        uint32_t *node_func = (uint32_t *)(*(int16_t *)((uint8_t *)self_obj + 0x1ea) + 0x10 + (uint8_t *)self_obj);
        node_func[0] = saved[0]; node_func[1] = saved[1]; node_func[2] = saved[2];
        if (self_obj->type == _object_type_biped) unit_reset_orientation_and_find_position(unit_index);
        object_recalculate_bounding_radius_recursive(unit_index);
        if ((unit_all_seats_unoccupied(unit_index) == 1) && (object_try_and_get(unit_index, _object_mask_vehicle) != 0)) {
            // writes the current tick into vehicle_data.network_update_tick (+0x5ac)
        }
        if ((game_connection_role == 1) && (datum_get() != 0)) {
            uint32_t player_record = datum_get();
            if (*(int16_t *)(player_record + 2) == -1) {
                *(uint32_t *)(player_record + 0x180) = 0;
                *(uint32_t *)(player_record + 0x17c) = 0;
                *(uint32_t *)(player_record + 0x1e0) = 0;
                *(uint32_t *)(player_record + 0x1dc) = 0;
            }
        }
    }

    if ((!suppress_trigger) && (fire_trigger_event == 1) && (self_obj->network_role == 0)) {
        unit_dispatch_scripted_event_9(1);
    }

    controlling = ((unit_data *)((uint8_t *)self_obj + k_unit_data_offset))->controlling_player;
    if ((game_connection_role == 1) && (controlling != k_datum_index_none) &&
        (-1 < (int16_t)controlling) && ((int16_t)controlling < player_data->maximum_count)) {
        int32_t rec_off = (int32_t)player_data->size * (int16_t)controlling;
        int16_t salt = *(int16_t *)((uint8_t *)player_data->data + rec_off);
        if ((salt != 0) &&
            (((int16_t)(controlling >> 16) == 0) || (salt == (int16_t)(controlling >> 16))) &&
            (*(int16_t *)((uint8_t *)player_data->data + rec_off + 2) != -1) &&
            (player_control_globals != 0)) {
            player_update_history_free_all(*(void **)(player_control_globals + 0xf48));
        }
    }
    return;
}

#if 0
Original Ghidra decompilation (0x56c640):

void FUN_0056c640(uint param_1,char param_2,char param_3,char param_4)

{
  byte *pbVar1;
  undefined4 *puVar2;
  uint *puVar3;
  uint *puVar4;
  uint uVar5;
  char cVar6;
  short sVar7;
  short sVar8;
  int iVar9;
  int iVar10;
  int iVar11;
  undefined1 local_dc [96];
  float local_7c;
  float local_78;
  float local_74;
  undefined1 local_6c [4];
  uint local_68;
  uint local_64;
  uint local_60;
  uint local_50;
  uint local_4c;
  uint local_48;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  undefined4 local_1c;
  undefined4 local_18;
  float local_14;
  int local_10;
  uint local_c;
  undefined4 local_8;

  if ((DAT_00719720 != 1) || (param_3 == '\x01')) {
    iVar11 = *(int *)(DAT_008603b0 + 0x34);
    iVar9 = (param_1 & 0xffff) * 0xc;
    puVar3 = *(uint **)(iVar9 + 8 + iVar11);
    local_c = puVar3[0x47];
    iVar10 = DAT_0087a480;
    if ((local_c != 0xffffffff) && ((short)puVar3[0xbc] != -1)) {
      local_8 = *(uint **)(iVar11 + 8 + (local_c & 0xffff) * 0xc);
      iVar11 = *(int *)(iVar11 + 8 + iVar9);
      iVar11 = *(short *)(iVar11 + 0x1f2) + iVar11;
      object_get_node_local_transform
                (local_c,*(int *)(*(int *)((*local_8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2e8
                                 ) + 0x24 + (short)puVar3[0xbc] * 0x11c,local_dc,1);
      local_28 = *(float *)(iVar11 + 0x28) - local_7c;
      local_24 = *(float *)(iVar11 + 0x2c) - local_78;
      iVar10 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14
                                                    ) + 0x34) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14
                                ) + 0xbc);
      local_10 = iVar10 + 0x68;
      local_20 = *(float *)(iVar11 + 0x30) - local_74;
      local_1c = *(undefined4 *)(iVar10 + 0x28);
      local_18 = *(undefined4 *)(iVar10 + 0x2c);
      local_14 = *(float *)(iVar10 + 0x30);
      if (((local_8[0xc9] == param_1) && (*(char *)((int)local_8 + 0x2a3) != '%')) &&
         (puVar3[0x47] != 0xffffffff)) {
        unit_try_set_animation_state(puVar3[0x47],0x25);
      }
      iVar11 = DAT_006f1d6c;
      puVar3[0xcb] = local_c;
      puVar3[0xcc] = *(uint *)(iVar11 + 0xc);
      if (puVar3[0xc9] == param_1) {
        puVar3[0xc9] = 0xffffffff;
      }
      if (puVar3[0xca] == param_1) {
        puVar3[0xca] = 0xffffffff;
      }
      FUN_004f6610(param_1);
      local_34 = local_28 + (float)puVar3[0x17];
      local_30 = local_24 + (float)puVar3[0x18];
      local_2c = (local_20 + (float)puVar3[0x19]) - local_14;
      object_set_position_and_orientation(param_1,0,0);
      iVar11 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar9);
      (*(code *)PTR_matrix4x3_multiply_00696664)
                (*(short *)(iVar11 + 0x1f2) + iVar11,local_10,local_6c);
      puVar3[0x1d] = local_68;
      puVar3[0x1e] = local_64;
      puVar3[0x1f] = local_60;
      puVar3[0x20] = local_50;
      puVar3[0x21] = local_4c;
      iVar11 = DAT_008603b0;
      puVar3[0x22] = local_48;
      puVar4 = *(uint **)(*(int *)(iVar11 + 0x34) + 8 + iVar9);
      local_10 = *(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      if (*(int *)(local_10 + 0x34) != -1) {
        if ((puVar4[4] & 1) != 0) {
          object_for_each_light_attachment(0,1);
        }
        if (*(int *)(local_10 + 0x34) != -1) {
          iVar11 = *(int *)(DAT_008603b0 + 0x34);
          puVar4[4] = puVar4[4] & 0xfffffffe;
          pbVar1 = (byte *)(iVar11 + iVar9 + 2);
          *pbVar1 = *pbVar1 | 2;
        }
      }
      *(undefined2 *)(puVar3 + 0xbc) = 0xffff;
      *(undefined1 *)((int)puVar3 + 0x2a7) = 2;
      if (local_8[0xc9] == param_1) {
        local_8[0xc9] = 0xffffffff;
      }
      if (local_8[0xca] == param_1) {
        local_8[0xca] = 0xffffffff;
      }
      FUN_0056ce30();
      FUN_0056d6a0();
      local_8 = (uint *)(uint)CONCAT12(0x14,(undefined2)local_8);
      FUN_00565420(param_1);
      puVar2 = (undefined4 *)(*(short *)((int)puVar3 + 0x1ea) + 0x10 + (int)puVar3);
      *puVar2 = local_1c;
      puVar2[1] = local_18;
      puVar2[2] = local_14;
      if ((short)puVar3[0x2d] == 0) {
        FUN_0055add0(param_1);
      }
      object_recalculate_bounding_radius_recursive(param_1);
      cVar6 = FUN_00566910();
      if ((cVar6 == '\x01') && (iVar11 = object_try_and_get(2), iVar11 != 0)) {
        *(undefined4 *)(iVar11 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
      }
      iVar10 = DAT_0087a480;
      if (((DAT_00719720 == 1) && (iVar11 = datum_get(), iVar11 != 0)) &&
         (*(short *)(iVar11 + 2) == -1)) {
        *(undefined4 *)(iVar11 + 0x180) = 0;
        *(undefined4 *)(iVar11 + 0x17c) = 0;
        *(undefined4 *)(iVar11 + 0x1e0) = 0;
        *(undefined4 *)(iVar11 + 0x1dc) = 0;
      }
    }
    if (((param_2 == '\0') && (param_4 == '\x01')) && (puVar3[1] == 0)) {
      FUN_0056c370(1);
      iVar10 = DAT_0087a480;
    }
    if (((DAT_00719720 == 1) && (uVar5 = puVar3[0x86], uVar5 != 0xffffffff)) &&
       ((sVar8 = (short)uVar5, -1 < sVar8 && (sVar8 < *(short *)(iVar10 + 0x20))))) {
      iVar11 = (int)*(short *)(iVar10 + 0x22) * (int)sVar8;
      sVar8 = *(short *)(iVar11 + *(int *)(iVar10 + 0x34));
      if (((sVar8 != 0) && ((sVar7 = (short)(uVar5 >> 0x10), sVar7 == 0 || (sVar8 == sVar7)))) &&
         ((*(short *)(iVar11 + *(int *)(iVar10 + 0x34) + 2) != -1 && (DAT_0071c2d8 != 0)))) {
        player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
      }
    }
  }
  return;
}
#endif
