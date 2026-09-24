// player_execute_pending_interaction  (Ghidra: FUN_004793a0; renamed -- the primary-mode
// handler game_engine_apply_player_interaction_message.c (this batch) dispatches to; executes
// whichever interaction is currently queued in player::interaction_type)
// address 0x4793a0, size 837 bytes
// name confidence: 0.3   rewrite confidence: 0.2
// evidence: types/game.h player (unit 0x34, interaction_type 0x28, interaction_object 0x24,
//   interaction_seat 0x2a, local_player_index 0x02); types/objects.h object (forward 0x074,
//   parent_object -- not used here but the sibling player at +0x11c pattern is); types/math.h
//   real_matrix4x3 (position 0x28); object_get_world_matrix, vector3d_cross_product,
//   player_update_history_free_all, unit_enter_vehicle_seat, unit_drop_current_weapon all
//   already established elsewhere in this module/repo. The interaction_type values dispatched
//   here (5, 8, 9, 10, 11, default-6) are this function's own switch cases; their semantic
//   names (assassinate/board/exit/pickup/etc.) are inferred from context, not independently
//   confirmed, and several fields this function touches (unit+0x2a3, object+0x32c/+0x330,
//   object+0x4cc/+0x4d1/+0x4d2) are not named by any header this module owns.
// register convention: none -- `player_index` is this function's own single stack parameter
//   (Ghidra's own `uint param_1`).
// UNSURE: essentially all of case 8/9 (vehicle board/exit) and case 0xb (a facing-relative
//   direction classification, written into undocumented fields) is preserved close to
//   literally; unit_clear_selected_equipment/unit_try_select_equipment/unit_seat_is_occupied_by_other/actor_check_vehicle_target_available/device_control_touched/unit_detach_from_seat's
//   exact effects (each called elsewhere in this module with only some of their arguments
//   visible to Ghidra).

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
extern real_vector3d object_placement_default_up; // 0x00696720 (PTR_DAT_00696720)
extern game_time_globals *game_time; // 0x006f1d6c, already established

extern void unit_clear_selected_equipment(void); // 0x56d2c0, not in this batch
extern uint8_t unit_try_select_equipment(uint32_t unit_index, datum_index target_object, uint32_t flags); // 0x56d1a0, not in this batch; UNSURE exact signature
extern void hud_post_item_message(int16_t a, uint8_t b); // 0x4ae350, not in this batch
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern uint8_t unit_seat_is_occupied_by_other(int16_t seat, datum_index *out_vehicle); // 0x566840, not in this batch; UNSURE exact signature
extern void unit_detach_from_seat(uint32_t unit_index, uint32_t a, uint32_t b, uint32_t c); // 0x56c640, not in this batch
extern void unit_enter_vehicle_seat(uint32_t vehicle_index, int16_t seat_index, uint32_t unit_index); // 0x566970
extern void player_update_history_free_all(void *queue); // 0x4e6f20
extern void device_control_touched(uint32_t unit_handle); // 0x44c090, not in this batch
extern real_matrix4x3 *object_get_world_matrix(uint32_t object_index, real_matrix4x3 *out); // 0x4f6a20
extern void vector3d_cross_product(real_vector3d *out, real_vector3d *ecx_operand, real_vector3d *stack_operand); // 0x4052c0
extern void actor_check_vehicle_target_available(uint32_t a); // 0x42b810, not in this batch
extern void game_engine_notify_player_interaction(uint32_t primary_key, uint32_t mode, uint32_t interaction_type_and_seat,
    int32_t secondary_key); // this batch, 0x478ff0

// blam-cc: stack -> player_index
// Executes `player_index`'s queued interaction (player::interaction_type). See header note:
// case 8/9 (vehicle board/exit) and case 0xb (a facing-relative direction classification) are a
// close, largely offset-preserving transcription of Ghidra's own decompilation.
uint8_t player_execute_pending_interaction(uint32_t player_index)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    datum_index unit_handle = p->unit;
    object *unit_obj = (object *)((object_header *)object_headers->data)[unit_handle & 0xffff].data;
    uint8_t handled = 0;

    switch (p->interaction_type) {
    case 5:
        unit_clear_selected_equipment();
        if (unit_try_select_equipment((uint32_t)unit_handle, p->interaction_object, 0) != 0) {
            hud_post_item_message(p->local_player_index, *(uint8_t *)((uint8_t *)p + 100));
        }
        handled = 1;
        break;

    case 8:
    case 9:
        {
            object *unit_for_gate = unit_obj;
            if (network_game_mode == 1) {
                if (unit_handle == (datum_index)0xffffffff) {
                    return 0;
                }
                unit_for_gate = object_try_and_get(unit_handle, 3);
                if (unit_for_gate == 0) {
                    return 0;
                }
            }

            {
                datum_index resolved_vehicle = (datum_index)0xffffffff;
                if (network_game_mode == 1 &&
                    unit_seat_is_occupied_by_other(p->interaction_seat, &resolved_vehicle) == 0) {
                    object *unit_reacquired = object_try_and_get(unit_handle, 3);
                    if (unit_reacquired != 0 && *((int8_t *)unit_reacquired + 0x2a3) == 0x1b) {
                        unit_detach_from_seat((uint32_t)unit_handle, 1, 1, 0);
                    }
                }

                if (unit_seat_is_occupied_by_other(p->interaction_seat, &resolved_vehicle) == 0) {
                    if (resolved_vehicle == (datum_index)0xffffffff) {
                        return 0;
                    }
                    {
                        object *resolved_obj = (object *)((object_header *)object_headers->data)[
                            (uint32_t)resolved_vehicle & 0xffff].data;
                        if (*(int32_t *)((uint8_t *)resolved_obj + 500) == -1) {
                            return 0;
                        }
                    }
                    actor_check_vehicle_target_available(1);
                    handled = 1;
                    break;
                }

                unit_enter_vehicle_seat((uint32_t)p->interaction_object, p->interaction_seat, (uint32_t)unit_handle);
                handled = 1;
                if (network_game_mode == 1) {
                    if (p->local_player_index == -1) {
                        *(int32_t *)((uint8_t *)p + 0x180) = 0;
                        *(int32_t *)((uint8_t *)p + 0x17c) = 0;
                        *(int32_t *)((uint8_t *)p + 0x1e0) = 0;
                        *(int32_t *)((uint8_t *)p + 0x1dc) = 0;
                    } else if (network_client != 0) {
                        player_update_history_free_all(*(void **)((uint8_t *)network_client + 0xf48));
                    }
                }
                goto notify;
            }
        }

    case 10:
        device_control_touched((uint32_t)unit_handle);
        break;

    case 0xb:
        {
            object *target = (object *)((object_header *)object_headers->data)[p->interaction_object & 0xffff].data;
            *(uint32_t *)((uint8_t *)unit_obj + 0x32c) = (uint32_t)p->interaction_object;
            *(int32_t *)((uint8_t *)unit_obj + 0x330) = game_time->game_time; // records the
                // current simulation tick, per types/game.h game_time_globals::game_time (+0x0c)
            if ((unit_obj->forward.k < 0.0f ? -unit_obj->forward.k : unit_obj->forward.k) <= 0.70710677f) {
                real_matrix4x3 self_matrix, target_matrix;
                real_vector3d cross;
                float d0, d1, d2;
                int8_t direction_code;

                object_get_world_matrix((uint32_t)unit_handle, &self_matrix);
                object_get_world_matrix((uint32_t)p->interaction_object, &target_matrix);
                d0 = self_matrix.position.x - target_matrix.position.x;
                d1 = self_matrix.position.y - target_matrix.position.y;
                d2 = self_matrix.position.z - target_matrix.position.z;
                // UNSURE: this cross product's EAX (out) / ECX (ecx_operand) arguments are
                // elided by Ghidra, and its result is never read by the dot-product below
                // either here or in Ghidra's own decompilation -- transcribed as a
                // side-effect-only call, exactly as decompiled.
                vector3d_cross_product(&cross, &self_matrix.forward, &object_placement_default_up);
                direction_code = (int8_t)((0.0f < d0 * unit_obj->forward.i + d1 * unit_obj->forward.j +
                    d2 * unit_obj->forward.k) + 1);
                (void)target;
                (void)cross;
                *((uint8_t *)unit_obj + 0x4cc) |= 0x10;
                *((int8_t *)unit_obj + 0x4d1) = direction_code;
            } else if (0.0f <= unit_obj->forward.k) {
                *((int8_t *)unit_obj + 0x4d1) = 3;
                *((uint8_t *)unit_obj + 0x4cc) |= 0x10;
            } else {
                *((int8_t *)unit_obj + 0x4d1) = 4;
                *((uint8_t *)unit_obj + 0x4cc) |= 0x10;
            }
            *((uint8_t *)unit_obj + 0x4d2) = 0;
        }
        handled = 1;
        break;

    default:
        return 0;
    }

    handled = 1;
notify:
    if (unit_obj->network_role == 0) {
        game_engine_notify_player_interaction(0, p->interaction_type, p->interaction_seat, -1);
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
