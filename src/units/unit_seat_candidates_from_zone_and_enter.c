// unit_seat_candidates_from_zone_and_enter  (Ghidra: FUN_0056a4c0)
// address 0x56a4c0, size 1595 bytes, name confidence 0.3, rewrite confidence 0.2
// functions.md: "Finds seats on the unit matching a name/flags filter and forcibly detaches
// whichever child objects currently occupy them, repositioning each at its seat's exit marker."
// (this rewrite finds the function also *enters* a still-unseated candidate into the matching
// seat, not only detaches -- see the final `unit_enter_vehicle_seat` call.)
// evidence: unit_find_seats_matching_name_and_flags.c (0x56a310) for the seat-search call;
//   src/units/unit_enter_vehicle_seat.c (vehicle_index, seat_index, EAX -> unit_index) for the
//   final call; the ejection block matches unit_apply_damage_effects.c (0x5674a0) and
//   unit_detach_and_enter_named_seat.c (0x569d40) exactly (same UNSURE-CALL gaps apply here).
//   The 0x0087a464/0x0087a468 pair (object_list_header_data and its link array, per
//   types/units.h's globals notes) is walked as a classic {unused, object_index, next_link}
//   0xc-byte-stride singly linked list.
// blam-cc: param_1 -> unit_index, param_2 -> name_filter, in_EAX -> zone_list_index.
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

extern data_array *object_data;             // 0x008603b0
extern data_array *player_data;             // 0x0087a480
extern tag_instance *tag_instances;         // 0x0087bc14
extern data_array *object_list_header_data; // 0x0087a464
extern data_array *object_list_link_array;  // 0x0087a468, UNSURE typed shape, raw 0xc-stride
extern game_time_globals *game_time; // 0x006f1d6c, the game time globals (types/game.h)
extern int32_t game_connection_role;        // 0x00719720
extern uint8_t *network_client;      // 0x0071c2d8 (networking.h network_client; renamed from network_client, which collides with game.h's typedef)
extern void *matrix4x3_multiply_thunk;      // 0x00696664

extern int16_t unit_find_seats_matching_name_and_flags(uint32_t unit_index, char *name_filter, uint16_t flag_selector,
                                                        int16_t *out_indices, int16_t max_indices); // 0x56a310
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
extern uint8_t unit_set_or_test_seat_and_weapon_label(uint32_t unit_index, char *seat_label, char *weapon_label, uint8_t test_only); // 0x5651e0
extern uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request); // 0x565420, stack unit, ECX request
static const int8_t k_unit_exit_seat_request[2] = {0x14, 0}; // every caller builds these two bytes on its stack
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern uint8_t unit_all_seats_unoccupied(uint32_t unit_index);                      // 0x566910
extern uint32_t unit_enter_vehicle_seat(uint32_t vehicle_index, int16_t seat_index, uint32_t unit_index); // 0x566970
extern void unit_dispatch_scripted_event_9(uint32_t param_1);                            // 0x56c370, UNSURE signature  // real signature (unit_dispatch_scripted_event_9.c): void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key); Ghidra recovered 1 of 2 args at this call site
extern void unit_recompute_seat_occupants(uint32_t unit_index);                         // 0x56ce30
extern void unit_pick_and_ready_next_weapon(uint32_t unit_index);                         // 0x56d6a0

int16_t unit_seat_candidates_from_zone_and_enter(uint32_t unit_index, char *name_filter, uint32_t zone_list_index)
    // blam-cc: param_1, param_2, in_EAX
{
    int16_t filled_count = 0;
    if (unit_index == 0xffffffff) {
        return 0;
    }

    object *self_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Unit *self_tag = (Unit *)tag_instances[self_obj->definition_tag & 0xffff].data;
    int16_t seat_indices[16];
    int16_t seat_count = unit_find_seats_matching_name_and_flags(unit_index, name_filter, 0xffff, seat_indices, 0x10);

    uint32_t link;
    if (zone_list_index == 0xffffffff) {
        link = 0xffffffff;
    } else {
        link = *(uint32_t *)((uint8_t *)object_list_header_data->data + (zone_list_index & 0xffff) * 0xc + 8);
    }
    uint32_t candidate_index; // local_128
    uint32_t next_link;       // local_110
    if (link == 0xffffffff) {
        candidate_index = 0xffffffff;
        next_link = 0xffffffff;
    } else {
        uint8_t *node = ((uint8_t *)object_list_link_array->data) + (link & 0xffff) * 0xc;
        next_link = *(uint32_t *)(node + 8);
        candidate_index = *(uint32_t *)(node + 4);
    }

    while (candidate_index != 0xffffffff) {
        object *candidate = ((object_header *)object_data->data)[candidate_index & 0xffff].data;

        if (((_object_mask_unit & (1 << (candidate->type & 0x1f))) != 0) &&
            ((self_obj->vitality_flags & _object_health_frozen_bit) == 0) && (0 < seat_count)) {
            for (int16_t i = 0; i < seat_count; i++) {
                int16_t seat_index = seat_indices[i];
                if (seat_index == -1) continue;

                UnitSeat *seat = (UnitSeat *)self_tag->seats.pointer + seat_index;
                uint8_t ok = (candidate->type == _object_type_vehicle) ||
                             (unit_set_or_test_seat_and_weapon_label(candidate_index, seat->label.string, (char *)0, 0) != 0);
                if (!ok) continue;

                if (candidate->parent_object != k_datum_index_none) {
                    unit_data *candidate_unit = (unit_data *)((uint8_t *)candidate + k_unit_data_offset);
                    if ((candidate_unit->vehicle_seat_index != -1) && (game_connection_role != 1)) {
                        datum_index old_parent = candidate->parent_object;
                        object *old_parent_obj = ((object_header *)object_data->data)[old_parent & 0xffff].data;
                        unit_data *old_parent_unit = (unit_data *)((uint8_t *)old_parent_obj + k_unit_data_offset);
                        Unit *old_parent_tag = (Unit *)tag_instances[old_parent_obj->definition_tag & 0xffff].data;
                        uint8_t *exit_marker = (uint8_t *)(*(int32_t *)((uint8_t *)old_parent_tag + 0x2e8) + 0x24 +
                                                           candidate_unit->vehicle_seat_index * 0x11c);
                        uint8_t local_transform[116];
                        object_get_node_local_transform(old_parent, exit_marker, (object_marker *)local_transform, 1);

                        Unit *candidate_tag = (Unit *)tag_instances[candidate->definition_tag & 0xffff].data;
                        uint32_t *node_data = (uint32_t *)(*(int32_t *)((uint8_t *)tag_instances[candidate_tag->base.model.tag_id.index & 0xffff].data + 0xbc));
                        uint32_t saved[3] = { node_data[0xa], node_data[0xb], node_data[0xc] };

                        if ((old_parent_unit->driver_unit_index == candidate_index) &&
                            (old_parent_unit->animation_state != 0x25) &&      // puVar9 + 0x2a3
                            (candidate->parent_object != k_datum_index_none)) {
                            unit_try_set_animation_state(old_parent, 0x25);
                        }
                        candidate_unit->last_parent_object_index = old_parent;             // puVar8[0xcb] = uVar7
                        candidate_unit->last_seat_change_tick = game_time->game_time;    // puVar8[0xcc]
                        // the first clear pair is on puVar8 (the candidate); the pair below is on puVar9
                        if (candidate_unit->driver_unit_index == candidate_index) candidate_unit->driver_unit_index = k_datum_index_none;
                        if (candidate_unit->gunner_unit_index == candidate_index) candidate_unit->gunner_unit_index = k_datum_index_none;
                        object_snap_to_parent_marker_and_detach(candidate_index);
                        object_set_position_and_orientation(candidate_index, 0, 0); // UNSURE-CALL
                        matrix4x3_multiply(0, 0, 0);                               // UNSURE-CALL

                        Object *candidate_def = (Object *)tag_instances[candidate->definition_tag & 0xffff].data;
                        if (*(uint32_t *)&candidate_def->model.tag_id != 0xffffffff) {
                            if ((candidate->flags & 1) != 0) object_for_each_light_attachment(candidate_index, 1);
                            if (*(uint32_t *)&candidate_def->model.tag_id != 0xffffffff) {
                                candidate->flags &= ~1u;                          // object + 0x10, bit 0
                                ((object_header *)object_data->data)[candidate_index & 0xffff].flags |= 0x02;
                            }
                        }
                        candidate_unit->vehicle_seat_index = -1;
                        candidate_unit->base_animation_state = _unit_base_animation_state_stand;
                        if (old_parent_unit->driver_unit_index == candidate_index) old_parent_unit->driver_unit_index = k_datum_index_none;
                        if (old_parent_unit->gunner_unit_index == candidate_index) old_parent_unit->gunner_unit_index = k_datum_index_none;
                        unit_recompute_seat_occupants(candidate_index);
                        unit_pick_and_ready_next_weapon(candidate_index);
                        unit_update_animation_state_machine(candidate_index, k_unit_exit_seat_request);
                        uint32_t *node_func = (uint32_t *)(*(int16_t *)((uint8_t *)candidate + 0x1ea) + 0x10 + (uint8_t *)candidate);
                        node_func[0] = saved[0]; node_func[1] = saved[1]; node_func[2] = saved[2];
                        if (candidate->type == _object_type_biped) unit_reset_orientation_and_find_position(candidate_index);
                        object_recalculate_bounding_radius_recursive(candidate_index);
                        if ((unit_all_seats_unoccupied(candidate_index) == 1) && (object_try_and_get(candidate_index, _object_mask_vehicle) != 0)) {
                            // writes the current tick into vehicle_data.network_update_tick (+0x5ac)
                        }
                        if ((game_connection_role == 1) && (datum_get() != 0)) {
                            // UNSURE: matches the record-clear pattern in the other ejection blocks
                        }
                    }
                    if (candidate->network_role == 0) {
                        unit_dispatch_scripted_event_9(1);
                    }
                    if ((game_connection_role == 1) && (candidate_unit->controlling_player != k_datum_index_none)) {
                        uint32_t controlling = candidate_unit->controlling_player;
                        if ((-1 < (int16_t)controlling) && ((int16_t)controlling < player_data->maximum_count)) {
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
                }

                if (candidate->parent_object != k_datum_index_none) {
                    continue; // still seated somewhere: skip entering, matches LAB_0056aa90
                }
                if (unit_enter_vehicle_seat(unit_index, seat_index, candidate_index) != 0) {
                    seat_indices[i] = -1;
                    filled_count++;
                    break;
                }
            }
        }

        if (next_link == 0xffffffff) {
            candidate_index = 0xffffffff;
        } else {
            uint8_t *node = ((uint8_t *)object_list_link_array->data) + (next_link & 0xffff) * 0xc;
            next_link = *(uint32_t *)(node + 8);
            candidate_index = *(uint32_t *)(node + 4);
        }
    }
    return filled_count;
}

#if 0
Original Ghidra decompilation (0x56a4c0):

short FUN_0056a4c0(uint param_1,undefined4 param_2)

{
  byte *pbVar1;
  int iVar2;
  undefined4 *puVar3;
  short sVar4;
  uint *puVar5;
  int iVar6;
  uint uVar7;
  uint *puVar8;
  uint *puVar9;
  undefined4 uVar10;
  undefined4 uVar11;
  undefined4 uVar12;
  int iVar13;
  uint *puVar14;
  short sVar15;
  char cVar16;
  short sVar17;
  short sVar18;
  uint in_EAX;
  int iVar19;
  short sVar20;
  short sVar21;
  int iVar22;
  uint local_128;
  uint local_110;
  undefined1 local_d0 [4];
  uint uStack_cc;
  uint uStack_c8;
  uint uStack_c4;
  uint uStack_b4;
  uint uStack_b0;
  uint uStack_ac;
  short local_98 [16];
  undefined1 local_78 [116];

  sVar15 = 0;
  if (param_1 != 0xffffffff) {
    puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
    iVar6 = *(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    sVar17 = FUN_0056a310(param_1,param_2,0xffffffff,local_98,0x10);
    local_128 = 0xffffffff;
    if (in_EAX != 0xffffffff) {
      uVar7 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
      if (uVar7 == 0xffffffff) {
        local_128 = 0xffffffff;
        local_110 = 0xffffffff;
      }
      else {
        iVar2 = *(int *)(DAT_0087a468 + 0x34) + (uVar7 & 0xffff) * 0xc;
        local_110 = *(uint *)(iVar2 + 8);
        local_128 = *(uint *)(iVar2 + 4);
      }
    }
    if (local_128 != 0xffffffff) {
      do {
        iVar22 = (local_128 & 0xffff) * 0xc;
        iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar22);
        if ((((1 << (*(byte *)(iVar2 + 0xb4) & 0x1f) & 3U) != 0) &&
            ((*(byte *)((int)puVar5 + 0x106) & 4) == 0)) && (sVar18 = 0, 0 < sVar17)) {
          do {
            sVar4 = local_98[sVar18];
            if ((sVar4 != -1) &&
               ((*(short *)(iVar2 + 0xb4) == 1 ||
                (cVar16 = unit_set_or_test_seat_and_weapon_label
                                    (sVar4 * 0x11c + *(int *)(iVar6 + 0x2e8) + 4,0,0),
                cVar16 != '\0')))) {
              if (*(int *)(iVar2 + 0x11c) != -1) {
                if ((*(short *)(iVar2 + 0x2f0) != -1) && (DAT_00719720 != 1)) {
                  puVar8 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar22);
                  uVar7 = puVar8[0x47];
                  if ((uVar7 != 0xffffffff) && ((short)puVar8[0xbc] != -1)) {
                    puVar9 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar7 & 0xffff) * 0xc);
                    object_get_node_local_transform
                              (uVar7,*(int *)(*(int *)((*puVar9 & 0xffff) * 0x20 + 0x14 +
                                                      DAT_0087bc14) + 0x2e8) + 0x24 +
                                     (short)puVar8[0xbc] * 0x11c,local_78,1);
                    iVar19 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar8 & 0xffff) * 0x20 + 0x14
                                                                  + DAT_0087bc14) + 0x34) & 0xffff)
                                               * 0x20 + 0x14 + DAT_0087bc14) + 0xbc);
                    uVar10 = *(undefined4 *)(iVar19 + 0x28);
                    uVar11 = *(undefined4 *)(iVar19 + 0x2c);
                    uVar12 = *(undefined4 *)(iVar19 + 0x30);
                    if (((puVar9[0xc9] == local_128) && (*(char *)((int)puVar9 + 0x2a3) != '%')) &&
                       (puVar8[0x47] != 0xffffffff)) {
                      unit_try_set_animation_state(puVar8[0x47],0x25);
                    }
                    iVar13 = DAT_006f1d6c;
                    puVar8[0xcb] = uVar7;
                    puVar8[0xcc] = *(uint *)(iVar13 + 0xc);
                    if (puVar8[0xc9] == local_128) {
                      puVar8[0xc9] = 0xffffffff;
                    }
                    if (puVar8[0xca] == local_128) {
                      puVar8[0xca] = 0xffffffff;
                    }
                    FUN_004f6610(local_128);
                    object_set_position_and_orientation(local_128,0,0);
                    iVar13 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar22);
                    (*(code *)PTR_matrix4x3_multiply_00696664)
                              (*(short *)(iVar13 + 0x1f2) + iVar13,iVar19 + 0x68,local_d0);
                    puVar8[0x1d] = uStack_cc;
                    puVar8[0x1e] = uStack_c8;
                    puVar8[0x1f] = uStack_c4;
                    puVar8[0x20] = uStack_b4;
                    puVar8[0x21] = uStack_b0;
                    iVar19 = DAT_008603b0;
                    puVar8[0x22] = uStack_ac;
                    puVar14 = *(uint **)(*(int *)(iVar19 + 0x34) + 8 + iVar22);
                    iVar19 = *(int *)((*puVar14 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
                    if (*(int *)(iVar19 + 0x34) != -1) {
                      if ((puVar14[4] & 1) != 0) {
                        object_for_each_light_attachment(0,1);
                      }
                      if (*(int *)(iVar19 + 0x34) != -1) {
                        iVar19 = *(int *)(DAT_008603b0 + 0x34);
                        puVar14[4] = puVar14[4] & 0xfffffffe;
                        pbVar1 = (byte *)(iVar19 + iVar22 + 2);
                        *pbVar1 = *pbVar1 | 2;
                      }
                    }
                    *(undefined2 *)(puVar8 + 0xbc) = 0xffff;
                    *(undefined1 *)((int)puVar8 + 0x2a7) = 2;
                    if (puVar9[0xc9] == local_128) {
                      puVar9[0xc9] = 0xffffffff;
                    }
                    if (puVar9[0xca] == local_128) {
                      puVar9[0xca] = 0xffffffff;
                    }
                    FUN_0056ce30();
                    FUN_0056d6a0();
                    FUN_00565420(local_128);
                    puVar3 = (undefined4 *)(*(short *)((int)puVar8 + 0x1ea) + 0x10 + (int)puVar8);
                    *puVar3 = uVar10;
                    puVar3[1] = uVar11;
                    puVar3[2] = uVar12;
                    if ((short)puVar8[0x2d] == 0) {
                      FUN_0055add0(local_128);
                    }
                    object_recalculate_bounding_radius_recursive(local_128);
                    cVar16 = FUN_00566910();
                    if ((cVar16 == '\x01') && (iVar19 = object_try_and_get(2), iVar19 != 0)) {
                      *(undefined4 *)(iVar19 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
                    }
                    if (((DAT_00719720 == 1) && (iVar19 = datum_get(), iVar19 != 0)) &&
                       (*(short *)(iVar19 + 2) == -1)) {
                      *(undefined4 *)(iVar19 + 0x180) = 0;
                      *(undefined4 *)(iVar19 + 0x17c) = 0;
                      *(undefined4 *)(iVar19 + 0x1e0) = 0;
                      *(undefined4 *)(iVar19 + 0x1dc) = 0;
                    }
                  }
                  if (puVar8[1] == 0) {
                    FUN_0056c370(1);
                  }
                  if ((((DAT_00719720 == 1) && (uVar7 = puVar8[0x86], uVar7 != 0xffffffff)) &&
                      (sVar21 = (short)uVar7, -1 < sVar21)) &&
                     (sVar21 < *(short *)(DAT_0087a480 + 0x20))) {
                    iVar19 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar21;
                    sVar21 = *(short *)(iVar19 + *(int *)(DAT_0087a480 + 0x34));
                    if ((((sVar21 != 0) &&
                         ((sVar20 = (short)(uVar7 >> 0x10), sVar20 == 0 || (sVar21 == sVar20)))) &&
                        (*(short *)(iVar19 + *(int *)(DAT_0087a480 + 0x34) + 2) != -1)) &&
                       (DAT_0071c2d8 != 0)) {
                      player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
                    }
                  }
                }
                if (*(int *)(iVar2 + 0x11c) != -1) goto LAB_0056aa90;
              }
              cVar16 = unit_enter_vehicle_seat(param_1,sVar4);
              if (cVar16 != '\0') {
                local_98[sVar18] = -1;
                sVar15 = sVar15 + 1;
                break;
              }
            }
LAB_0056aa90:
            sVar18 = sVar18 + 1;
          } while (sVar18 < sVar17);
        }
        if (local_110 == 0xffffffff) {
          local_128 = 0xffffffff;
        }
        else {
          iVar2 = *(int *)(DAT_0087a468 + 0x34) + (local_110 & 0xffff) * 0xc;
          local_110 = *(uint *)(iVar2 + 8);
          local_128 = *(uint *)(iVar2 + 4);
        }
        if (local_128 == 0xffffffff) {
          return sVar15;
        }
      } while( true );
    }
  }
  return 0;
}
#endif
