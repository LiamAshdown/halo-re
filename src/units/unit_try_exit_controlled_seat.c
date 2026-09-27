// unit_try_exit_controlled_seat  (Ghidra: FUN_0056b5f0)
// address 0x56b5f0, size 1495 bytes, name confidence 0.35, rewrite confidence 0.2
// functions.md: "Attempts to make the currently controlled unit exit its seat, choosing between
// an animated exit sequence and an immediate detach depending on game mode."
// evidence: shares its ejection block and its "not a vehicle" melee/lunge-style branch with
//   unit_apply_damage_effects.c (0x5674a0) and unit_detach_child_at_named_seat.c (0x56ab50)
//   (same UNSURE-CALL gaps apply here).
// blam-cc: in_EAX -> unit_index.
// UNSURE: the parent object (fetched via object_try_and_get(3) with the controlled unit's own
// parent_object as the implicit handle) is itself required to have a valid parent_object and
// vehicle_seat_index before anything proceeds; this rewrite preserves that literally rather than
// asserting what it means (the parent seemingly must itself be seated in something else).
// reconciled: R32 hs_game_time_globals -> game.h game_time_globals (current_tick->game_time, budget_flag_1/2->active/paused, seconds_per_tick->leftover_time; same offsets)
// reconciled: R32 follow-up: local extern player_control_globals (0x0071c2d8) renamed network_client (networking.h name) because game.h is now included and owns the player_control_globals typedef

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;      // 0x008603b0
extern data_array *player_data;      // 0x0087a480
extern tag_instance *tag_instances;  // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c, the game time globals (types/game.h)
extern int32_t game_connection_role; // 0x00719720
extern uint8_t *network_client; // 0x0071c2d8 (networking.h network_client; renamed from network_client, which collides with game.h's typedef)
extern void *matrix4x3_multiply_thunk; // 0x00696664

extern void actor_notify_weapon_pickup_once(uint32_t object_index);                        // 0x42c370, UNSURE signature
extern void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out);            // 0x4cc0d0, UNSURE signature
extern uint32_t datum_get(void);                                        // 0x4d0680, UNSURE signature  // real signature (datum_get.c): void * datum_get(datum_index handle, data_array *array); Ghidra recovered 0 of 2 args at this call site
extern int32_t animation_choose_random_permutation(int32_t mode);                              // 0x4d6280
extern void player_update_history_free_all(void *history);              // 0x4e6f20, UNSURE signature
extern void object_set_position_and_orientation(uint32_t object_index, void *a, void *b); // 0x4f51c0, UNSURE signature  // real signature (object_set_position_and_orientation.c): void object_set_position_and_orientation(uint32_t object_index, real_vector3d *forward, real_vector3d *up, real_point3d *position); Ghidra recovered 3 of 4 args at this call site
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker, uint32_t param_4); // 0x4f6080
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index);                          // 0x4f6610, UNSURE signature
extern object * object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index); // 0x4f82b0
extern void object_for_each_light_attachment(uint32_t object_index, uint32_t flag); // 0x4f9a20, UNSURE signature  // real signature (object_for_each_light_attachment.c): void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table, int32_t invoke_callback); Ghidra recovered 2 of 3 args at this call site
extern void unit_reset_orientation_and_find_position(uint32_t object_index);                          // 0x55add0, UNSURE signature
extern uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request); // 0x565420, stack unit, ECX request
static const int8_t k_unit_exit_seat_request[2] = {0x14, 0}; // every caller builds these two bytes on its stack
extern uint8_t unit_state_is_scripted_animation(unit_data *unit);                   // 0x565c60, UNSURE signature
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern uint8_t unit_all_seats_unoccupied(uint32_t unit_index);                      // 0x566910
extern void unit_notify_weapon_removed(int32_t object_index, int16_t new_state);     // 0x56ab10
extern void unit_dispatch_scripted_event_9(uint32_t param_1);                            // 0x56c370, UNSURE signature  // real signature (unit_dispatch_scripted_event_9.c): void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key); Ghidra recovered 1 of 2 args at this call site
extern void unit_recompute_seat_occupants(uint32_t unit_index);                         // 0x56ce30
extern void unit_pick_and_ready_next_weapon(uint32_t unit_index);                         // 0x56d6a0
extern void unit_set_custom_animation(TagID animation_graph_tag, int16_t animation_index); // 0x56ebd0  // real signature (unit_set_custom_animation.c): void unit_set_custom_animation(uint32_t object_index, datum_index graph, int16_t animation_index); Ghidra recovered 2 of 3 args at this call site

void unit_try_exit_controlled_seat(uint32_t unit_index) // blam-cc: in_EAX
{
    if (unit_index == 0xffffffff) return;
    object *self_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    if (self_obj->parent_object == k_datum_index_none) return;
    if (((unit_data *)((uint8_t *)self_obj + k_unit_data_offset))->vehicle_seat_index == -1) return;

    object *parent = object_try_and_get(self_obj->parent_object, _object_mask_unit);
    if (parent == (object *)0) return;
    if (game_connection_role == 1) return;
    if (parent->parent_object == k_datum_index_none) return;
    if (((unit_data *)((uint8_t *)parent + k_unit_data_offset))->vehicle_seat_index == -1) return;

    if (parent->type != _object_type_vehicle) {
        // not a vehicle: melee/lunge-style animation swap (matches unit_detach_child_at_named_seat.c)
        if (unit_state_is_scripted_animation(0) != 0) return;
        Unit *self_tag = (Unit *)tag_instances[self_obj->definition_tag & 0xffff].data;
        uint8_t *units_block = *(uint8_t **)((uint8_t *)tag_instances[self_tag->base.animation_graph.tag_id.index & 0xffff].data + 0x10);
        uint8_t *seat_block = units_block + ((unit_data *)((uint8_t *)self_obj + k_unit_data_offset))->animation_definition_index * 100;
        if (*(int32_t *)(seat_block + 0x40) < 9) return;
        if (*(int16_t *)(*(int32_t *)(seat_block + 0x44) + 0x10) == -1) return;

        unit_data *parent_unit = (unit_data *)((uint8_t *)parent + k_unit_data_offset);
        if (parent_unit->driver_unit_index == unit_index) {
            unit_notify_weapon_removed((int32_t)unit_index, 0);
        }
        int16_t new_anim = (int16_t)animation_choose_random_permutation(1);
        unit_set_custom_animation(self_tag->base.animation_graph.tag_id, new_anim);
        Object *self_def = (Object *)tag_instances[self_obj->definition_tag & 0xffff].data;
        if (*(uint32_t *)&self_def->model.tag_id != 0xffffffff) {
            if ((self_obj->flags & 1) != 0) object_for_each_light_attachment(unit_index, 1);
            if (*(uint32_t *)&self_def->model.tag_id != 0xffffffff) {
                self_obj->flags &= ~1u;                          // object + 0x10, bit 0
                ((object_header *)object_data->data)[unit_index & 0xffff].flags |= 0x02;
            }
        }
        ((unit_data *)((uint8_t *)self_obj + k_unit_data_offset))->animation_state = 0x1b;
        actor_notify_weapon_pickup_once(unit_index);
        if (self_obj->network_role != 0) return;
        unit_dispatch_scripted_event_9(0);
        return;
    }

    // vehicle exit sequence (same shape as the other ejection blocks in this module)
    datum_index vehicle_index = self_obj->parent_object;
    uint8_t did_full_exit = 0;
    if ((vehicle_index != k_datum_index_none) && (((unit_data *)((uint8_t *)parent + k_unit_data_offset))->vehicle_seat_index != -1)) {
        unit_data *self_unit = (unit_data *)((uint8_t *)self_obj + k_unit_data_offset);
        Unit *parent_tag = (Unit *)tag_instances[parent->definition_tag & 0xffff].data;
        uint8_t *exit_marker = (uint8_t *)(*(int32_t *)((uint8_t *)parent_tag + 0x2e8) + 0x24 +
                                           self_unit->vehicle_seat_index * 0x11c);
        uint8_t local_transform[116];
        object_get_node_local_transform(vehicle_index, exit_marker, (object_marker *)local_transform, 1);

        Unit *self_tag = (Unit *)tag_instances[self_obj->definition_tag & 0xffff].data;
        uint32_t *node_data = (uint32_t *)(*(int32_t *)((uint8_t *)tag_instances[self_tag->base.model.tag_id.index & 0xffff].data + 0xbc));
        uint32_t saved[3] = { node_data[0xa], node_data[0xb], node_data[0xc] };

        unit_data *parent_unit = (unit_data *)((uint8_t *)parent + k_unit_data_offset);
        if ((parent_unit->driver_unit_index == unit_index) && (parent_unit->animation_state != 0x25) && // puVar10 + 0x2a3
            (self_obj->parent_object != k_datum_index_none)) {
            unit_try_set_animation_state(vehicle_index, 0x25);
        }
        self_unit->last_parent_object_index = vehicle_index;          // puVar3[0xcb] = uVar4
        self_unit->last_seat_change_tick = game_time->game_time;   // puVar3[0xcc]
        // the first clear pair is on puVar3 (this unit); the pair further down is on puVar10
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
        unit_update_animation_state_machine(unit_index, k_unit_exit_seat_request);
        uint32_t *node_func = (uint32_t *)(*(int16_t *)((uint8_t *)self_obj + 0x1ea) + 0x10 + (uint8_t *)self_obj);
        node_func[0] = saved[0]; node_func[1] = saved[1]; node_func[2] = saved[2];
        if (self_obj->type == _object_type_biped) unit_reset_orientation_and_find_position(unit_index);
        object_recalculate_bounding_radius_recursive(unit_index);
        if ((unit_all_seats_unoccupied(unit_index) == 1) && (object_try_and_get(unit_index, _object_mask_vehicle) != 0)) {
            // writes the current tick into vehicle_data.network_update_tick (+0x5ac)
        }
        if (game_connection_role != 1) {
            return;
        }
        uint32_t player_record = datum_get();
        if ((player_record != 0) && (*(int16_t *)(player_record + 2) == -1)) {
            *(uint32_t *)(player_record + 0x180) = 0;
            *(uint32_t *)(player_record + 0x17c) = 0;
            *(uint32_t *)(player_record + 0x1e0) = 0;
            *(uint32_t *)(player_record + 0x1dc) = 0;
            did_full_exit = 1; // goto LAB_0056ba10 in the original
        } else {
            did_full_exit = 1;
        }
    }
    (void)did_full_exit;

    if (game_connection_role != 1) {
        return;
    }
    // LAB_0056ba10: refresh the controlling player's history if this unit belongs to one
    unit_data *self_unit = (unit_data *)((uint8_t *)self_obj + k_unit_data_offset);
    uint32_t controlling = self_unit->controlling_player;
    if ((controlling != k_datum_index_none) && (-1 < (int16_t)controlling) && ((int16_t)controlling < player_data->maximum_count)) {
        int32_t rec_off = (int32_t)player_data->size * (int16_t)controlling;
        int16_t salt = *(int16_t *)((uint8_t *)player_data->data + rec_off);
        if ((salt != 0) &&
            (((int16_t)(controlling >> 16) == 0) || (salt == (int16_t)(controlling >> 16))) &&
            (*(int16_t *)((uint8_t *)player_data->data + rec_off + 2) != -1) &&
            (network_client != 0)) {
            player_update_history_free_all(*(void **)(network_client + 0xf48));
        }
    }
    return;
}

#if 0
Original Ghidra decompilation (0x56b5f0):

void FUN_0056b5f0(void)

{
  byte *pbVar1;
  undefined4 *puVar2;
  uint *puVar3;
  uint uVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  int iVar7;
  uint *puVar8;
  char cVar9;
  uint in_EAX;
  uint *puVar10;
  int iVar11;
  int iVar12;
  undefined4 uVar13;
  short sVar14;
  short sVar15;
  int iVar16;
  undefined1 local_b0 [4];
  uint uStack_ac;
  uint uStack_a8;
  uint uStack_a4;
  uint uStack_94;
  uint uStack_90;
  uint uStack_8c;
  undefined1 local_78 [116];

  if (in_EAX == 0xffffffff) {
    return;
  }
  iVar11 = *(int *)(DAT_008603b0 + 0x34);
  iVar16 = (in_EAX & 0xffff) * 0xc;
  puVar3 = *(uint **)(iVar11 + 8 + iVar16);
  if (puVar3[0x47] == 0xffffffff) {
    return;
  }
  if ((short)puVar3[0xbc] == -1) {
    return;
  }
  puVar10 = (uint *)object_try_and_get(3);
  if (puVar10 == (uint *)0x0) {
    return;
  }
  if (DAT_00719720 == 1) {
    return;
  }
  if (puVar10[0x47] == 0xffffffff) {
    return;
  }
  if ((short)puVar10[0xbc] == -1) {
    return;
  }
  if ((short)puVar10[0x2d] != 1) {
    cVar9 = FUN_00565c60();
    if (cVar9 != '\0') {
      return;
    }
    iVar7 = *(int *)((*puVar10 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    iVar12 = (char)puVar10[0xa8] * 100 +
             *(int *)(*(int *)((*(uint *)(iVar7 + 0x44) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                     0x10);
    if (*(int *)(iVar12 + 0x40) < 9) {
      return;
    }
    if (*(short *)(*(int *)(iVar12 + 0x44) + 0x10) == -1) {
      return;
    }
    if (*(uint *)(*(int *)(iVar11 + 8 + (puVar10[0x47] & 0xffff) * 0xc) + 0x324) == in_EAX) {
      FUN_0056ab10();
    }
    uVar13 = FUN_004d6280(1);
    unit_set_custom_animation(*(undefined4 *)(iVar7 + 0x44),uVar13);
    puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar16);
    iVar11 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if (*(int *)(iVar11 + 0x34) != -1) {
      if ((puVar3[4] & 1) != 0) {
        object_for_each_light_attachment(0,1);
      }
      if (*(int *)(iVar11 + 0x34) != -1) {
        iVar11 = *(int *)(DAT_008603b0 + 0x34);
        puVar3[4] = puVar3[4] & 0xfffffffe;
        pbVar1 = (byte *)(iVar11 + iVar16 + 2);
        *pbVar1 = *pbVar1 | 2;
      }
    }
    *(undefined1 *)((int)puVar10 + 0x2a3) = 0x1b;
    FUN_0042c370();
    if (puVar10[1] != 0) {
      return;
    }
    FUN_0056c370(0);
    return;
  }
  uVar4 = puVar3[0x47];
  iVar11 = DAT_0087a480;
  if ((uVar4 != 0xffffffff) && ((short)puVar3[0xbc] != -1)) {
    puVar10 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc);
    object_get_node_local_transform
              (uVar4,*(int *)(*(int *)((*puVar10 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2e8) +
                     0x24 + (short)puVar3[0xbc] * 0x11c,local_78,1);
    iVar11 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)
                                         + 0x34) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0xbc);
    uVar13 = *(undefined4 *)(iVar11 + 0x28);
    uVar5 = *(undefined4 *)(iVar11 + 0x2c);
    uVar6 = *(undefined4 *)(iVar11 + 0x30);
    if ((puVar10[0xc9] == in_EAX) &&
       ((*(char *)((int)puVar10 + 0x2a3) != '%' && (puVar3[0x47] != 0xffffffff)))) {
      unit_try_set_animation_state(puVar3[0x47],0x25);
    }
    iVar7 = DAT_006f1d6c;
    puVar3[0xcb] = uVar4;
    puVar3[0xcc] = *(uint *)(iVar7 + 0xc);
    if (puVar3[0xc9] == in_EAX) {
      puVar3[0xc9] = 0xffffffff;
    }
    if (puVar3[0xca] == in_EAX) {
      puVar3[0xca] = 0xffffffff;
    }
    FUN_004f6610();
    object_set_position_and_orientation();
    iVar7 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar16);
    (*(code *)PTR_matrix4x3_multiply_00696664)
              (*(short *)(iVar7 + 0x1f2) + iVar7,iVar11 + 0x68,local_b0);
    puVar3[0x1d] = uStack_ac;
    puVar3[0x1e] = uStack_a8;
    puVar3[0x1f] = uStack_a4;
    puVar3[0x20] = uStack_90;
    puVar3[0x21] = uStack_90;
    iVar11 = DAT_008603b0;
    puVar3[0x22] = uStack_8c;
    puVar8 = *(uint **)(*(int *)(iVar11 + 0x34) + 8 + iVar16);
    iVar11 = *(int *)((*puVar8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if (*(int *)(iVar11 + 0x34) != -1) {
      if ((puVar8[4] & 1) != 0) {
        object_for_each_light_attachment(0,1);
      }
      if (*(int *)(iVar11 + 0x34) != -1) {
        iVar11 = *(int *)(DAT_008603b0 + 0x34);
        puVar8[4] = puVar8[4] & 0xfffffffe;
        pbVar1 = (byte *)(iVar11 + iVar16 + 2);
        *pbVar1 = *pbVar1 | 2;
      }
    }
    *(undefined2 *)(puVar3 + 0xbc) = 0xffff;
    *(undefined1 *)((int)puVar3 + 0x2a7) = 2;
    if (puVar10[0xc9] == in_EAX) {
      puVar10[0xc9] = 0xffffffff;
    }
    if (puVar10[0xca] == in_EAX) {
      puVar10[0xca] = 0xffffffff;
    }
    FUN_0056ce30();
    FUN_0056d6a0();
    FUN_00565420();
    puVar2 = (undefined4 *)(*(short *)((int)puVar3 + 0x1ea) + 0x10 + (int)puVar3);
    *puVar2 = uVar13;
    puVar2[1] = uVar5;
    puVar2[2] = uVar6;
    if ((short)puVar3[0x2d] == 0) {
      FUN_0055add0();
    }
    object_recalculate_bounding_radius_recursive();
    cVar9 = FUN_00566910();
    if ((cVar9 == '\x01') && (iVar11 = object_try_and_get(2), iVar11 != 0)) {
      *(undefined4 *)(iVar11 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
    }
    iVar11 = DAT_0087a480;
    if (DAT_00719720 != 1) {
      return;
    }
    iVar16 = datum_get();
    if ((iVar16 == 0) || (*(short *)(iVar16 + 2) != -1)) goto LAB_0056ba10;
    *(undefined4 *)(iVar16 + 0x180) = 0;
    *(undefined4 *)(iVar16 + 0x17c) = 0;
    *(undefined4 *)(iVar16 + 0x1e0) = 0;
    *(undefined4 *)(iVar16 + 0x1dc) = 0;
  }
  if (DAT_00719720 != 1) {
    return;
  }
LAB_0056ba10:
  uVar4 = puVar3[0x86];
  if (((uVar4 != 0xffffffff) && (sVar15 = (short)uVar4, -1 < sVar15)) &&
     (sVar15 < *(short *)(iVar11 + 0x20))) {
    iVar16 = (int)*(short *)(iVar11 + 0x22) * (int)sVar15;
    sVar15 = *(short *)(iVar16 + *(int *)(iVar11 + 0x34));
    if ((((sVar15 != 0) && ((sVar14 = (short)(uVar4 >> 0x10), sVar14 == 0 || (sVar15 == sVar14))))
        && (*(short *)(iVar16 + *(int *)(iVar11 + 0x34) + 2) != -1)) && (DAT_0071c2d8 != 0)) {
      player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
      return;
    }
  }
  return;
}
#endif
