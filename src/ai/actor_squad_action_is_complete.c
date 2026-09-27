// actor_squad_action_is_complete  (Ghidra: actor_squad_action_is_complete, already named)
// address 0x4066d0, size 1318 bytes
// name confidence: 0.5   rewrite confidence: 0.2
// evidence: types/ai.h actor.unit_index/unknown_504/flying (0x99)/unknown_60c; types/tags.h
//   Scenario.command_lists / ScenarioCommandList.commands / ScenarioCommand (same layout
//   confirmed in actor_squad_action_execute.c); types/objects.h object.velocity; phase-4
//   summary "tests whether the actor's current squad action-list entry has finished, based
//   on its action type's completion condition". Parameter roles (actor index, command list
//   index, state, and register-inherited aim_state/check_object_index) match
//   actor_squad_action_execute.c exactly, which is strong corroborating evidence for both
//   functions' conventions.
//
// Kept close to the Ghidra decompilation given the size and the number of per-case
// unconfirmed offsets; see actor_squad_action_execute.c's header for the general caveats
// about `state`/`aim_state`/check_object_index that also apply here.
// UNSURE: actor+0x174/0x178/0x17c is used in case 1/2 as a normalized direction (dotted
// against another normalized vector and compared to a ~10-degree cosine threshold), which
// conflicts with types/ai.h calling this offset actor.position (real_point3d) on the
// strength of actor_snapshot_orientation, a different, out-of-range function. Kept as the
// raw offset rather than resolved either way.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"

extern data_array *actor_data;  // 0x00880360
extern data_array *object_data; // 0x008603b0
extern Scenario *global_scenario; // 0x00746f8c

extern real vector3d_magnitude_squared(real_vector3d *v);              // 0x401000, src/math; blam-cc: EAX v
extern real vector3d_distance_squared(real_point3d *a, real_point3d *b); // 0x401020, src/math; blam-cc: EAX a, ECX b
extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern uint32_t actor_commit_grenade_toss(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_commit_grenade_toss at 0x411180
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern void actor_movement_action_stop(datum_index actor_index); // 0x417570, this module,
                                                                 // blam-cc: EDX -> actor_index
extern char actor_movement_action_in_progress(datum_index actor_index);      // 0x41a980, not yet rewritten
extern float actor_compute_accuracy_scale(datum_index actor_index);                          // 0x429620, not yet rewritten
extern uint8_t recorded_animation_object_is_playing(datum_index unit_index);      // 0x44acc0, src/cutscene; blam-cc: ESI unit_index
extern void object_get_position(void);                    // 0x4f6900, writes through a register-inherited pointer
extern char unit_is_in_busy_animation_state(void);                           // 0x569c90, not yet rewritten
extern char unit_get_biped_specific_value(void);                           // 0x570ad0, not yet rewritten (result in extraout_AL)

// Tests whether the actor's current squad action-list entry (command list command_list_index,
// state[0]) has finished. See the file header for scope.
uint8_t actor_squad_action_is_complete(uint8_t *aim_state, uint32_t actor_index, uint32_t check_object_index, int16_t command_list_index, uint8_t *state)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint8_t *actor_base = (uint8_t *)a;
    ScenarioCommandList *lists = (ScenarioCommandList *)global_scenario->command_lists.pointer;
    ScenarioCommandList *list = &lists[command_list_index];

    if (state[0] >= (int32_t)list->commands.count) {
        return 1;
    }

    {
        ScenarioCommand *entry = (ScenarioCommand *)((uint8_t *)list->commands.pointer + state[0] * sizeof(ScenarioCommand));

        switch (entry->atom_type) {
        case 0: case 4: case 0x16: case 0x17: case 0x18: case 0x19:
            return *(int16_t *)(state + 2) == 0;

        case 1:
        case 2:
            if (check_object_index == a->unit_index && aim_state != 0) {
                char done = actor_movement_action_in_progress(actor_index);
                uint8_t complete = 0;

                if (done == 0 && aim_state[5] != 0 && aim_state[4] != 0) {
                    float range = actor_compute_accuracy_scale(actor_index);
                    real_vector3d delta;

                    delta.i = *(float *)(aim_state + 8) - ((actor *)actor_base)->body_position.x;
                    delta.j = *(float *)(aim_state + 0xc) - ((actor *)actor_base)->body_position.y;
                    delta.k = *(float *)(aim_state + 0x10) - ((actor *)actor_base)->body_position.z;
                    {
                        float distance_sq = vector3d_magnitude_squared(&delta); // 0x406793: EAX = &delta
                        if (distance_sq < range * range) {
                            complete = 1;
                        } else if (distance_sq < (range + 0.5f) * (range + 0.5f)) {
                            object *obj = ((object_header *)object_data->data)[a->unit_index & 0xffff].data;
                            float closing = delta.i * obj->velocity.i + delta.j * obj->velocity.j + delta.k * obj->velocity.k;
                            if (closing < 0.0f) {
                                complete = 1;
                            }
                        }
                    }
                }

                if (entry->atom_type == 2 || entry->atom_modifier == 0) {
                    if (a->unknown_504 != 0) {
                        state[2] = 10;
                        state[3] = 0;
                    }
                    if (complete == 0) {
                        return 0;
                    }
                    if (*(int16_t *)(state + 2) != 0) {
                        return 0;
                    }
                    complete = 1;
                } else if (complete == 0) {
                    return 0;
                }

                if (aim_state[0x18] != 0) {
                    float facing;

                    facing = *(float *)(aim_state + 0x1c) - ((actor *)actor_base)->body_position.x;
                    if (a->flying == 0) {
                        real_vector2d dir2;
                        float dot;

                        dir2.i = facing;
                        dir2.j = *(float *)(aim_state + 0x20) - ((actor *)actor_base)->body_position.y;
                        if (vector2d_normalize_with_length(&dir2) <= 0.0f) {
                            goto arrived;
                        }
                        dot = facing * ((actor *)actor_base)->facing.i + (dir2.j) * ((actor *)actor_base)->facing.j;
                        (void)dot;
                        facing = facing * ((actor *)actor_base)->facing.i +
                                 ((*(float *)(aim_state + 0x20) - ((actor *)actor_base)->body_position.y) * ((actor *)actor_base)->facing.j);
                        if (facing < 0.984f) {
                            return 0;
                        }
                        goto arrived;
                    } else {
                        real_vector3d dir3;
                        float weighted;

                        dir3.i = facing;
                        dir3.j = *(float *)(aim_state + 0x20) - ((actor *)actor_base)->body_position.y;
                        dir3.k = *(float *)(aim_state + 0x24) - ((actor *)actor_base)->body_position.z;
                        if (vector3d_normalize_with_length(&dir3) <= 0.0f) {
                            goto arrived;
                        }
                        weighted = dir3.j * ((actor *)actor_base)->facing.j + dir3.k * ((actor *)actor_base)->facing.k;
                        if (dir3.i * ((actor *)actor_base)->facing.i + weighted < 0.984f) {
                            return 0;
                        }
                    }
                }
            arrived:
                actor_movement_action_stop(actor_index);
                return complete;
            }
            break;

        case 3: {
            real_point3d p;

            if (check_object_index == a->unit_index) {
                p.x = ((actor *)actor_base)->body_position.x;
                p.y = ((actor *)actor_base)->body_position.y;
                p.z = ((actor *)actor_base)->body_position.z;
            } else {
                object_get_position();
            }
            if ((p.x - *(float *)(state + 0x18)) * *(float *)(state + 0xc) +
                (p.y - *(float *)(state + 0x1c)) * *(float *)(state + 0x10) +
                (p.z - *(float *)(state + 0x20)) * *(float *)(state + 0x14) <= entry->parameter1) {
                return 0;
            }
            break;
        }

        case 5: case 6: case 9: case 0xc: case 0x11: case 0x12: case 0x14: case 0x15: case 0x1a: case 0x1b:
            break;

        case 7:
            if (check_object_index == a->unit_index && aim_state != 0) {
                // 0x4069d1..0x4069da: EAX = actor + 0x610 (a point here, see types/ai.h unknown_610),
                // ECX = aim_state + 0x38
                if (a->unknown_60c != 2 ||
                    vector3d_distance_squared((real_point3d *)((uint8_t *)a + 0x610), (real_point3d *)(aim_state + 0x38)) >= 0.25f) {
                    int16_t ticks = (int16_t)(entry->parameter1 * 30.0f); // UNSURE: __ftol with no visible operand, see actor_squad_action_execute.c case 0
                    if (ticks < 0x3d) {
                        ticks = 0x3c;
                    }
                    *(int16_t *)(state + 2) = ticks;
                }
                return *(int16_t *)(state + 2) == 0;
            }
            break;

        case 8:
            if (check_object_index == a->unit_index && aim_state != 0) {
                if (aim_state[0x49] == 0) {
                    char ok = unit_is_in_busy_animation_state();
                    if (ok == 0) {
                        real_point3d p;
                        p.x = *(float *)(aim_state + 0x4c);
                        p.y = *(float *)(aim_state + 0x50);
                        p.z = *(float *)(aim_state + 0x54);
                        if (actor_commit_grenade_toss(&p, 0xffffffff, 0xffffffff) != 0) {
                            aim_state[0x48] = 1;
                        }
                    }
                    return *(int16_t *)(state + 2) == 0;
                }
                {
                    object *obj = ((object_header *)object_data->data)[check_object_index & 0xffff].data;
                    uint16_t v = -(uint16_t)(*((uint8_t *)obj + 0x28d) != 0) & 0x1e;
                    *(uint16_t *)(state + 2) = v;
                    return v == 0;
                }
            }
            break;

        case 10: case 0xb:
            if ((state[5] & 4) != 0) {
                char done;
                if (check_object_index == a->unit_index) {
                    done = *(char *)(actor_base + 0x15c);
                } else {
                    unit_get_biped_specific_value();
                    done = 0; // UNSURE: original reads extraout_AL from unit_get_biped_specific_value
                }
                if ((state[5] & 8) != 0 && done != 0) {
                    state[2] = 0;
                    state[3] = 0;
                }
                return *(int16_t *)(state + 2) == 0;
            }
            break;

        case 0xd: {
            object *obj = ((object_header *)object_data->data)[check_object_index & 0xffff].data;
            return *((uint8_t *)obj + 0x2a3) != 0x1c;
        }
        case 0xe:
            return recorded_animation_object_is_playing((datum_index)check_object_index) == 0; // 0x406b3f: ESI = ECX = check_object_index
        case 0xf:
            if (aim_state != 0 && aim_state[0x30] != 0) {
                return 0;
            }
            break;
        case 0x10: {
            object *obj = ((object_header *)object_data->data)[check_object_index & 0xffff].data;
            return *(int16_t *)((uint8_t *)obj + 0x388) != 6;
        }
        case 0x13:
            if (entry->atom_modifier == 0) {
                return a->unknown_6e > 0;
            }
            if (entry->atom_modifier == 1) {
                return a->unknown_6e > 6;
            }
            if (entry->atom_modifier == 2) {
                uint8_t flags = state[4];
                if ((flags & 8) == 0) {
                    state[4] = flags | 0x10;
                    return 0;
                }
                state[4] = flags & 0xe7;
            }
            break;
        default:
            return 1;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4066d0):

bool actor_squad_action_is_complete(uint param_1,short param_2,byte *param_3)

{
  float fVar1;
  float fVar2;
  float fVar3;
  byte bVar4;
  uint uVar5;
  ushort uVar6;
  char extraout_AL;
  char cVar7;
  short sVar8;
  int in_EAX;
  int iVar9;
  uint in_ECX;
  short *psVar10;
  int iVar11;
  float10 fVar12;
  float local_c;
  float local_8;
  float local_4;

  iVar11 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar9 = param_2 * 0x60 + *(int *)(global_scenario + 0x43c);
  if ((int)(uint)*param_3 < *(int *)(iVar9 + 0x30)) {
    psVar10 = (short *)((uint)*param_3 * 0x20 + *(int *)(iVar9 + 0x34));
    switch(*psVar10) {
    case 0:
    case 4:
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19:
switchD_0040673c_caseD_0:
      return *(short *)(param_3 + 2) == 0;
    case 1:
    case 2:
      uVar5 = *(uint *)(iVar11 + 0x18);
      if ((in_ECX == uVar5) && (in_EAX != 0)) {
        cVar7 = actor_movement_action_in_progress();
        if ((cVar7 == '\0') && ((*(char *)(in_EAX + 5) != '\0' && (*(char *)(in_EAX + 4) != '\0'))))
        {
          fVar12 = (float10)FUN_00429620();
          fVar1 = (float)fVar12;
          local_c = *(float *)(in_EAX + 8) - *(float *)(iVar11 + 300);
          local_8 = *(float *)(in_EAX + 0xc) - *(float *)(iVar11 + 0x130);
          local_4 = *(float *)(in_EAX + 0x10) - *(float *)(iVar11 + 0x134);
          fVar12 = (float10)FUN_00401000();
          if ((fVar12 < (float10)fVar1 * (float10)fVar1) ||
             ((fVar12 < ((float10)fVar1 + (float10)0.5) * ((float10)fVar1 + (float10)0.5) &&
              (iVar9 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar5 & 0xffff) * 0xc),
              local_c * *(float *)(iVar9 + 0x68) +
              local_8 * *(float *)(iVar9 + 0x6c) + local_4 * *(float *)(iVar9 + 0x70) < 0.0)))) {
            cVar7 = '\x01';
          }
        }
        if ((*psVar10 == 2) || (psVar10[1] == 0)) {
          if (*(char *)(iVar11 + 0x504) != '\0') {
            param_3[2] = 10;
            param_3[3] = 0;
          }
          if (cVar7 == '\0') {
            return false;
          }
          if (*(short *)(param_3 + 2) != 0) {
            return false;
          }
          cVar7 = '\x01';
        }
        else if (cVar7 == '\0') {
          return false;
        }
        if (*(char *)(in_EAX + 0x18) != '\0') {
          fVar1 = *(float *)(in_EAX + 0x1c) - *(float *)(iVar11 + 300);
          if (*(char *)(iVar11 + 0x99) == '\0') {
            fVar2 = *(float *)(in_EAX + 0x20);
            fVar3 = *(float *)(iVar11 + 0x130);
            fVar12 = (float10)vector2d_normalize_with_length();
            if (fVar12 <= (float10)0.0) goto LAB_00406935;
            fVar2 = (fVar2 - fVar3) * *(float *)(iVar11 + 0x178);
          }
          else {
            local_8 = *(float *)(in_EAX + 0x20) - *(float *)(iVar11 + 0x130);
            local_4 = *(float *)(in_EAX + 0x24) - *(float *)(iVar11 + 0x134);
            local_c = fVar1;
            fVar12 = (float10)vector3d_normalize_with_length();
            if (fVar12 <= (float10)0.0) goto LAB_00406935;
            fVar2 = local_8 * *(float *)(iVar11 + 0x178) + local_4 * *(float *)(iVar11 + 0x17c);
            fVar1 = local_c;
          }
          if (fVar1 * *(float *)(iVar11 + 0x174) + fVar2 < 0.984) {
            return false;
          }
        }
LAB_00406935:
        actor_movement_action_stop();
        return (bool)cVar7;
      }
      break;
    case 3:
      if (in_ECX == *(uint *)(iVar11 + 0x18)) {
        local_c = *(float *)(iVar11 + 300);
        local_8 = *(float *)(iVar11 + 0x130);
        local_4 = *(float *)(iVar11 + 0x134);
      }
      else {
        object_get_position();
      }
      if ((local_c - *(float *)(param_3 + 0x18)) * *(float *)(param_3 + 0xc) +
          (local_8 - *(float *)(param_3 + 0x1c)) * *(float *)(param_3 + 0x10) +
          (local_4 - *(float *)(param_3 + 0x20)) * *(float *)(param_3 + 0x14) <=
          *(float *)(psVar10 + 2)) {
        return false;
      }
      break;
    case 5:
    case 6:
    case 9:
    case 0xc:
    case 0x11:
    case 0x12:
    case 0x14:
    case 0x15:
    case 0x1a:
    case 0x1b:
      break;
    case 7:
      if ((in_ECX == *(uint *)(iVar11 + 0x18)) && (in_EAX != 0)) {
        if ((*(short *)(iVar11 + 0x60c) != 2) ||
           (fVar12 = (float10)FUN_00401020(), (float10)0.25 <= fVar12)) {
          sVar8 = __ftol();
          if (sVar8 < 0x3d) {
            sVar8 = 0x3c;
          }
          *(short *)(param_3 + 2) = sVar8;
        }
        goto switchD_0040673c_caseD_0;
      }
      break;
    case 8:
      if ((in_ECX == *(uint *)(iVar11 + 0x18)) && (in_EAX != 0)) {
        if (*(char *)(in_EAX + 0x49) == '\0') {
          cVar7 = FUN_00569c90();
          if (cVar7 == '\0') {
            local_c = *(float *)(in_EAX + 0x4c);
            local_8 = *(float *)(in_EAX + 0x50);
            local_4 = *(float *)(in_EAX + 0x54);
            cVar7 = FUN_00411180(&local_c,0xffffffff,0xffffffff);
            if (cVar7 != '\0') {
              *(undefined1 *)(in_EAX + 0x48) = 1;
            }
          }
          return *(short *)(param_3 + 2) == 0;
        }
        uVar6 = -(ushort)(*(char *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                            (in_ECX & 0xffff) * 0xc) + 0x28d) != '\0');
        *(ushort *)(param_3 + 2) = uVar6 & 0x1e;
        return (uVar6 & 0x1e) == 0;
      }
      break;
    case 10:
    case 0xb:
      if ((param_3[5] & 4) != 0) {
        if (in_ECX == *(uint *)(iVar11 + 0x18)) {
          cVar7 = *(char *)(iVar11 + 0x15c);
        }
        else {
          FUN_00570ad0();
          cVar7 = extraout_AL;
        }
        if (((param_3[5] & 8) != 0) && (cVar7 != '\0')) {
          param_3[2] = 0;
          param_3[3] = 0;
        }
        return *(short *)(param_3 + 2) == 0;
      }
      break;
    case 0xd:
      return *(char *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc) + 0x2a3
                      ) != '\x1c';
    case 0xe:
      cVar7 = FUN_0044acc0();
      return (bool)('\x01' - (cVar7 != '\0'));
    case 0xf:
      if ((in_EAX != 0) && (*(char *)(in_EAX + 0x30) != '\0')) {
        return false;
      }
      break;
    case 0x10:
      return *(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc) +
                       0x388) != 6;
    case 0x13:
      sVar8 = psVar10[1];
      if (sVar8 == 0) {
        return 0 < *(short *)(iVar11 + 0x6e);
      }
      if (sVar8 == 1) {
        return 6 < *(short *)(iVar11 + 0x6e);
      }
      if (sVar8 == 2) {
        bVar4 = param_3[4];
        if ((bVar4 & 8) == 0) {
          param_3[4] = bVar4 | 0x10;
          return false;
        }
        param_3[4] = bVar4 & 0xe7;
      }
      break;
    default:
      goto switchD_0040673c_default;
    }
  }
switchD_0040673c_default:
  return true;
}
#endif
