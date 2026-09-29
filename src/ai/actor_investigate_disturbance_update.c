// actor_investigate_disturbance_update  (Ghidra: FUN_00408ba0; really: the mode 9 "board vehicle seat" update)
// address 0x408ba0, size 732 bytes
// name confidence: 0.2   rewrite confidence: 0.9
// REWRITTEN from objdump 0x408ba0..0x408e7b (the draft swapped the close / in-front flags, stored the wrong one
//   at +0xc4 and called 0x4095c0 without its register arguments). Stack: actor. Mode data at actor +0x9c:
//   +0x9c vehicle, +0xa0 seat, +0xa2 scripted, +0xa3 near-line flag, +0xa4 entered, +0xa5 done (driving),
//   +0xa6 failed, +0xa8 path failures, +0xaa stuck count, +0xac / +0xb0 stuck test time / position, +0xbc / +0xc0
//   alert radii, +0xc4 close, +0xc5 facing, +0xc6 in-front ticks, +0xc8 at entry, +0xcc approach point, +0xd8
//   approach direction, +0xe4 approach surface. Returns 1 once the mode is over (done or failed).
//   An actor that drives something is done; one that entered, or whose vehicle is gone, waits for that. The
//   boarding fails when the vehicle leaves the alert range (0x408f30), the actor makes no progress (25 world
//   units squared) over 8 checks 150 ticks apart, or the seat cannot be approached (0x4091d0). Close and facing
//   the seat (or 30 ticks in front of it) the actor's unit enters the seat (0x566970); close but not facing it
//   stops; otherwise a fresh approach point is taken while the actor wants a path (+0x4c).
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "fn_ai.h"

extern data_array *actor_data;       // 0x00880360
extern game_time_globals *game_time; // 0x006f1d6c

extern real vector3d_distance_squared(real_point3d *a, real_point3d *b); // 0x401020, src/math; blam-cc: EAX a, ECX b


extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX object, stack mask
extern uint32_t unit_enter_vehicle_seat(uint32_t vehicle_index, int16_t seat_index, uint32_t unit_index); // 0x566970, EAX unit, stack

int32_t actor_investigate_disturbance_update(uint32_t actor_index)
{
    uint8_t *act = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    void *vehicle = object_try_and_get(*(datum_index *)&((struct actor *)act)->mode_data, 2);

    if (((actor *)act)->active_unit_index != k_datum_index_none) {
        act[0xa5] = 1;
    } else if (act[0xa4] == 0) {
        if (vehicle == 0) {
            *(datum_index *)&((struct actor *)act)->mode_data = k_datum_index_none;
            act[0xa6] = 1;
        } else if (!actor_is_within_alert_range(act[0xa2] == 0, *(float *)(act + 0xbc), *(float *)(act + 0xc0), 0, 1,
                                                actor_index, *(datum_index *)&((struct actor *)act)->mode_data)) {
            act[0xa6] = 1;
        } else {
            real_point3d entry;
            real_vector3d direction;
            real_point3d hint;
            uint8_t facing;
            uint8_t close;
            uint8_t in_front;

            if (game_time->game_time >= *(int32_t *)(act + 0xac) + 150) {
                *(int32_t *)(act + 0xac) = game_time->game_time;
                if (vector3d_distance_squared((real_point3d *)(act + 0x12c), (real_point3d *)(act + 0xb0)) <= 25.0f) {
                    *(int16_t *)(act + 0xaa) += 1;
                } else {
                    *(int16_t *)(act + 0xaa) = 0;
                    *(real_point3d *)(act + 0xb0) = *(real_point3d *)&((actor *)act)->body_position.x;
                }
            }
            if (*(int16_t *)(act + 0xaa) >= 8 ||
                !actor_evaluate_search_node(actor_index, *(datum_index *)&((struct actor *)act)->mode_data, *(int16_t *)(act + 0xa0), &entry,
                                            &direction, &hint, 0, &close, &facing, &in_front)) {
                act[0xa6] = 1;
            } else {
                if (in_front) {
                    *(int16_t *)(act + 0xc6) += 1;
                    if (*(int16_t *)(act + 0xc6) >= 30) {
                        facing = 1;
                        close = 1;
                    }
                } else {
                    *(int16_t *)(act + 0xc6) = 0;
                }
                if (close) {
                    if (facing) {
                        unit_enter_vehicle_seat(*(datum_index *)&((struct actor *)act)->mode_data, *(int16_t *)(act + 0xa0),
                                                ((actor *)act)->unit_index);
                        act[0xa4] = 1;
                    } else {
                        actor_movement_action_stop(actor_index);
                    }
                } else if (act[0x4c] != 0) {
                    if (actor_avoid_obstacle_and_project(actor_index, *(datum_index *)&((struct actor *)act)->mode_data, &entry, &hint,
                                                         act + 0xa3, (real_point3d *)(act + 0xcc),
                                                         (int32_t *)(act + 0xe4)) &&
                        actor_movement_set_destination_point((real_point3d *)(act + 0xcc), actor_index,
                                                             *(int32_t *)(act + 0xe4), *(datum_index *)&((struct actor *)act)->mode_data)) {
                        *(int16_t *)(act + 0xa8) = 0;
                    } else {
                        *(int16_t *)(act + 0xa8) += 1;
                        if (*(int16_t *)(act + 0xa8) > (act[0xa2] != 0 ? 5 : 50)) {
                            act[0xa6] = 1;
                        }
                    }
                }
                act[0xc8] = (uint8_t)(vector3d_distance_squared(&entry, (real_point3d *)(act + 0x12c)) <= 1.0f);
                *(real_vector3d *)(act + 0xd8) = direction;
                act[0xc5] = facing;
                act[0xc4] = close;
            }
        }
    }
    return act[0xa5] != 0 || act[0xa6] != 0;
}

#if 0
Original Ghidra decompilation (0x408ba0):

undefined4 FUN_00408ba0(uint param_1)

{
  char cVar1;
  int iVar2;
  int iVar3;
  float10 fVar4;
  char local_2f;
  char local_2e;
  char local_2d;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined1 local_20 [12];
  undefined1 local_14 [16];

  iVar3 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar2 = object_try_and_get(2);
  if (*(int *)(iVar3 + 0x158) != -1) {
    *(undefined1 *)(iVar3 + 0xa5) = 1;
    goto LAB_00408c08;
  }
  if (*(char *)(iVar3 + 0xa4) != '\0') goto LAB_00408c08;
  if (iVar2 == 0) {
    *(undefined4 *)(iVar3 + 0x9c) = 0xffffffff;
  }
  else {
    cVar1 = actor_is_within_alert_range
                      (*(char *)(iVar3 + 0xa2) == '\0',*(undefined4 *)(iVar3 + 0xbc),
                       *(undefined4 *)(iVar3 + 0xc0),0,1);
    if (cVar1 != '\0') {
      if (*(int *)(iVar3 + 0xac) + 0x96 <= *(int *)(DAT_006f1d6c + 0xc)) {
        *(int *)(iVar3 + 0xac) = *(int *)(DAT_006f1d6c + 0xc);
        fVar4 = (float10)FUN_00401020();
        if ((float10)25.0 <= fVar4) {
          *(undefined2 *)(iVar3 + 0xaa) = 0;
          *(undefined4 *)(iVar3 + 0xb0) = *(undefined4 *)(iVar3 + 300);
          *(undefined4 *)(iVar3 + 0xb4) = *(undefined4 *)(iVar3 + 0x130);
          *(undefined4 *)(iVar3 + 0xb8) = *(undefined4 *)(iVar3 + 0x134);
        }
        else {
          *(short *)(iVar3 + 0xaa) = *(short *)(iVar3 + 0xaa) + 1;
        }
      }
      if ((*(short *)(iVar3 + 0xaa) < 8) &&
         (cVar1 = FUN_004091d0(param_1,*(undefined4 *)(iVar3 + 0x9c),*(undefined2 *)(iVar3 + 0xa0),
                               local_20,&local_2c,local_14,0,&local_2e,&local_2f,&local_2d),
         cVar1 != '\0')) {
        if (local_2d == '\0') {
          *(undefined2 *)(iVar3 + 0xc6) = 0;
LAB_00408d3f:
          if (local_2e == '\0') {
            if (*(char *)(iVar3 + 0x4c) != '\0') {
              cVar1 = FUN_004095c0(local_14,iVar3 + 0xa3,iVar3 + 0xcc,(undefined4 *)(iVar3 + 0xe4));
              if ((cVar1 == '\0') ||
                 (cVar1 = actor_movement_set_destination_point
                                    (param_1,*(undefined4 *)(iVar3 + 0xe4),
                                     *(undefined4 *)(iVar3 + 0x9c)), cVar1 == '\0')) {
                *(short *)(iVar3 + 0xa8) = *(short *)(iVar3 + 0xa8) + 1;
                if ((short)((-(ushort)(*(char *)(iVar3 + 0xa2) != '\0') & 0xffd3) + 0x32) <
                    *(short *)(iVar3 + 0xa8)) {
                  *(undefined1 *)(iVar3 + 0xa6) = 1;
                }
              }
              else {
                *(undefined2 *)(iVar3 + 0xa8) = 0;
              }
            }
          }
          else {
            if (local_2f != '\0') goto LAB_00408d4f;
            actor_movement_action_stop();
          }
        }
        else {
          *(short *)(iVar3 + 0xc6) = *(short *)(iVar3 + 0xc6) + 1;
          if (*(short *)(iVar3 + 0xc6) < 0x1e) goto LAB_00408d3f;
          local_2f = '\x01';
          local_2e = '\x01';
LAB_00408d4f:
          unit_enter_vehicle_seat(*(undefined4 *)(iVar3 + 0x9c),*(undefined2 *)(iVar3 + 0xa0));
          *(undefined1 *)(iVar3 + 0xa4) = 1;
        }
        fVar4 = (float10)FUN_00401020();
        *(bool *)(iVar3 + 200) = fVar4 < (float10)1.0;
        *(undefined4 *)(iVar3 + 0xd8) = local_2c;
        *(undefined4 *)(iVar3 + 0xdc) = local_28;
        *(undefined4 *)(iVar3 + 0xe0) = local_24;
        *(char *)(iVar3 + 0xc5) = local_2f;
        *(char *)(iVar3 + 0xc4) = local_2e;
        goto LAB_00408c08;
      }
    }
  }
  *(undefined1 *)(iVar3 + 0xa6) = 1;
LAB_00408c08:
  if ((*(char *)(iVar3 + 0xa5) == '\0') && (*(char *)(iVar3 + 0xa6) == '\0')) {
    return 0;
  }
  return 1;
}
#endif
