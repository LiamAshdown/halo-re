// player_execute_pending_interaction  (Ghidra: FUN_004793a0)
// address 0x4793a0, size 837 bytes
// name confidence: 0.3   rewrite confidence: 0.85
// REWRITTEN from objdump 0x4793a0..0x4796e4 (jump table 0x4796e8 on the interaction type - 5). Stack: player.
//   5 equipment: clears the selection (0x56d2c0) and selects the object (0x56d1a0), showing it on the HUD.
//   8 / 9 vehicle seat: a client first needs its unit and drops a stale seat-exit animation (0x1b); if the seat
//   is free (0x566840) the unit enters it (0x566970) and a client drops the local player's prediction history
//   (or zeroes the remote player's); otherwise an AI-driven occupant is asked to make room (0x42b810).
//   10 device control: 0x44c090 on the device (the unit is pushed but never read).
//   11 melee interaction: the unit records the target (+0x32c) and the tick (+0x330); the TARGET gets +0x4cc
//   bit 0x10, +0x4d2 = 0 and a direction +0x4d1: 3 / 4 for a target facing up / down (|forward.k| > 0x673258),
//   else 2 / 1 for the unit on the target's left / right ((target - unit) x up against the target's forward).
//   6 / 7 and anything else return 0. A handled interaction of an authoritative unit notifies the game engine
//   (0x478ff0: ECX player, EDI the object, stack 0, type, seat, -1).
// blam-cc: stack -> player_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_headers; // 0x008603b0
extern int16_t network_game_mode;  // 0x00719720
extern uint8_t *network_client;    // 0x0071c2d8
extern real_vector3d *global_up3d_pointer; // 0x00696720
extern game_time_globals *game_time; // 0x006f1d6c

extern double fabs(double x);

extern void unit_clear_selected_equipment(uint32_t unit_index); // 0x56d2c0, ECX
extern uint8_t unit_try_select_equipment(uint32_t unit_index, uint32_t new_equipment_object_index,
    int16_t release_current); // 0x56d1a0
extern void hud_post_item_message(int16_t count, int32_t source, uint8_t kind, int16_t local_player_index,
    int8_t machine_id); // 0x4ae350, EAX, ECX, DL, stack
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern uint8_t unit_seat_is_occupied_by_other(uint32_t self_index, int16_t seat_index, uint32_t vehicle_index,
    uint32_t *out_occupant_index); // 0x566840, EAX, EDX vehicle, stack (seat, out)
extern void unit_detach_from_seat(uint32_t unit_index, uint8_t suppress_trigger, uint8_t require_client_flag,
    uint8_t fire_trigger_event); // 0x56c640
extern uint32_t unit_enter_vehicle_seat(uint32_t vehicle_index, int16_t seat_index, uint32_t unit_index); // 0x566970, EAX unit, stack
extern void player_update_history_free_all(void *queue); // 0x4e6f20
extern void device_control_touched(uint32_t object_index); // 0x44c090, EAX
extern real_matrix4x3 *object_get_world_matrix(uint32_t object_index, real_matrix4x3 *out); // 0x4f6a20, EAX, EDI
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0, EAX, ECX, stack
extern uint8_t actor_check_vehicle_target_available(datum_index vehicle_object_index, datum_index actor_index,
    uint8_t flag_pursue); // 0x42b810, EAX, ECX, stack
extern void game_engine_notify_player_interaction(uint32_t primary_key, uint32_t edi_key,
    uint32_t mode, int32_t interaction_type, int32_t interaction_seat, int32_t secondary_key); // 0x478ff0, ECX, EDI, stack

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_headers->data)[(h) & 0xffff].data)

uint8_t player_execute_pending_interaction(uint32_t player_index)
{
    uint8_t *record = (uint8_t *)player_data->data + (player_index & 0xffff) * 0x200;
    datum_index unit_index = *(datum_index *)(record + 0x34);
    uint8_t *unit = OBJECT_DATA(unit_index);
    datum_index target_index = *(datum_index *)(record + 0x24);
    uint16_t seat = *(uint16_t *)(record + 0x2a);
    uint8_t handled = 0;

    switch (*(int16_t *)(record + 0x28)) {
    case 5:
        unit_clear_selected_equipment(unit_index);
        if (unit_try_select_equipment(unit_index, target_index, 0)) {
            hud_post_item_message(0, (int32_t)*(datum_index *)OBJECT_DATA(target_index), 0,
                *(int16_t *)(record + 0x2), (int8_t)record[0x64]);
        }
        break;
    case 8:
    case 9: {
        uint32_t occupant = k_datum_index_none;

        if (network_game_mode == 1 &&
            (unit_index == k_datum_index_none || object_try_and_get(unit_index, 3) == 0)) {
            return 0;
        }
        if (network_game_mode == 1 && !unit_seat_is_occupied_by_other(unit_index, (int16_t)seat, target_index,
                                                                      &occupant)) {
            datum_index self_index = *(datum_index *)(record + 0x34);
            uint8_t *self = (uint8_t *)object_try_and_get(self_index, 3);

            if (self != 0 && self[0x2a3] == 0x1b) {
                unit_detach_from_seat(self_index, 1, 1, 0);
            }
        }
        if (unit_seat_is_occupied_by_other(*(datum_index *)(record + 0x34), (int16_t)*(uint16_t *)(record + 0x2a),
                                           *(datum_index *)(record + 0x24), &occupant)) {
            unit_enter_vehicle_seat(*(datum_index *)(record + 0x24), (int16_t)*(uint16_t *)(record + 0x2a),
                *(datum_index *)(record + 0x34));
            handled = 1;
            if (network_game_mode == 1) {
                if (*(int16_t *)(record + 0x2) != -1) {
                    if (network_client != 0) {
                        player_update_history_free_all(*(void **)(network_client + 0xf48));
                    }
                } else {
                    *(int32_t *)(record + 0x180) = 0;
                    *(int32_t *)(record + 0x17c) = 0;
                    *(int32_t *)(record + 0x1e0) = 0;
                    *(int32_t *)(record + 0x1dc) = 0;
                }
            }
            goto notify;
        }
        if (occupant == k_datum_index_none ||
            *(datum_index *)(OBJECT_DATA(occupant) + 0x1f4) == k_datum_index_none) {
            return 0;
        }
        actor_check_vehicle_target_available(*(datum_index *)(record + 0x34),
            *(datum_index *)(OBJECT_DATA(occupant) + 0x1f4), 1);
        break;
    }
    case 10:
        device_control_touched(target_index);
        break;
    case 11: {
        uint8_t *target = OBJECT_DATA(target_index);
        int8_t direction;

        *(datum_index *)(unit + 0x32c) = target_index;
        *(int32_t *)(unit + 0x330) = game_time->game_time;
        if (fabs(*(float *)(target + 0x7c)) > 0.7071067690849304) { // 0x673258 (double)
            direction = (int8_t)((*(float *)(target + 0x7c) < 0.0f) ? 4 : 3);
        } else {
            real_matrix4x3 target_matrix;
            real_matrix4x3 unit_matrix;
            real_point3d *target_position =
                (real_point3d *)((uint8_t *)object_get_world_matrix(target_index, &target_matrix) + 0x28);
            real_point3d *unit_position =
                (real_point3d *)((uint8_t *)object_get_world_matrix(*(datum_index *)(record + 0x34), &unit_matrix) + 0x28);
            real_vector3d side;

            side.i = target_position->x - unit_position->x;
            side.j = target_position->y - unit_position->y;
            side.k = target_position->z - unit_position->z;
            vector3d_cross_product(&side, &side, global_up3d_pointer);
            direction = (int8_t)((side.k * *(float *)(target + 0x7c) + side.j * *(float *)(target + 0x78) +
                                  side.i * *(float *)(target + 0x74) > 0.0f) ? 2 : 1);
        }
        target[0x4cc] |= 0x10;
        target[0x4d1] = (uint8_t)direction;
        target[0x4d2] = 0;
        break;
    }
    default:
        return 0;
    }
    handled = 1;
notify:
    if (*(int32_t *)(unit + 0x4) == 0) {
        game_engine_notify_player_interaction(player_index, *(datum_index *)(record + 0x24), 0,
            *(uint16_t *)(record + 0x28), *(uint16_t *)(record + 0x2a), -1);
    }
    return handled;
}

#if 0
Original Ghidra decompilation (0x4793a0), from tools/pack.py 0x4793a0:

undefined1 FUN_004793a0(uint param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  uint uVar7;
  int iVar8;
  undefined4 uVar9;
  char cVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  float *local_c;
  undefined1 local_5;

  iVar13 = DAT_008603b0;
  iVar12 = DAT_006f1d6c;
  iVar14 = (param_1 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  uVar7 = *(uint *)(iVar14 + 0x34);
  iVar11 = (uVar7 & 0xffff) * 0xc;
  iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar11);
  local_5 = 0;
  switch(*(undefined2 *)(iVar14 + 0x28)) {
  case 5:
    FUN_0056d2c0();
    cVar10 = FUN_0056d1a0(*(undefined4 *)(iVar14 + 0x34),*(undefined4 *)(iVar14 + 0x24),0);
    if (cVar10 != '\0') {
      FUN_004ae350(*(undefined2 *)(iVar14 + 2),*(undefined1 *)(iVar14 + 100));
    }
    break;
  default:
    goto switchD_004793f6_caseD_6;
  case 8:
  case 9:
    if (DAT_00719720 == 1) {
      if (uVar7 == 0xffffffff) {
        return 0;
      }
      iVar12 = object_try_and_get(3);
      if (iVar12 == 0) {
        return local_5;
      }
    }
    local_c = (float *)0xffffffff;
    if ((DAT_00719720 == 1) &&
       (cVar10 = FUN_00566840(*(undefined2 *)(iVar14 + 0x2a),&local_c), cVar10 == '\0')) {
      uVar9 = *(undefined4 *)(iVar14 + 0x34);
      iVar12 = object_try_and_get(3);
      if ((iVar12 != 0) && (*(char *)(iVar12 + 0x2a3) == '\x1b')) {
        FUN_0056c640(uVar9,1,1,0);
        iVar13 = DAT_008603b0;
      }
    }
    cVar10 = FUN_00566840(*(undefined2 *)(iVar14 + 0x2a),&local_c);
    if (cVar10 == '\0') {
      if (local_c == (float *)0xffffffff) {
        return local_5;
      }
      if (*(int *)(*(int *)(*(int *)(iVar13 + 0x34) + 8 + ((uint)local_c & 0xffff) * 0xc) + 500) ==
          -1) {
        return local_5;
      }
      FUN_0042b810(1);
      break;
    }
    unit_enter_vehicle_seat(*(undefined4 *)(iVar14 + 0x24),*(undefined2 *)(iVar14 + 0x2a));
    local_5 = 1;
    if (DAT_00719720 == 1) {
      if (*(short *)(iVar14 + 2) == -1) {
        *(undefined4 *)(iVar14 + 0x180) = 0;
        *(undefined4 *)(iVar14 + 0x17c) = 0;
        *(undefined4 *)(iVar14 + 0x1e0) = 0;
        *(undefined4 *)(iVar14 + 0x1dc) = 0;
      }
      else if (DAT_0071c2d8 != 0) {
        player_update_history_free_all(*(undefined4 *)(DAT_0071c2d8 + 0xf48));
      }
    }
    goto LAB_004796b1;
  case 10:
    FUN_0044c090(uVar7);
    break;
  case 0xb:
    iVar13 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar11);
    iVar11 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar14 + 0x24) & 0xffff) * 0xc)
    ;
    *(uint *)(iVar13 + 0x32c) = *(uint *)(iVar14 + 0x24);
    *(undefined4 *)(iVar13 + 0x330) = *(undefined4 *)(iVar12 + 0xc);
    if (ABS(*(float *)(iVar11 + 0x7c)) <= 0.70710677) {
      iVar12 = object_get_world_matrix();
      local_c = (float *)(iVar12 + 0x28);
      iVar12 = object_get_world_matrix();
      fVar1 = *local_c;
      fVar2 = *(float *)(iVar12 + 0x28);
      fVar3 = local_c[1];
      fVar4 = *(float *)(iVar12 + 0x2c);
      fVar5 = local_c[2];
      fVar6 = *(float *)(iVar12 + 0x30);
      vector3d_cross_product(PTR_DAT_00696720);
      cVar10 = (0.0 < (fVar1 - fVar2) * *(float *)(iVar11 + 0x74) +
                      (fVar3 - fVar4) * *(float *)(iVar11 + 0x78) +
                      (fVar5 - fVar6) * *(float *)(iVar11 + 0x7c)) + '\x01';
    }
    else if (0.0 <= *(float *)(iVar11 + 0x7c)) {
      cVar10 = '\x03';
    }
    else {
      cVar10 = '\x04';
    }
    *(byte *)(iVar11 + 0x4cc) = *(byte *)(iVar11 + 0x4cc) | 0x10;
    *(char *)(iVar11 + 0x4d1) = cVar10;
    *(undefined1 *)(iVar11 + 0x4d2) = 0;
  }
  local_5 = 1;
LAB_004796b1:
  if (*(int *)(iVar8 + 4) == 0) {
    FUN_00478ff0(0,*(undefined2 *)(iVar14 + 0x28),*(undefined2 *)(iVar14 + 0x2a),0xffffffff);
  }
switchD_004793f6_caseD_6:
  return local_5;
}
#endif
