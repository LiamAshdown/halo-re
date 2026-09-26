// actor_investigate_disturbance_update  (Ghidra: actor_investigate_disturbance_update, renamed)
// address 0x408ba0, size 732 bytes
// name confidence: 0.4   rewrite confidence: 0.2
// evidence: types/ai.h actor.active_unit_index (0x158)/needs_new_path (0x4c); phase-4
//   summary "maintains and periodically advances the actor's 'investigating a disturbance'
//   state, abandoning the search after too many failed attempts". Every other field this
//   function touches (0xa2..0xe4) falls inside actor.mode_data (a per-mode union, see
//   types/ai.h) and is used here as this mode's own scratch record: a target prop/unit
//   handle, position-changed counters, a cached "last checked" position and tick, a
//   retry/attempt counter, and output slots for actor_evaluate_search_node
//   (actor_evaluate_search_node)'s position/direction/extra/close/facing/flag outputs.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"

extern data_array *actor_data;       // 0x00880360
extern game_time_globals *game_time; // 0x006f1d6c

extern real vector3d_distance_squared(real_point3d *a, real_point3d *b); // 0x401020, src/math; blam-cc: EAX a, ECX b
extern uint8_t actor_is_within_alert_range(uint8_t always_in_range, float radius_a, float radius_b, uint8_t vitality_only, uint8_t use_radius_b, uint32_t actor_index, uint32_t object_index); // 0x408f30, this session
extern int32_t actor_evaluate_search_node(uint32_t actor_index, uint32_t from_object, uint32_t node_table, float *out_position, float *out_direction, uint32_t *out_extra, float *out_score, uint8_t *out_close, uint8_t *out_facing, uint8_t *out_flag); // 0x4091d0, this session
extern int32_t actor_avoid_obstacle_and_project(uint32_t actor_index, real_point3d *candidate, char *out_avoided_flag, uint32_t object_index, float *out_point, uint32_t *out_extra); // 0x4095c0, this session
extern void actor_movement_action_stop(datum_index actor_index); // 0x417570, this module,
extern uint8_t actor_movement_set_destination_point(real_point3d *destination, datum_index actor_index,
                                                    int32_t parameter, uint32_t extra); // 0x417610, this module,
extern void *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX object, stack mask
extern char unit_enter_vehicle_seat(uint32_t unit_object_index, int16_t seat); // 0x566970

// FIXED (register inputs, objdump + difftest): the original never reads EAX; actor_index arrive(s) on the stack (1 stack argument(s)).
// blam-cc: stack -> actor_index
int32_t actor_investigate_disturbance_update(uint32_t actor_index)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint8_t *m = a->mode_data;
    void *self_unit_obj = object_try_and_get(*(datum_index *)m, 2); // 0x408bc5: ECX = actor+0x9c, the mode's target object

    if (a->active_unit_index != (datum_index)k_datum_index_none) {
        m[0xa5 - 0x9c] = 1;
        goto finish;
    }
    if (m[0xa4 - 0x9c] != 0) {
        goto finish;
    }

    if (self_unit_obj == 0) {
        *(uint32_t *)(m + (0x9c - 0x9c)) = 0xffffffff;
    } else {
        uint32_t target = *(uint32_t *)(m + (0x9c - 0x9c));
        uint16_t node_table = *(uint16_t *)(m + (0xa0 - 0x9c));

        if (actor_is_within_alert_range(m[0xa2 - 0x9c] == 0, *(float *)(m + (0xbc - 0x9c)), *(float *)(m + (0xc0 - 0x9c)), 0, 1, actor_index, target) != 0) {
            if (*(int32_t *)(m + (0xac - 0x9c)) + 0x96 <= game_time->game_time) {
                *(int32_t *)(m + (0xac - 0x9c)) = game_time->game_time;
                // 0x408c7a..0x408c8a: EAX = actor + 0x12c, ECX = actor + 0xb0 (the remembered position)
                if (vector3d_distance_squared(&a->body_position, (real_point3d *)(m + (0xb0 - 0x9c))) >= 25.0f) {
                    *(int16_t *)(m + (0xaa - 0x9c)) = 0;
                    *(float *)(m + (0xb0 - 0x9c)) = a->body_position.x;
                    *(float *)(m + (0xb4 - 0x9c)) = a->body_position.y;
                    *(float *)(m + (0xb8 - 0x9c)) = a->body_position.z;
                } else {
                    *(int16_t *)(m + (0xaa - 0x9c)) = *(int16_t *)(m + (0xaa - 0x9c)) + 1;
                }
            }

            if (*(int16_t *)(m + (0xaa - 0x9c)) < 8) {
                float search_position[3];  // local_20 (esp+0x20): the node's own position, compared with body_position below
                float search_direction[3]; // local_2c/local_28/local_24: written to mode_data+0xd8/0xdc/0xe0 below
                real_point3d search_extra; // local_14: reused below as the obstacle-avoidance candidate point
                uint8_t close_flag = 0, facing_flag = 0, done_flag = 0;

                if (actor_evaluate_search_node(actor_index, target, node_table, search_position, search_direction, (uint32_t *)&search_extra, 0, &close_flag, &facing_flag, &done_flag) != 0) {
                    if (done_flag == 0) {
                        *(int16_t *)(m + (0xc6 - 0x9c)) = 0;
                    have_node:
                        if (close_flag == 0) {
                            if (a->needs_new_path != 0) {
                                char avoided_flag = *(char *)(m + (0xa3 - 0x9c));
                                uint32_t projected_extra;

                                if (actor_avoid_obstacle_and_project(actor_index, &search_extra, &avoided_flag, target, (float *)(m + (0xcc - 0x9c)), &projected_extra) == 0 ||
                                    // 0x408dc0..0x408dd1: EAX = &mode_data[0x30] (actor+0xcc), then
                                    // push [actor+0x9c] / *[actor+0xe4] / ebx. Ghidra hides the EAX argument.
                                    actor_movement_set_destination_point((real_point3d *)(m + (0xcc - 0x9c)), actor_index,
                                                                         *(int32_t *)(m + (0xe4 - 0x9c)), target) == 0) {
                                    *(int16_t *)(m + (0xa8 - 0x9c)) = *(int16_t *)(m + (0xa8 - 0x9c)) + 1;
                                    {
                                        int16_t limit = (int16_t)((-(uint16_t)(m[0xa2 - 0x9c] != 0) & 0xffd3) + 0x32);
                                        if (limit < *(int16_t *)(m + (0xa8 - 0x9c))) {
                                            m[0xa6 - 0x9c] = 1;
                                        }
                                    }
                                } else {
                                    *(int16_t *)(m + (0xa8 - 0x9c)) = 0;
                                }
                            }
                        } else {
                            if (facing_flag != 0) {
                                goto enter_seat;
                            }
                            actor_movement_action_stop(actor_index);
                        }
                    } else {
                        *(int16_t *)(m + (0xc6 - 0x9c)) = *(int16_t *)(m + (0xc6 - 0x9c)) + 1;
                        if (*(int16_t *)(m + (0xc6 - 0x9c)) < 0x1e) {
                            goto have_node;
                        }
                        facing_flag = 1;
                        done_flag = 1;
                    enter_seat:
                        unit_enter_vehicle_seat(target, (int16_t)node_table);
                        m[0xa4 - 0x9c] = 1;
                    }

                    // 0x408e12..0x408e1c: EAX = &search_position (esp+0x20), ECX = actor + 0x12c
                    *(uint8_t *)(m + (200 - 0x9c)) =
                        vector3d_distance_squared((real_point3d *)search_position, &a->body_position) < 1.0f;
                    *(float *)(m + (0xd8 - 0x9c)) = search_direction[0];
                    *(float *)(m + (0xdc - 0x9c)) = search_direction[1];
                    *(float *)(m + (0xe0 - 0x9c)) = search_direction[2];
                    m[0xc5 - 0x9c] = facing_flag;
                    m[0xc4 - 0x9c] = done_flag;
                    goto finish;
                }
            }
        }
    }
    m[0xa6 - 0x9c] = 1;

finish:
    if (m[0xa5 - 0x9c] == 0 && m[0xa6 - 0x9c] == 0) {
        return 0;
    }
    return 1;
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
