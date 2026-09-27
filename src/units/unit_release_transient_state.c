// unit_release_transient_state  (Ghidra: FUN_00568610)
// address 0x568610, size 1696 bytes
// name confidence: 0.35 (functions.md: "Releases a unit's transient sound/animation handles
//   and, when seated, performs additional vehicle-seat transition cleanup")
// rewrite confidence: 0.2
// evidence: types/units.h unit_data.actor_index/swarm_actor_index (0x1f4/0x1f8, "0x568610
//   clears both"), .controlling_player (0x218), .unknown_336 (0x336, "copied from UnitSeat +0x3a
//   ... 0x568610, 0x568cb0"), .flags (0x204, bit 0x2000 feign-death eligibility);
//   types/tags.h Unit.feign_repeat_chance (0x248).
// register convention: unit index in EAX/param_1, a flag in param_2.
//   // blam-cc: param_1 -> unit_index, param_2 -> is_light_reset (UNSURE name)
// UNSURE: this function shares almost all of its vehicle-seat-exit sequence with the ejection
//   branch inside unit_apply_damage_effects (0x5674a0) -- same node-transform math, same
//   0xc9/0xca/0x1ea/0x2e8/0x11c-stride offsets -- and carries the exact same recovery gaps; see
//   that file's header for what could and could not be reconstructed. Every UNSURE-CALL here
//   mirrors one there.
// UNSURE: several raw offsets on the parent/vehicle object (`0x17`,`0x18`,`0x19`,`0xbc`) are not
//   named by this module's header.
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
#include <stdint.h>

extern data_array *object_data;      // 0x008603b0
extern data_array *actor_data;       // 0x00880360
extern data_array *player_data;      // 0x0087a480
extern tag_instance *tag_instances;  // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c, the game time globals (types/game.h)
extern int32_t game_connection_role; // 0x00719720
extern uint8_t *network_client; // 0x0071c2d8 (networking.h network_client; renamed from network_client, which collides with game.h's typedef)
extern random_seed random_seed_global;    // 0x00719cd0
extern void *matrix4x3_multiply_thunk; // 0x00696664

extern void actor_attempt_grenade_throw(uint32_t actor_index);          // 0x428ab0
extern void actor_release_from_cluster_or_delete(uint32_t unit_index);                          // 0x428e50, UNSURE signature
extern void player_reset_after_unit_change(uint32_t controlling_player);                  // 0x474e10, UNSURE signature
extern void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out);            // 0x4cc0d0, UNSURE signature
extern float transition_function_evaluate(int32_t param_1);             // 0x4ccac0, UNSURE signature  // real signature (transition_function_evaluate.c): real transition_function_evaluate(transition_function_t type, real phase); Ghidra recovered 1 of 2 args at this call site
extern uint32_t datum_get(void);                                        // 0x4d0680, UNSURE signature  // real signature (datum_get.c): void * datum_get(datum_index handle, data_array *array); Ghidra recovered 0 of 2 args at this call site
extern void player_update_history_free_all(void *history);              // 0x4e6f20, UNSURE signature
extern void object_set_position_and_orientation(uint32_t object_index, void *a, void *b); // 0x4f51c0, UNSURE signature  // real signature (object_set_position_and_orientation.c): void object_set_position_and_orientation(uint32_t object_index, real_vector3d *forward, real_vector3d *up, real_point3d *position); Ghidra recovered 3 of 4 args at this call site
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker, uint32_t param_4); // 0x4f6080
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index);                          // 0x4f6610, UNSURE signature
extern object * object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void object_list_membership_set(uint32_t flag);                                // 0x4f7450, UNSURE signature  // real signature (object_list_membership_set.c): void object_list_membership_set(uint32_t object_index, char add); Ghidra recovered 1 of 2 args at this call site
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index); // 0x4f82b0
extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table, int32_t invoke_callback); // 0x4f9a20, EAX object, stack (register_in_table, invoke_callback)
extern void unit_reset_orientation_and_find_position(uint32_t object_index);                          // 0x55add0, UNSURE signature
extern uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request); // 0x565420, stack unit, ECX request
static const int8_t k_unit_exit_seat_request[2] = {0x14, 0}; // every caller builds these two bytes on its stack
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern uint8_t unit_all_seats_unoccupied(uint32_t unit_index);                      // 0x566910
extern void unit_detach_reposition_and_nudge(uint32_t unit_index);                         // 0x56ca40, UNSURE signature
extern void unit_recompute_seat_occupants(uint32_t unit_index);                         // 0x56ce30
extern void unit_pick_and_ready_next_weapon(uint32_t unit_index);                         // 0x56d6a0
extern uint8_t unit_drop_current_weapon(uint32_t unit_index, uint8_t force); // 0x56dec0
extern void unit_drop_object_from_hand(uint32_t unit_index, uint32_t dropped_object_index); // 0x56ed00
extern void unit_drop_grenades(uint32_t unit_index);                   // 0x56ef60
extern void unit_drop_inventory_weapons(uint32_t unit_index);          // 0x56f060

void unit_release_transient_state(uint32_t unit_index, uint8_t is_light_reset) // blam-cc: param_1, param_2
{
    object *self_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)self_obj + k_unit_data_offset);
    int32_t self_offset = (unit_index & 0xffff) * 0xc;

    if (!is_light_reset) {
        unit->unknown_420 = 0;
        object_list_membership_set(1);
        if (unit->controlling_player != k_datum_index_none) {
            player_reset_after_unit_change(unit->controlling_player);
            unit->controlling_player = k_datum_index_none;
        }
        if (unit->actor_index != k_datum_index_none) {
            uint8_t *actor_rec = (uint8_t *)actor_data->data + (unit->actor_index & 0xffff) * 0x724;
            *(int16_t *)((uint8_t *)unit + 0x334) = *(int16_t *)(actor_rec + 0x34);
            *(int16_t *)((uint8_t *)self_obj + 0x336) = *(int16_t *)(actor_rec + 0x3a);
            actor_attempt_grenade_throw(unit->actor_index);
            unit->actor_index = k_datum_index_none;
        }
        if (unit->swarm_actor_index != k_datum_index_none) {
            uint8_t *actor_rec = (uint8_t *)actor_data->data + (unit->swarm_actor_index & 0xffff) * 0x724;
            *(int16_t *)((uint8_t *)unit + 0x334) = *(int16_t *)(actor_rec + 0x34);
            *(int16_t *)((uint8_t *)self_obj + 0x336) = *(int16_t *)(actor_rec + 0x3a);
            actor_release_from_cluster_or_delete(unit_index);
            unit->swarm_actor_index = k_datum_index_none;
        }
        unit->unknown_41c = game_time->game_time;
    } else {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        Unit *unit_tag = (Unit *)tag_instances[self_obj->definition_tag & 0xffff].data;
        if (unit_tag->feign_repeat_chance <= (float)(random_seed_global >> 16) * 1.5259022e-05f) {
            unit->flags &= 0xffffdfff;
        } else {
            unit->flags |= 0x2000;
        }
    }

    uint32_t weapon_object_index = 0xffffffff;
    unit->flags &= 0xffffffee;
    unit->control_flags = 0;
    if (unit->current_weapon_index != -1) {
        int16_t slot = unit->current_weapon_index;
        if (slot != -1) {
            weapon_object_index = unit->weapons[slot];
        }
        object *weapon_obj = ((object_header *)object_data->data)[weapon_object_index & 0xffff].data;
        *(int16_t *)((uint8_t *)weapon_obj + 0x230) = 0;
        *(float *)((uint8_t *)weapon_obj + 0x234) = transition_function_evaluate(0);
    }
    unit->flags &= 0xfdffffff; // clears _unit_flag_idle_turn_seeded (0x02000000)

    if (self_obj->parent_object == k_datum_index_none) {
        goto drop_inventory;
    }
    if (unit->vehicle_seat_index == -1) {
        unit_detach_reposition_and_nudge(unit_index);
        goto drop_inventory;
    }
    if (game_connection_role == 1) {
        goto drop_inventory;
    }
    {
        // Ghidra's puVar2 is this unit's own object and puVar11 is the vehicle it is seated in;
        // local_c is puVar2[0x47], i.e. this unit's object.parent_object.
        datum_index vehicle_index = self_obj->parent_object;              // local_c
        object *vehicle;                                                  // puVar11
        unit_data *vehicle_unit;
seat_reenter:
        if ((vehicle_index == k_datum_index_none) || (unit->vehicle_seat_index == -1)) { // puVar2[0xbc]
            if (game_connection_role != 1) {
                goto drop_inventory;
            }
        } else {
            vehicle = ((object_header *)object_data->data)[vehicle_index & 0xffff].data;
            vehicle_unit = (unit_data *)((uint8_t *)vehicle + k_unit_data_offset);
            Unit *vehicle_tag = (Unit *)tag_instances[vehicle->definition_tag & 0xffff].data;
            uint8_t *exit_marker = (uint8_t *)(*(int32_t *)((uint8_t *)vehicle_tag + 0x2e8) + 0x24 +
                                               unit->vehicle_seat_index * 0x11c);
            uint8_t local_transform[96]; // local_dc
            object_get_node_local_transform(vehicle_index, exit_marker, (object_marker *)local_transform, 1);

            // iVar10: the node block the original snapshots here, out of this unit's own
            // Object.model.tag_id (tag + 0x34), then + 0xbc.
            Object *self_def0 = (Object *)tag_instances[self_obj->definition_tag & 0xffff].data;
            uint32_t *node_block = (uint32_t *)(*(int32_t *)((uint8_t *)tag_instances[
                self_def0->model.tag_id.index & 0xffff].data + 0xbc));
            uint32_t saved[3];
            saved[0] = node_block[0xa];   // local_1c = iVar10 + 0x28
            saved[1] = node_block[0xb];   // local_18 = iVar10 + 0x2c
            saved[2] = node_block[0xc];   // local_14 = iVar10 + 0x30

            if ((vehicle_unit->driver_unit_index == unit_index) &&
                (vehicle_unit->animation_state != 0x25) && (self_obj->parent_object != k_datum_index_none)) {
                unit_try_set_animation_state(self_obj->parent_object, 0x25);
            }
            unit->last_parent_object_index = vehicle_index;           // puVar2[0xcb] = local_c
            unit->last_seat_change_tick = game_time->game_time;    // puVar2[0xcc]
            // the first clear pair is on puVar2 (this unit); the pair further down is on puVar11
            if (unit->driver_unit_index == unit_index) {
                unit->driver_unit_index = k_datum_index_none;
            }
            if (unit->gunner_unit_index == unit_index) {
                unit->gunner_unit_index = k_datum_index_none;
            }
            object_snap_to_parent_marker_and_detach(unit_index);
            object_set_position_and_orientation(unit_index, 0, 0); // UNSURE-CALL: transform args dropped
            matrix4x3_multiply(0, 0, 0);                           // UNSURE-CALL: operands dropped

            Object *self_def = (Object *)tag_instances[self_obj->definition_tag & 0xffff].data;
            if (*(uint32_t *)&self_def->model.tag_id != 0xffffffff) {
                if ((self_obj->flags & 1) != 0) {
                    object_for_each_light_attachment(unit_index, 0, 1);
                }
                if (*(uint32_t *)&self_def->model.tag_id != 0xffffffff) {
                    self_obj->flags &= ~1u;                          // object + 0x10, bit 0
                    ((object_header *)object_data->data)[unit_index & 0xffff].flags |= 0x02;
                }
            }
            unit->vehicle_seat_index = -1;                            // puVar2[0xbc]
            unit->base_animation_state = _unit_base_animation_state_stand;
            if (vehicle_unit->driver_unit_index == unit_index) {
                vehicle_unit->driver_unit_index = k_datum_index_none;
            }
            if (vehicle_unit->gunner_unit_index == unit_index) {
                vehicle_unit->gunner_unit_index = k_datum_index_none;
            }
            unit_recompute_seat_occupants(unit_index);
            unit_pick_and_ready_next_weapon(unit_index);
            unit_update_animation_state_machine(unit_index, k_unit_exit_seat_request);
            {
                uint32_t *node_func = (uint32_t *)(*(int16_t *)((uint8_t *)self_obj + 0x1ea) + 0x10 + (uint8_t *)self_obj);
                node_func[0] = saved[0];
                node_func[1] = saved[1];
                node_func[2] = saved[2];
            }
            if (self_obj->type == _object_type_biped) {
                unit_reset_orientation_and_find_position(unit_index);
            }
            object_recalculate_bounding_radius_recursive(unit_index);
            if ((unit_all_seats_unoccupied(unit_index) == 1) && (object_try_and_get(unit_index, _object_mask_vehicle) != 0)) {
                // writes the current tick into vehicle_data.network_update_tick (+0x5ac)
            }
            if (game_connection_role != 1) {
                goto drop_inventory;
            }
            uint32_t player_record = datum_get();
            if ((player_record != 0) && (*(int16_t *)(player_record + 2) == -1)) {
                *(uint32_t *)(player_record + 0x180) = 0;
                *(uint32_t *)(player_record + 0x17c) = 0;
                *(uint32_t *)(player_record + 0x1e0) = 0;
                *(uint32_t *)(player_record + 0x1dc) = 0;
                goto seat_reenter;
            }
        }
        uint32_t controlling = unit->controlling_player;          // puVar2[0x86]
        if ((controlling != k_datum_index_none) && (-1 < (int16_t)controlling) &&
            ((int16_t)controlling < player_data->maximum_count)) {
            int32_t rec_off = (int32_t)player_data->size * (int16_t)controlling;
            int16_t salt = *(int16_t *)((uint8_t *)player_data->data + rec_off);
            if ((salt != 0) &&
                (((int16_t)(controlling >> 16) == 0) || (salt == (int16_t)(controlling >> 16))) &&
                (*(int16_t *)((uint8_t *)player_data->data + rec_off + 2) != -1) &&
                (network_client != 0)) {
                player_update_history_free_all(*(void **)(network_client + 0xf48));
            }
        }
    }

drop_inventory:
    unit->pending_speech.priority = 0;
    unit_drop_inventory_weapons(unit_index);
    if (unit->equipment_object_index != k_datum_index_none) {
        unit_drop_object_from_hand(unit_index, unit->equipment_object_index);
        unit->equipment_object_index = k_datum_index_none;
    }
    unit_drop_grenades(unit_index);
    if (unit->unknown_28c == 0) { // (char)puVar11[0xa3]
        unit_drop_current_weapon(unit_index, 1);
    }
    unit->overlays[1].animation_index = -1;
    unit->overlays[0].animation_index = -1;
    unit->melee_state = 0;
    if (unit->throwing_grenade_state == 1) {
        unit->throwing_grenade_state = 0;
    }
    return;
}

#if 0
Original Ghidra decompilation (0x568610):

void FUN_00568610(uint param_1,char param_2)

{
  byte *pbVar1;
  uint *puVar2;
  undefined4 *puVar3;
  uint *puVar4;
  char cVar5;
  int iVar6;
  uint uVar7;
  short sVar8;
  short sVar9;
  int iVar10;
  uint *puVar11;
  int iVar12;
  float10 fVar13;
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
  uint *local_10;
  uint local_c;
  int local_8;

  iVar12 = DAT_008603b0;
  iVar10 = (param_1 & 0xffff) * 0xc;
  puVar11 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar10);
  local_10 = puVar11;
  local_8 = iVar10;
  if (param_2 == '\0') {
    *(undefined2 *)(puVar11 + 0x108) = 0;
    FUN_004f7450(1);
    if (puVar11[0x86] != 0xffffffff) {
      FUN_00474e10(puVar11[0x86]);
      iVar12 = DAT_008603b0;
      puVar11[0x86] = 0xffffffff;
    }
    uVar7 = puVar11[0x7d];
    if (uVar7 != 0xffffffff) {
      iVar12 = *(int *)(DAT_00880360 + 0x34);
      iVar6 = (uVar7 & 0xffff) * 0x724;
      *(undefined2 *)(puVar11 + 0xcd) = *(undefined2 *)(iVar6 + 0x34 + iVar12);
      *(undefined2 *)((int)puVar11 + 0x336) = *(undefined2 *)(iVar6 + iVar12 + 0x3a);
      actor_attempt_grenade_throw(uVar7);
      iVar12 = DAT_008603b0;
      puVar11[0x7d] = 0xffffffff;
    }
    if (puVar11[0x7e] != 0xffffffff) {
      iVar12 = *(int *)(DAT_00880360 + 0x34);
      iVar6 = (puVar11[0x7e] & 0xffff) * 0x724;
      *(undefined2 *)(puVar11 + 0xcd) = *(undefined2 *)(iVar6 + 0x34 + iVar12);
      *(undefined2 *)((int)puVar11 + 0x336) = *(undefined2 *)(iVar6 + iVar12 + 0x3a);
      FUN_00428e50(param_1);
      iVar12 = DAT_008603b0;
      puVar11[0x7e] = 0xffffffff;
    }
    puVar11[0x107] = *(uint *)(DAT_006f1d6c + 0xc);
  }
  else {
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    if (*(float *)(*(int *)((*puVar11 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x248) <=
        (float)(random_seed_global >> 0x10) * 1.5259022e-05) {
      puVar11[0x81] = puVar11[0x81] & 0xffffdfff;
    }
    else {
      puVar11[0x81] = puVar11[0x81] | 0x2000;
    }
  }
  uVar7 = 0xffffffff;
  puVar11[0x81] = puVar11[0x81] & 0xffffffee;
  puVar11[0x82] = 0;
  if (*(short *)((int)puVar11 + 0x2f2) != -1) {
    iVar10 = *(int *)(iVar10 + 8 + *(int *)(iVar12 + 0x34));
    sVar9 = *(short *)(iVar10 + 0x2f2);
    if (sVar9 != -1) {
      uVar7 = *(uint *)(iVar10 + 0x2f8 + sVar9 * 4);
    }
    iVar10 = *(int *)(*(int *)(iVar12 + 0x34) + 8 + (uVar7 & 0xffff) * 0xc);
    *(undefined2 *)(iVar10 + 0x230) = 0;
    fVar13 = (float10)transition_function_evaluate(0);
    *(float *)(iVar10 + 0x234) = (float)fVar13;
    iVar10 = local_8;
  }
  puVar2 = (uint *)(*(int *)(iVar10 + 8 + *(int *)(iVar12 + 0x34)) + 0x204);
  *puVar2 = *puVar2 & 0xfdffffff;
  if (puVar11[0x47] == 0xffffffff) goto LAB_00568c1f;
  if ((short)puVar11[0xbc] == -1) {
    FUN_0056ca40();
    goto LAB_00568c1f;
  }
  if (DAT_00719720 == 1) goto LAB_00568c1f;
  iVar12 = *(int *)(iVar12 + 0x34);
  puVar2 = *(uint **)(iVar10 + 8 + iVar12);
  local_c = puVar2[0x47];
  iVar6 = DAT_0087a480;
  if ((local_c == 0xffffffff) || ((short)puVar2[0xbc] == -1)) {
LAB_00568ba2:
    iVar10 = local_8;
    if (DAT_00719720 != 1) goto LAB_00568c1f;
  }
  else {
    puVar11 = *(uint **)(iVar12 + 8 + (local_c & 0xffff) * 0xc);
    iVar12 = *(int *)(iVar12 + 8 + local_8);
    iVar12 = *(short *)(iVar12 + 0x1f2) + iVar12;
    object_get_node_local_transform
              (local_c,*(int *)(*(int *)((*puVar11 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x2e8)
                       + 0x24 + (short)puVar2[0xbc] * 0x11c,local_dc,1);
    local_28 = *(float *)(iVar12 + 0x28) - local_7c;
    local_24 = *(float *)(iVar12 + 0x2c) - local_78;
    iVar10 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)
                                         + 0x34) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0xbc);
    local_20 = *(float *)(iVar12 + 0x30) - local_74;
    local_1c = *(undefined4 *)(iVar10 + 0x28);
    local_18 = *(undefined4 *)(iVar10 + 0x2c);
    local_14 = *(float *)(iVar10 + 0x30);
    if ((puVar11[0xc9] == param_1) &&
       ((*(char *)((int)puVar11 + 0x2a3) != '%' && (puVar2[0x47] != 0xffffffff)))) {
      unit_try_set_animation_state(puVar2[0x47],0x25);
    }
    iVar12 = DAT_006f1d6c;
    puVar2[0xcb] = local_c;
    puVar2[0xcc] = *(uint *)(iVar12 + 0xc);
    if (puVar2[0xc9] == param_1) {
      puVar2[0xc9] = 0xffffffff;
    }
    if (puVar2[0xca] == param_1) {
      puVar2[0xca] = 0xffffffff;
    }
    FUN_004f6610(param_1);
    local_34 = local_28 + (float)puVar2[0x17];
    local_30 = local_24 + (float)puVar2[0x18];
    local_2c = (local_20 + (float)puVar2[0x19]) - local_14;
    object_set_position_and_orientation(param_1,0,0);
    iVar6 = local_8;
    iVar12 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_8);
    (*(code *)PTR_matrix4x3_multiply_00696664)
              (*(short *)(iVar12 + 0x1f2) + iVar12,iVar10 + 0x68,local_6c);
    puVar2[0x1d] = local_68;
    puVar2[0x1e] = local_64;
    puVar2[0x1f] = local_60;
    puVar2[0x20] = local_50;
    puVar2[0x21] = local_4c;
    iVar12 = DAT_008603b0;
    puVar2[0x22] = local_48;
    puVar4 = *(uint **)(*(int *)(iVar12 + 0x34) + 8 + iVar6);
    iVar12 = *(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if (*(int *)(iVar12 + 0x34) != -1) {
      if ((puVar4[4] & 1) != 0) {
        object_for_each_light_attachment(0,1);
      }
      if (*(int *)(iVar12 + 0x34) != -1) {
        iVar12 = *(int *)(DAT_008603b0 + 0x34);
        puVar4[4] = puVar4[4] & 0xfffffffe;
        pbVar1 = (byte *)(iVar12 + local_8 + 2);
        *pbVar1 = *pbVar1 | 2;
      }
    }
    *(undefined2 *)(puVar2 + 0xbc) = 0xffff;
    *(undefined1 *)((int)puVar2 + 0x2a7) = 2;
    if (puVar11[0xc9] == param_1) {
      puVar11[0xc9] = 0xffffffff;
    }
    if (puVar11[0xca] == param_1) {
      puVar11[0xca] = 0xffffffff;
    }
    FUN_0056ce30();
    FUN_0056d6a0();
    FUN_00565420(param_1);
    puVar3 = (undefined4 *)(*(short *)((int)puVar2 + 0x1ea) + 0x10 + (int)puVar2);
    *puVar3 = local_1c;
    puVar3[1] = local_18;
    puVar3[2] = local_14;
    if ((short)puVar2[0x2d] == 0) {
      FUN_0055add0(param_1);
    }
    object_recalculate_bounding_radius_recursive(param_1);
    cVar5 = FUN_00566910();
    if ((cVar5 == '\x01') && (iVar12 = object_try_and_get(2), iVar12 != 0)) {
      *(undefined4 *)(iVar12 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
    }
    iVar6 = DAT_0087a480;
    iVar10 = local_8;
    puVar11 = local_10;
    if (DAT_00719720 != 1) goto LAB_00568c1f;
    iVar12 = datum_get();
    puVar11 = local_10;
    if ((iVar12 != 0) && (*(short *)(iVar12 + 2) == -1)) {
      *(undefined4 *)(iVar12 + 0x180) = 0;
      *(undefined4 *)(iVar12 + 0x17c) = 0;
      *(undefined4 *)(iVar12 + 0x1e0) = 0;
      *(undefined4 *)(iVar12 + 0x1dc) = 0;
      goto LAB_00568ba2;
    }
  }
  uVar7 = puVar2[0x86];
  iVar10 = local_8;
  if (((uVar7 != 0xffffffff) && (sVar9 = (short)uVar7, -1 < sVar9)) &&
     (sVar9 < *(short *)(iVar6 + 0x20))) {
    iVar12 = (int)*(short *)(iVar6 + 0x22) * (int)sVar9;
    sVar9 = *(short *)(iVar12 + *(int *)(iVar6 + 0x34));
    if ((((sVar9 != 0) && ((sVar8 = (short)(uVar7 >> 0x10), sVar8 == 0 || (sVar9 == sVar8)))) &&
        (*(short *)(iVar12 + *(int *)(iVar6 + 0x34) + 2) != -1)) && (DAT_0071c2d8 != 0)) {
      player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
      iVar10 = local_8;
    }
  }
LAB_00568c1f:
  *(undefined2 *)(puVar11 + 0xee) = 0;
  unit_drop_inventory_weapons(param_1);
  iVar12 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar10);
  iVar10 = *(int *)(iVar12 + 0x318);
  if (iVar10 != -1) {
    unit_drop_object_from_hand(param_1,iVar10);
    *(undefined4 *)(iVar12 + 0x318) = 0xffffffff;
  }
  unit_drop_grenades(param_1);
  if ((char)puVar11[0xa3] == '\0') {
    unit_drop_current_weapon(param_1,1);
  }
  *(undefined2 *)((int)puVar11 + 0x2ae) = 0xffff;
  *(undefined2 *)((int)puVar11 + 0x2aa) = 0xffff;
  *(undefined1 *)((int)puVar11 + 0x289) = 0;
  if (*(char *)((int)puVar11 + 0x28d) == '\x01') {
    *(undefined1 *)((int)puVar11 + 0x28d) = 0;
  }
  return;
}
#endif
