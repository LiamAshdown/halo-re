// unit_detach_and_enter_named_seat  (Ghidra: FUN_00569d40)
// address 0x569d40, size 1340 bytes, name confidence 0.35, rewrite confidence 0.2
// functions.md: "Detaches a unit from its current parent object and, given a target parent and
// named seat marker, re-attaches/positions it there, refreshing occupancy tracking and physics
// state."
// evidence: shares its seat-exit sequence with unit_apply_damage_effects.c (0x5674a0) and
//   unit_release_transient_state.c (0x568610) -- same node-transform math and the same
//   recovery gaps (see those files' headers); the seat-search tail matches
//   unit_find_best_seat_to_enter's use of Unit.seats (0x2e4 count / 0x2e8 pointer) and UnitSeat
//   (0x11c stride, label TagString at +4).
// blam-cc: param_1 -> unit_index, param_2 -> target_parent_index, param_3 -> seat_marker_name.
// UNSURE: every UNSURE-CALL note from unit_apply_damage_effects.c's ejection block applies here
//   identically.
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
extern void unit_update_animation_state_machine(uint32_t unit_index);                         // 0x565420, UNSURE signature  // real signature (unit_update_animation_state_machine.c): uint16_t unit_update_animation_state_machine(uint32_t unit_index, const int8_t *request); Ghidra recovered 1 of 2 args at this call site
extern uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state); // 0x565f90
extern uint8_t unit_all_seats_unoccupied(uint32_t unit_index);                      // 0x566910
extern void unit_enter_vehicle_seat(uint32_t target_parent_index, int32_t seat_index); // 0x566970  // real signature (unit_enter_vehicle_seat.c): uint32_t unit_enter_vehicle_seat(uint32_t vehicle_index, int16_t seat_index, uint32_t unit_index); Ghidra recovered 2 of 3 args at this call site
extern void unit_dispatch_scripted_event_9(uint32_t param_1);                            // 0x56c370, UNSURE signature  // real signature (unit_dispatch_scripted_event_9.c): void unit_dispatch_scripted_event_9(uint8_t event_byte, int32_t hash_key); Ghidra recovered 1 of 2 args at this call site
extern uint8_t unit_is_seat_occupied(int32_t parent_index, int16_t seat_index); // 0x56cc10, UNSURE signature
extern void unit_recompute_seat_occupants(uint32_t unit_index);                         // 0x56ce30
extern void unit_pick_and_ready_next_weapon(uint32_t unit_index);                         // 0x56d6a0
extern int32_t __stricmp(const char *a, const char *b);                // 0x628d8b

void unit_detach_and_enter_named_seat(uint32_t unit_index, uint32_t target_parent_index, char *seat_marker_name)
{
    if ((unit_index == 0xffffffff) || (target_parent_index == 0xffffffff)) {
        return;
    }
    if (seat_marker_name[0] == '\0') {
        return;
    }

    object *self_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)self_obj + k_unit_data_offset);

    if ((self_obj->vitality_flags & _object_health_frozen_bit) == 0) {
        datum_index old_parent = self_obj->parent_object;
        if ((old_parent != k_datum_index_none) && (unit->vehicle_seat_index != -1) && (game_connection_role != 1)) {
            object *parent = ((object_header *)object_data->data)[old_parent & 0xffff].data;
            Unit *parent_tag = (Unit *)tag_instances[parent->definition_tag & 0xffff].data;
            uint8_t *exit_marker = (uint8_t *)(*(int32_t *)((uint8_t *)parent_tag + 0x2e8) + 0x24 +
                                               unit->vehicle_seat_index * 0x11c);
            uint8_t local_transform[116];
            object_get_node_local_transform(old_parent, exit_marker, (object_marker *)local_transform, 1);

            Unit *self_tag = (Unit *)tag_instances[self_obj->definition_tag & 0xffff].data;
            uint32_t *node_data = (uint32_t *)(*(int32_t *)((uint8_t *)tag_instances[self_tag->base.model.tag_id.index & 0xffff].data + 0xbc));
            uint32_t saved[3] = { node_data[0xa], node_data[0xb], node_data[0xc] }; // +0x28/+0x2c/+0x30

            unit_data *parent_unit = (unit_data *)((uint8_t *)parent + k_unit_data_offset);
            if ((parent_unit->driver_unit_index == unit_index) && (parent_unit->animation_state != 0x25) &&
                (self_obj->parent_object != k_datum_index_none)) {   // puVar5 + 0x2a3
                unit_try_set_animation_state(old_parent, 0x25);
            }
            unit->last_parent_object_index = old_parent;             // puVar3[0xcb] = uVar4
            unit->last_seat_change_tick = game_time->game_time;   // puVar3[0xcc]
            // the first clear pair is on puVar3 (this unit); the pair further down is on puVar5
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
                    object_for_each_light_attachment(unit_index, 1);
                }
                if (*(uint32_t *)&self_def->model.tag_id != 0xffffffff) {
                    self_obj->flags &= ~1u;                          // object + 0x10, bit 0
                    ((object_header *)object_data->data)[unit_index & 0xffff].flags |= 0x02;
                }
            }
            unit->vehicle_seat_index = -1;
            unit->base_animation_state = _unit_base_animation_state_stand;
            if (parent_unit->driver_unit_index == unit_index) {
                parent_unit->driver_unit_index = k_datum_index_none;
            }
            if (parent_unit->gunner_unit_index == unit_index) {
                parent_unit->gunner_unit_index = k_datum_index_none;
            }
            unit_recompute_seat_occupants(unit_index);
            unit_pick_and_ready_next_weapon(unit_index);
            unit_update_animation_state_machine(unit_index);
            uint32_t *node_func = (uint32_t *)(*(int16_t *)((uint8_t *)self_obj + 0x1ea) + 0x10 + (uint8_t *)self_obj);
            node_func[0] = saved[0];
            node_func[1] = saved[1];
            node_func[2] = saved[2];
            if (self_obj->type == _object_type_biped) {
                unit_reset_orientation_and_find_position(unit_index);
            }
            object_recalculate_bounding_radius_recursive(unit_index);
            if ((unit_all_seats_unoccupied(unit_index) == 1) && (object_try_and_get(unit_index, _object_mask_vehicle) != 0)) {
                // writes the current tick into vehicle_data.network_update_tick (+0x5ac)
            }
            if ((game_connection_role == 1) && (datum_get() != 0)) {
                // UNSURE: original re-fetches datum_get()'s record and clears four fields when
                // *(short*)(record+2) == -1; matches the pattern in unit_apply_damage_effects.
            }
        }
        if (self_obj->network_role == 0) {
            unit_dispatch_scripted_event_9(1);
        }
        if ((game_connection_role == 1) && (unit->controlling_player != k_datum_index_none)) {
            uint32_t controlling = unit->controlling_player;
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

    if (self_obj->parent_object == k_datum_index_none) {
        object *target_parent = ((object_header *)object_data->data)[target_parent_index & 0xffff].data;
        Unit *target_tag = (Unit *)tag_instances[target_parent->definition_tag & 0xffff].data;
        int32_t found_index = 0;
        if (0 < (int32_t)target_tag->seats.count) {
            UnitSeat *seats = (UnitSeat *)target_tag->seats.pointer;
            for (;;) {
                char *label = seats[found_index].label.string;
                if ((__stricmp(seat_marker_name, label) == 0) && (unit_is_seat_occupied(target_parent_index, (int16_t)found_index) == 0) &&
                    ((self_obj->type == _object_type_vehicle) ||
                     (unit_set_or_test_seat_and_weapon_label(target_parent_index, label, (char *)0, 0) != 0))) {
                    break;
                }
                found_index++;
                if ((int32_t)target_tag->seats.count <= found_index) {
                    return;
                }
            }
            unit_enter_vehicle_seat(target_parent_index, found_index);
        }
    }
    return;
}

#if 0
Original Ghidra decompilation (0x569d40):

void FUN_00569d40(uint param_1,uint param_2,char *param_3)

{
  byte *pbVar1;
  undefined4 *puVar2;
  uint *puVar3;
  uint uVar4;
  uint *puVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  undefined4 uVar8;
  uint *puVar9;
  char cVar10;
  char *pcVar11;
  short *psVar12;
  int iVar13;
  short sVar14;
  short sVar15;
  int iVar16;
  int iVar17;
  undefined1 local_b0 [4];
  uint uStack_ac;
  uint uStack_a8;
  uint uStack_a4;
  uint uStack_94;
  uint uStack_90;
  uint uStack_8c;
  undefined1 local_78 [116];

  if ((param_1 != 0xffffffff) && (param_2 != 0xffffffff)) {
    pcVar11 = param_3;
    do {
      cVar10 = *pcVar11;
      pcVar11 = pcVar11 + 1;
    } while (cVar10 != '\0');
    if (pcVar11 != param_3 + 1) {
      iVar16 = (param_1 & 0xffff) * 0xc;
      puVar3 = *(uint **)(iVar16 + 8 + *(int *)(DAT_008603b0 + 0x34));
      if ((*(byte *)((int)puVar3 + 0x106) & 4) == 0) {
        uVar4 = puVar3[0x47];
        if (((uVar4 != 0xffffffff) && ((short)puVar3[0xbc] != -1)) && (DAT_00719720 != 1)) {
          if ((uVar4 != 0xffffffff) && ((short)puVar3[0xbc] != -1)) {
            puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc);
            object_get_node_local_transform
                      (uVar4,*(int *)(*(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                                     0x2e8) + 0x24 + (short)puVar3[0xbc] * 0x11c,local_78,1);
            iVar17 = *(int *)(*(int *)((*(uint *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 +
                                                          DAT_0087bc14) + 0x34) & 0xffff) * 0x20 +
                                       0x14 + DAT_0087bc14) + 0xbc);
            uVar6 = *(undefined4 *)(iVar17 + 0x28);
            uVar7 = *(undefined4 *)(iVar17 + 0x2c);
            uVar8 = *(undefined4 *)(iVar17 + 0x30);
            if ((puVar5[0xc9] == param_1) &&
               ((*(char *)((int)puVar5 + 0x2a3) != '%' && (puVar3[0x47] != 0xffffffff)))) {
              unit_try_set_animation_state(puVar3[0x47],0x25);
            }
            iVar13 = DAT_006f1d6c;
            puVar3[0xcb] = uVar4;
            puVar3[0xcc] = *(uint *)(iVar13 + 0xc);
            if (puVar3[0xc9] == param_1) {
              puVar3[0xc9] = 0xffffffff;
            }
            if (puVar3[0xca] == param_1) {
              puVar3[0xca] = 0xffffffff;
            }
            FUN_004f6610(param_1);
            object_set_position_and_orientation(param_1,0,0);
            iVar13 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar16);
            (*(code *)PTR_matrix4x3_multiply_00696664)
                      (*(short *)(iVar13 + 0x1f2) + iVar13,iVar17 + 0x68,local_b0);
            puVar3[0x1d] = uStack_ac;
            puVar3[0x1e] = uStack_a8;
            puVar3[0x1f] = uStack_a4;
            puVar3[0x20] = uStack_94;
            puVar3[0x21] = uStack_90;
            iVar17 = DAT_008603b0;
            puVar3[0x22] = uStack_8c;
            puVar9 = *(uint **)(*(int *)(iVar17 + 0x34) + 8 + iVar16);
            iVar17 = *(int *)((*puVar9 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
            if (*(int *)(iVar17 + 0x34) != -1) {
              if ((puVar9[4] & 1) != 0) {
                object_for_each_light_attachment(0,1);
              }
              if (*(int *)(iVar17 + 0x34) != -1) {
                iVar17 = *(int *)(DAT_008603b0 + 0x34);
                puVar9[4] = puVar9[4] & 0xfffffffe;
                pbVar1 = (byte *)(iVar17 + iVar16 + 2);
                *pbVar1 = *pbVar1 | 2;
              }
            }
            *(undefined2 *)(puVar3 + 0xbc) = 0xffff;
            *(undefined1 *)((int)puVar3 + 0x2a7) = 2;
            if (puVar5[0xc9] == param_1) {
              puVar5[0xc9] = 0xffffffff;
            }
            if (puVar5[0xca] == param_1) {
              puVar5[0xca] = 0xffffffff;
            }
            FUN_0056ce30();
            FUN_0056d6a0();
            FUN_00565420(param_1);
            puVar2 = (undefined4 *)(*(short *)((int)puVar3 + 0x1ea) + 0x10 + (int)puVar3);
            *puVar2 = uVar6;
            puVar2[1] = uVar7;
            puVar2[2] = uVar8;
            if ((short)puVar3[0x2d] == 0) {
              FUN_0055add0(param_1);
            }
            object_recalculate_bounding_radius_recursive(param_1);
            cVar10 = FUN_00566910();
            if ((cVar10 == '\x01') && (iVar16 = object_try_and_get(2), iVar16 != 0)) {
              *(undefined4 *)(iVar16 + 0x5ac) = *(undefined4 *)(DAT_006f1d6c + 0xc);
            }
            if (((DAT_00719720 == 1) && (iVar16 = datum_get(), iVar16 != 0)) &&
               (*(short *)(iVar16 + 2) == -1)) {
              *(undefined4 *)(iVar16 + 0x180) = 0;
              *(undefined4 *)(iVar16 + 0x17c) = 0;
              *(undefined4 *)(iVar16 + 0x1e0) = 0;
              *(undefined4 *)(iVar16 + 0x1dc) = 0;
            }
          }
          if (puVar3[1] == 0) {
            FUN_0056c370(1);
          }
          if (((DAT_00719720 == 1) && (uVar4 = puVar3[0x86], uVar4 != 0xffffffff)) &&
             ((sVar14 = (short)uVar4, -1 < sVar14 && (sVar14 < *(short *)(DAT_0087a480 + 0x20))))) {
            psVar12 = (short *)((int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar14 +
                               *(int *)(DAT_0087a480 + 0x34));
            sVar14 = *psVar12;
            if (((sVar14 != 0) &&
                ((sVar15 = (short)(uVar4 >> 0x10), sVar15 == 0 || (sVar14 == sVar15)))) &&
               ((psVar12[1] != -1 && (DAT_0071c2d8 != 0)))) {
              player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
            }
          }
        }
        if (puVar3[0x47] == 0xffffffff) {
          iVar16 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                        (param_2 & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14 +
                           DAT_0087bc14);
          iVar17 = 0;
          if (0 < *(int *)(iVar16 + 0x2e4)) {
            iVar13 = 0;
            while( true ) {
              pcVar11 = (char *)(iVar13 * 0x11c + *(int *)(iVar16 + 0x2e8) + 4);
              iVar13 = __stricmp(param_3,pcVar11);
              if (((iVar13 == 0) && (cVar10 = FUN_0056cc10(), cVar10 == '\0')) &&
                 (((short)puVar3[0x2d] == 1 ||
                  (cVar10 = unit_set_or_test_seat_and_weapon_label(pcVar11,0,0), cVar10 != '\0'))))
              break;
              iVar17 = iVar17 + 1;
              iVar13 = (int)(short)iVar17;
              if (*(int *)(iVar16 + 0x2e4) <= iVar13) {
                return;
              }
            }
            unit_enter_vehicle_seat(param_2,iVar17);
          }
        }
      }
    }
  }
  return;
}
#endif
