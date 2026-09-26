// unit_detach_child_at_named_seat  (Ghidra: FUN_0056ab50)
// address 0x56ab50, size 1848 bytes, name confidence 0.4, rewrite confidence 0.2
// functions.md: "Finds the child object attached to the unit at a seat whose marker name
// matches param_2 and detaches/repositions it at that marker."
// evidence: types/objects.h object_iterator (0x4f6f20, type_mask/flags_mask/index/handle);
//   shares its ejection block with unit_apply_damage_effects.c (0x5674a0) and its "not a
//   vehicle" melee/lunge-style branch likewise (same UNSURE-CALL gaps apply here).
// blam-cc: param_1 -> unit_index, param_2 -> seat_marker_name (may be NULL/empty, meaning "any").
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
#include <string.h>

extern data_array *object_data;      // 0x008603b0
extern data_array *player_data;      // 0x0087a480
extern tag_instance *tag_instances;  // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c, the game time globals (types/game.h)
extern int32_t game_connection_role; // 0x00719720
extern uint8_t *network_client; // 0x0071c2d8 (networking.h network_client; renamed from network_client, which collides with game.h's typedef)
extern void *matrix4x3_multiply_thunk; // 0x00696664

extern int32_t _tolower(int32_t c); // 0x624687
extern char *strstr(const char *haystack, const char *needle); // CRT strstr (0x625430: the MSVC asm strstr, haystack then needle; case-sensitive)
extern void actor_notify_weapon_pickup_once(uint32_t object_index);                        // 0x42c370, UNSURE signature
extern void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out);            // 0x4cc0d0, UNSURE signature
extern uint32_t datum_get(void);                                        // 0x4d0680, UNSURE signature  // real signature (datum_get.c): void * datum_get(datum_index handle, data_array *array); Ghidra recovered 0 of 2 args at this call site
extern int32_t animation_choose_random_permutation(int32_t mode);                              // 0x4d6280
extern void player_update_history_free_all(void *history);              // 0x4e6f20, UNSURE signature
extern void object_set_position_and_orientation(uint32_t object_index, void *a, void *b); // 0x4f51c0, UNSURE signature  // real signature (object_set_position_and_orientation.c): void object_set_position_and_orientation(uint32_t object_index, real_vector3d *forward, real_vector3d *up, real_point3d *position); Ghidra recovered 3 of 4 args at this call site
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker, uint32_t param_4); // 0x4f6080
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index);                          // 0x4f6610, UNSURE signature
extern object * object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern object * object_iterator_next(object_iterator *iterator);        // 0x4f6f20
extern void object_recalculate_bounding_radius_recursive(uint32_t object_index); // 0x4f82b0
extern void object_for_each_light_attachment(uint32_t object_index, uint32_t flag); // 0x4f9a20, UNSURE signature  // real signature (object_for_each_light_attachment.c): void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table, int32_t invoke_callback); Ghidra recovered 2 of 3 args at this call site
extern void unit_reset_orientation_and_find_position(uint32_t object_index);                          // 0x55add0, UNSURE signature
extern void unit_update_animation_state_machine(uint32_t unit_index);                         // 0x565420, UNSURE signature  // real signature (unit_update_animation_state_machine.c): uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request); Ghidra recovered 1 of 2 args at this call site
extern uint8_t unit_state_is_scripted_animation(unit_data *unit);                   // 0x565c60, UNSURE signature
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern uint8_t unit_all_seats_unoccupied(uint32_t unit_index);                      // 0x566910
extern void unit_notify_weapon_removed(int32_t object_index, int16_t new_state);     // 0x56ab10
extern void unit_dispatch_scripted_event_9(uint32_t param_1);                            // 0x56c370, UNSURE signature  // real signature (unit_dispatch_scripted_event_9.c): void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key); Ghidra recovered 1 of 2 args at this call site
extern void unit_recompute_seat_occupants(uint32_t unit_index);                         // 0x56ce30
extern void unit_pick_and_ready_next_weapon(uint32_t unit_index);                         // 0x56d6a0
extern void unit_set_custom_animation(TagID animation_graph_tag, int16_t animation_index); // 0x56ebd0  // real signature (unit_set_custom_animation.c): void unit_set_custom_animation(uint32_t object_index, datum_index graph, int16_t animation_index); Ghidra recovered 2 of 3 args at this call site

int16_t unit_detach_child_at_named_seat(uint32_t unit_index, char *seat_marker_name)
{
    int16_t detach_count = 0;
    if (unit_index == 0xffffffff) {
        return 0;
    }

    object *self_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Unit *self_tag = (Unit *)tag_instances[self_obj->definition_tag & 0xffff].data;
    uint8_t name_is_empty = (seat_marker_name == (char *)0) || (seat_marker_name[0] == '\0');

    object_iterator iter = { _object_mask_unit, 0, 0, 0, 0xffffffff };
    object *candidate = object_iterator_next(&iter);
    if (candidate == (object *)0) {
        return 0;
    }

    do {
        if (candidate->parent_object == unit_index) {
            int16_t seat_index = ((unit_data *)((uint8_t *)candidate + k_unit_data_offset))->vehicle_seat_index;
            UnitSeat *seat = (UnitSeat *)self_tag->seats.pointer + seat_index;
            char lowered[260];
            char *src = seat->label.string;
            int32_t i = 0;
            do { lowered[i] = (char)_tolower((uint8_t)src[i]); i++; } while (src[i - 1] != '\0');

            if (name_is_empty || (strstr((const char *)lowered, seat_marker_name) != 0)) {
                uint32_t child_index = iter.handle;
                object *child = object_try_and_get(child_index, _object_mask_unit);
                if ((child != (object *)0) && (game_connection_role != 1) && (child->parent_object != k_datum_index_none) &&
                    (((unit_data *)((uint8_t *)child + k_unit_data_offset))->vehicle_seat_index != -1)) {
                    if (child->type == _object_type_vehicle) {
                        datum_index old_parent = child->parent_object;
                        object *old_parent_obj = ((object_header *)object_data->data)[old_parent & 0xffff].data;
                        unit_data *old_parent_unit = (unit_data *)((uint8_t *)old_parent_obj + k_unit_data_offset);
                        if ((old_parent_obj->parent_object == k_datum_index_none) ||
                            (((unit_data *)((uint8_t *)old_parent_obj + k_unit_data_offset))->vehicle_seat_index == -1)) {
                            if (game_connection_role == 1) {
                                goto refresh_history;
                            }
                        } else {
                            Unit *old_parent_tag = (Unit *)tag_instances[old_parent_obj->definition_tag & 0xffff].data;
                            uint8_t *exit_marker = (uint8_t *)(*(int32_t *)((uint8_t *)old_parent_tag + 0x2e8) + 0x24 +
                                                               ((unit_data *)((uint8_t *)old_parent_obj + k_unit_data_offset))->vehicle_seat_index * 0x11c);
                            uint8_t local_transform[96];
                            object_get_node_local_transform(old_parent, exit_marker, (object_marker *)local_transform, 1);

                            Unit *child_tag = (Unit *)tag_instances[child->definition_tag & 0xffff].data;
                            uint32_t *node_data = (uint32_t *)(*(int32_t *)((uint8_t *)tag_instances[child_tag->base.model.tag_id.index & 0xffff].data + 0xbc));
                            uint32_t saved[3] = { node_data[0xa], node_data[0xb], node_data[0xc] };

                            unit_data *child_unit = (unit_data *)((uint8_t *)child + k_unit_data_offset);
                            if ((old_parent_unit->driver_unit_index == child_index) &&
                                (old_parent_unit->animation_state != 0x25) &&
                                (child->parent_object != k_datum_index_none)) {   // puVar5 + 0x2a3
                                unit_try_set_animation_state(old_parent, 0x25);
                            }
                            child_unit->last_parent_object_index = old_parent;           // puVar15[0xcb] = uVar4
                            child_unit->last_seat_change_tick = game_time->game_time; // puVar15[0xcc]
                            // the first clear pair is on puVar15 (the child); the pair below is on puVar5
                            if (child_unit->driver_unit_index == child_index) child_unit->driver_unit_index = k_datum_index_none;
                            if (child_unit->gunner_unit_index == child_index) child_unit->gunner_unit_index = k_datum_index_none;
                            object_snap_to_parent_marker_and_detach(child_index);
                            object_set_position_and_orientation(child_index, 0, 0); // UNSURE-CALL
                            matrix4x3_multiply(0, 0, 0);                           // UNSURE-CALL

                            Object *child_def = (Object *)tag_instances[child->definition_tag & 0xffff].data;
                            if (*(uint32_t *)&child_def->model.tag_id != 0xffffffff) {
                                if ((child->flags & 1) != 0) object_for_each_light_attachment(child_index, 1);
                                if (*(uint32_t *)&child_def->model.tag_id != 0xffffffff) {
                                    child->flags &= ~1u;                          // object + 0x10, bit 0
                                    ((object_header *)object_data->data)[child_index & 0xffff].flags |= 0x02;
                                }
                            }
                            child_unit->vehicle_seat_index = -1;
                            child_unit->base_animation_state = _unit_base_animation_state_stand;
                            if (old_parent_unit->driver_unit_index == child_index) old_parent_unit->driver_unit_index = k_datum_index_none;
                            if (old_parent_unit->gunner_unit_index == child_index) old_parent_unit->gunner_unit_index = k_datum_index_none;
                            unit_recompute_seat_occupants(child_index);
                            unit_pick_and_ready_next_weapon(child_index);
                            unit_update_animation_state_machine(child_index);
                            uint32_t *node_func = (uint32_t *)(*(int16_t *)((uint8_t *)child + 0x1ea) + 0x10 + (uint8_t *)child);
                            node_func[0] = saved[0]; node_func[1] = saved[1]; node_func[2] = saved[2];
                            if (child->type == _object_type_biped) unit_reset_orientation_and_find_position(child_index);
                            object_recalculate_bounding_radius_recursive(child_index);
                            if ((unit_all_seats_unoccupied(child_index) == 1) && (object_try_and_get(child_index, _object_mask_vehicle) != 0)) {
                                // writes the current tick into vehicle_data.network_update_tick (+0x5ac)
                            }
                            if (game_connection_role == 1) {
                                if (datum_get() != 0) {
                                    // UNSURE: matches the record-clear pattern in the other ejection blocks
                                    goto refresh_history;
                                }
                                goto refresh_history;
                            }
                        }
refresh_history:
                        {
                            unit_data *cur_unit = (unit_data *)((uint8_t *)child + k_unit_data_offset);
                            uint32_t controlling = cur_unit->controlling_player;
                            if ((game_connection_role == 1) && (controlling != k_datum_index_none) &&
                                (-1 < (int16_t)controlling) && ((int16_t)controlling < player_data->maximum_count)) {
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
                    } else {
                        // not a vehicle: melee/lunge-style animation swap, matching
                        // unit_apply_damage_effects.c's identical "responsible is not a vehicle" branch.
                        if (unit_state_is_scripted_animation(0) == 0) {
                            Unit *child_tag = (Unit *)tag_instances[child->definition_tag & 0xffff].data;
                            uint8_t *weapon_types = *(uint8_t **)(
                                (uint8_t *)tag_instances[child_tag->base.animation_graph.tag_id.index & 0xffff].data + 0x10);
                            uint8_t *seat_block = weapon_types + ((unit_data *)((uint8_t *)child + k_unit_data_offset))->animation_definition_index * 100; // animation_definition_index (puVar15[0xa8])
                            if ((8 < *(int32_t *)(seat_block + 0x40)) && (*(int16_t *)(*(int32_t *)(seat_block + 0x44) + 0x10) != -1)) {
                                unit_data *old_parent_unit_check = (unit_data *)((uint8_t *)((object_header *)object_data->data)[child->parent_object & 0xffff].data + k_unit_data_offset);
                                if (old_parent_unit_check->driver_unit_index == child_index) {
                                    unit_notify_weapon_removed((int32_t)child_index, 0);
                                }
                                int16_t new_anim = (int16_t)animation_choose_random_permutation(1);
                                unit_set_custom_animation(child_tag->base.animation_graph.tag_id, new_anim);
                                Object *child_def = (Object *)tag_instances[child->definition_tag & 0xffff].data;
                                if (*(uint32_t *)&child_def->model.tag_id != 0xffffffff) {
                                    if ((child->flags & 1) != 0) object_for_each_light_attachment(child_index, 1);
                                    if (*(uint32_t *)&child_def->model.tag_id != 0xffffffff) {
                                        child->flags &= ~1u;                          // object + 0x10, bit 0
                                        ((object_header *)object_data->data)[child_index & 0xffff].flags |= 0x02;
                                    }
                                }
                                ((unit_data *)((uint8_t *)child + k_unit_data_offset))->animation_state = 0x1b;
                                actor_notify_weapon_pickup_once(child_index);
                                if (child->network_role == 0) {
                                    unit_dispatch_scripted_event_9(0);
                                }
                                detach_count++;
                            }
                        }
                    }
                }
            }
        }
        candidate = object_iterator_next(&iter);
    } while (candidate != (object *)0);

    return detach_count;
}

#if 0
Original Ghidra decompilation (0x56ab50):

short FUN_0056ab50(uint param_1,char *param_2)

{
  byte *pbVar1;
  undefined4 *puVar2;
  byte bVar3;
  uint uVar4;
  uint *puVar5;
  undefined4 uVar6;
  float fVar7;
  uint *puVar8;
  uint uVar9;
  bool bVar10;
  short sVar11;
  char cVar12;
  char *pcVar13;
  int iVar14;
  uint *puVar15;
  int iVar16;
  undefined4 uVar17;
  short sVar18;
  short sVar19;
  byte *pbVar20;
  int iVar21;
  undefined4 local_1dc;
  undefined1 local_1d8;
  undefined2 local_1d6;
  uint local_1d4;
  undefined4 local_1d0;
  int local_1cc;
  float local_1c8;
  float local_1c4;
  float local_1c0;
  float local_1bc;
  float local_1b8;
  float local_1b4;
  undefined1 local_1b0 [4];
  uint uStack_1ac;
  uint uStack_1a8;
  uint uStack_1a4;
  uint uStack_194;
  uint uStack_190;
  uint uStack_18c;
  undefined1 local_178 [96];
  float local_118;
  float local_114;
  float local_110;
  char acStack_10c [4];
  byte local_108 [260];

  sVar11 = 0;
  if (param_1 == 0xffffffff) {
    return 0;
  }
  local_1cc = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) &
                       0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (param_2 != (char *)0x0) {
    pcVar13 = param_2;
    do {
      cVar12 = *pcVar13;
      pcVar13 = pcVar13 + 1;
    } while (cVar12 != '\0');
    bVar10 = false;
    if (pcVar13 != param_2 + 1) goto LAB_0056abc8;
  }
  bVar10 = true;
LAB_0056abc8:
  local_1d0 = 0x86868686;
  local_1dc = 3;
  local_1d8 = 0;
  local_1d6 = 0;
  local_1d4 = 0xffffffff;
  iVar14 = object_iterator_next(&local_1dc);
  if (iVar14 == 0) {
    return 0;
  }
  do {
    if (*(uint *)(iVar14 + 0x11c) == param_1) {
      sVar19 = *(short *)(iVar14 + 0x2f0);
      iVar14 = *(int *)(local_1cc + 0x2e8);
      pcVar13 = (char *)(sVar19 * 0x11c + 4 + iVar14);
      do {
        cVar12 = *pcVar13;
        pcVar13[(int)(acStack_10c + (sVar19 * -0x11c - iVar14))] = cVar12;
        pcVar13 = pcVar13 + 1;
      } while (cVar12 != '\0');
      pbVar20 = local_108;
      bVar3 = local_108[0];
      while (bVar3 != 0) {
        iVar14 = _tolower((uint)*pbVar20);
        *pbVar20 = (byte)iVar14;
        pbVar1 = pbVar20 + 1;
        pbVar20 = pbVar20 + 1;
        bVar3 = *pbVar1;
      }
      if ((((bVar10) || (iVar14 = FUN_00625430(local_108,param_2), iVar14 != 0)) &&
          (uVar9 = local_1d4, puVar15 = (uint *)object_try_and_get(3), puVar15 != (uint *)0x0)) &&
         (((DAT_00719720 != 1 && (uVar4 = puVar15[0x47], uVar4 != 0xffffffff)) &&
          ((short)puVar15[0xbc] != -1)))) {
        if ((short)puVar15[0x2d] == 1) {
          iVar21 = (uVar9 & 0xffff) * 0xc;
          puVar15 = *(uint **)(iVar21 + 8 + *(int *)(DAT_008603b0 + 0x34));
          uVar4 = puVar15[0x47];
          iVar14 = DAT_0087a480;
          if ((uVar4 == 0xffffffff) || ((short)puVar15[0xbc] == -1)) {
LAB_0056b09b:
            if (DAT_00719720 == 1) {
LAB_0056b0a8:
              uVar9 = puVar15[0x86];
              if (((uVar9 != 0xffffffff) && (sVar19 = (short)uVar9, -1 < sVar19)) &&
                 (sVar19 < *(short *)(iVar14 + 0x20))) {
                iVar21 = (int)*(short *)(iVar14 + 0x22) * (int)sVar19;
                sVar19 = *(short *)(iVar21 + *(int *)(iVar14 + 0x34));
                if ((((sVar19 != 0) &&
                     ((sVar18 = (short)(uVar9 >> 0x10), sVar18 == 0 || (sVar19 == sVar18)))) &&
                    (*(short *)(iVar21 + *(int *)(iVar14 + 0x34) + 2) != -1)) && (DAT_0071c2d8 != 0)
                   ) {
                  player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
                }
              }
            }
          }
          else {
            puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc);
            iVar14 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar21);
            iVar14 = *(short *)(iVar14 + 0x1f2) + iVar14;
            object_get_node_local_transform
                      (uVar4,*(int *)(*(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                                     0x2e8) + 0x24 + (short)puVar15[0xbc] * 0x11c,local_178,1);
            local_1c8 = *(float *)(iVar14 + 0x28) - local_118;
            local_1c4 = *(float *)(iVar14 + 0x2c) - local_114;
            iVar16 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar15 & 0xffff) * 0x20 + 0x14 +
                                                          DAT_0087bc14) + 0x34) & 0xffff) * 0x20 +
                                       0x14 + DAT_0087bc14) + 0xbc);
            uVar17 = *(undefined4 *)(iVar16 + 0x28);
            local_1c0 = *(float *)(iVar14 + 0x30) - local_110;
            uVar6 = *(undefined4 *)(iVar16 + 0x2c);
            fVar7 = *(float *)(iVar16 + 0x30);
            if ((puVar5[0xc9] == uVar9) &&
               ((*(char *)((int)puVar5 + 0x2a3) != '%' && (puVar15[0x47] != 0xffffffff)))) {
              unit_try_set_animation_state(puVar15[0x47],0x25);
            }
            iVar14 = DAT_006f1d6c;
            puVar15[0xcb] = uVar4;
            puVar15[0xcc] = *(uint *)(iVar14 + 0xc);
            if (puVar15[0xc9] == uVar9) {
              puVar15[0xc9] = 0xffffffff;
            }
            if (puVar15[0xca] == uVar9) {
              puVar15[0xca] = 0xffffffff;
            }
            FUN_004f6610(uVar9);
            local_1bc = local_1c8 + (float)puVar15[0x17];
            local_1b8 = local_1c4 + (float)puVar15[0x18];
            local_1b4 = (local_1c0 + (float)puVar15[0x19]) - fVar7;
            object_set_position_and_orientation(uVar9,0,0);
            iVar14 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar21);
            (*(code *)PTR_matrix4x3_multiply_00696664)
                      (*(short *)(iVar14 + 0x1f2) + iVar14,iVar16 + 0x68,local_1b0);
            puVar15[0x1d] = uStack_1ac;
            puVar15[0x1e] = uStack_1a8;
            puVar15[0x1f] = uStack_1a4;
            puVar15[0x20] = uStack_194;
            puVar15[0x21] = uStack_190;
            iVar14 = DAT_008603b0;
            puVar15[0x22] = uStack_18c;
            puVar8 = *(uint **)(*(int *)(iVar14 + 0x34) + 8 + iVar21);
            iVar14 = *(int *)((*puVar8 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
            if (*(int *)(iVar14 + 0x34) != -1) {
              if ((puVar8[4] & 1) != 0) {
                object_for_each_light_attachment(0,1);
              }
              if (*(int *)(iVar14 + 0x34) != -1) {
                iVar14 = *(int *)(DAT_008603b0 + 0x34);
                puVar8[4] = puVar8[4] & 0xfffffffe;
                pbVar20 = (byte *)(iVar14 + iVar21 + 2);
                *pbVar20 = *pbVar20 | 2;
              }
            }
            *(undefined2 *)(puVar15 + 0xbc) = 0xffff;
            *(undefined1 *)((int)puVar15 + 0x2a7) = 2;
            if (puVar5[0xc9] == uVar9) {
              puVar5[0xc9] = 0xffffffff;
            }
            if (puVar5[0xca] == uVar9) {
              puVar5[0xca] = 0xffffffff;
            }
            FUN_0056ce30();
            FUN_0056d6a0();
            FUN_00565420(uVar9);
            puVar2 = (undefined4 *)(*(short *)((int)puVar15 + 0x1ea) + 0x10 + (int)puVar15);
            *puVar2 = uVar17;
            puVar2[1] = uVar6;
            puVar2[2] = fVar7;
            if ((short)puVar15[0x2d] == 0) {
              FUN_0055add0(uVar9);
            }
            object_recalculate_bounding_radius_recursive(uVar9);
            cVar12 = FUN_00566910();
            if ((cVar12 == '\x01') && (iVar14 = object_try_and_get(2), iVar14 != 0)) {
              *(undefined4 *)(iVar14 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
            }
            iVar14 = DAT_0087a480;
            if (DAT_00719720 == 1) {
              iVar21 = datum_get();
              if ((iVar21 != 0) && (*(short *)(iVar21 + 2) == -1)) {
                *(undefined4 *)(iVar21 + 0x180) = 0;
                *(undefined4 *)(iVar21 + 0x17c) = 0;
                *(undefined4 *)(iVar21 + 0x1e0) = 0;
                *(undefined4 *)(iVar21 + 0x1dc) = 0;
                goto LAB_0056b09b;
              }
              goto LAB_0056b0a8;
            }
          }
        }
        else {
          cVar12 = FUN_00565c60();
          if (cVar12 == '\0') {
            iVar14 = *(int *)((*puVar15 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
            iVar21 = *(int *)(*(int *)((*(uint *)(iVar14 + 0x44) & 0xffff) * 0x20 + 0x14 +
                                      DAT_0087bc14) + 0x10);
            iVar16 = (char)puVar15[0xa8] * 100;
            if ((8 < *(int *)(iVar16 + 0x40 + iVar21)) &&
               (*(short *)(*(int *)(iVar16 + iVar21 + 0x44) + 0x10) != -1)) {
              if (*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc) +
                           0x324) == uVar9) {
                FUN_0056ab10();
              }
              uVar17 = FUN_004d6280(1);
              unit_set_custom_animation(*(undefined4 *)(iVar14 + 0x44),uVar17);
              iVar21 = (uVar9 & 0xffff) * 0xc;
              puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar21);
              iVar14 = *(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
              if (*(int *)(iVar14 + 0x34) != -1) {
                if ((puVar5[4] & 1) != 0) {
                  object_for_each_light_attachment(0,1);
                }
                if (*(int *)(iVar14 + 0x34) != -1) {
                  iVar14 = *(int *)(DAT_008603b0 + 0x34);
                  puVar5[4] = puVar5[4] & 0xfffffffe;
                  pbVar20 = (byte *)(iVar14 + iVar21 + 2);
                  *pbVar20 = *pbVar20 | 2;
                }
              }
              *(undefined1 *)((int)puVar15 + 0x2a3) = 0x1b;
              FUN_0042c370();
              if (puVar15[1] == 0) {
                FUN_0056c370(0);
              }
              sVar11 = sVar11 + 1;
            }
          }
        }
      }
    }
    iVar14 = object_iterator_next(&local_1dc);
    if (iVar14 == 0) {
      return sVar11;
    }
  } while( true );
}
#endif
