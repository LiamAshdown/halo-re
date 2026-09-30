// actor_squad_action_is_complete  (Ghidra: actor_squad_action_is_complete; has the current command-list atom
//   finished?)
// address 0x4066d0, size 1318 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN from objdump 0x4066d0..0x406bf5, every atom checked (the draft measured "go to and face" against
//   the unnormalized offset, fetched no position for another unit's "move in direction", took the shoot timer
//   from the atom instead of the actor variant (+0x84) and called the grenade / jump helpers without their
//   register arguments). EAX: the mode's aim record, ECX: the unit the list drives; stack (actor, command list,
//   list state). The draft's argument order is kept for the C callers.
// blam-cc: EAX -> aim_state, ECX -> check_object_index, stack -> (actor_index, command_list_index, state)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "fn_ai.h"
#include "fn_units.h"


extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern Scenario *global_scenario;   // 0x00746f8c

extern real vector3d_magnitude_squared(real_vector3d *v);                // 0x401000, EAX
extern real vector3d_distance_squared(real_point3d *a, real_point3d *b); // 0x401020, EAX a, ECX b
extern real vector2d_normalize_with_length(real_vector2d *v);            // 0x4018e0, ECX
extern real vector3d_normalize_with_length(real_vector3d *v);            // 0x401990, ECX


extern uint8_t recorded_animation_object_is_playing(datum_index unit_index); // 0x44acc0, ESI
extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, EAX out, ECX object
extern uint8_t unit_is_in_busy_animation_state(uint32_t unit_index); // 0x569c90, ECX


#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)
#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

uint8_t actor_squad_action_is_complete(uint8_t *aim_state, uint32_t actor_index, uint32_t check_object_index,
                                       int16_t command_list_index, uint8_t *state)
{
    uint8_t *act = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    ScenarioCommandList *list = &((ScenarioCommandList *)global_scenario->command_lists.pointer)[command_list_index];
    datum_index unit_index = ((actor *)act)->unit_index;
    ScenarioCommand *entry;
    uint8_t done;

    if (state[0] >= (int32_t)list->commands.count) {
        return 1;
    }
    entry = &((ScenarioCommand *)list->commands.pointer)[state[0]];

    switch (entry->atom_type) {
    case 0: case 4: case 0x16: case 0x17: case 0x18: case 0x19: // timed
        return *(int16_t *)(state + 0x2) == 0;

    case 1:   // go to
    case 2: { // go to and face
        if (check_object_index != unit_index || aim_state == 0) {
            return 1;
        }
        done = actor_movement_action_in_progress(actor_index);
        if (!done && aim_state[0x5] && aim_state[0x4]) {
            float range = actor_compute_accuracy_scale(actor_index);
            real_vector3d delta;
            float distance_squared;

            delta.i = *(float *)(aim_state + 0x8) - ((actor *)act)->body_position.x;
            delta.j = *(float *)(aim_state + 0xc) - ((actor *)act)->body_position.y;
            delta.k = *(float *)(aim_state + 0x10) - ((actor *)act)->body_position.z;
            distance_squared = vector3d_magnitude_squared(&delta);
            if (distance_squared <= range * range) {
                done = 1;
            } else if (distance_squared <= (range + 0.5f) * (range + 0.5f)) {
                // 0x4067f4: still closing in when moving towards the point (unit velocity +0x68)
                uint8_t *unit = OBJECT_DATA(unit_index);

                if (delta.k * ((unit_object *)unit)->base.velocity.k + delta.j * ((unit_object *)unit)->base.velocity.j +
                    delta.i * ((unit_object *)unit)->base.velocity.i <= 0.0f) {
                    done = 1;
                }
            }
        }
        if (entry->atom_type == 2 || entry->atom_modifier == 0) {
            if (act[0x504]) {
                *(int16_t *)(state + 0x2) = 10;
            }
            if (!done || *(int16_t *)(state + 0x2) != 0) {
                return 0;
            }
            done = 1;
        } else if (!done) {
            return 0;
        }
        if (aim_state[0x18]) {
            if (act[0x99]) { // flying: face in 3d
                real_vector3d direction;

                direction.i = *(float *)(aim_state + 0x1c) - ((actor *)act)->body_position.x;
                direction.j = *(float *)(aim_state + 0x20) - ((actor *)act)->body_position.y;
                direction.k = *(float *)(aim_state + 0x24) - ((actor *)act)->body_position.z;
                if (vector3d_normalize_with_length(&direction) > 0.0f &&
                    direction.k * ((actor *)act)->facing.k + direction.j * ((actor *)act)->facing.j +
                    direction.i * ((actor *)act)->facing.i < 0.984f) {
                    return 0;
                }
            } else {
                real_vector2d direction;

                direction.i = *(float *)(aim_state + 0x1c) - ((actor *)act)->body_position.x;
                direction.j = *(float *)(aim_state + 0x20) - ((actor *)act)->body_position.y;
                if (vector2d_normalize_with_length(&direction) > 0.0f &&
                    direction.j * ((actor *)act)->facing.j + direction.i * ((actor *)act)->facing.i < 0.984f) {
                    return 0;
                }
            }
        }
        actor_movement_action_stop(actor_index);
        return done;
    }

    case 3: { // move in direction: done once past parameter1 along the direction
        real_point3d position;

        if (check_object_index == unit_index) {
            position = *(real_point3d *)&((actor *)act)->body_position.x;
        } else {
            object_get_position(&position, check_object_index);
        }
        if ((position.x - *(float *)(state + 0x18)) * *(float *)(state + 0xc) +
            (position.y - *(float *)(state + 0x1c)) * *(float *)(state + 0x10) +
            (position.z - *(float *)(state + 0x20)) * *(float *)(state + 0x14) > entry->parameter1) {
            return 1;
        }
        return 0;
    }

    case 7: // shoot
        if (check_object_index != unit_index || aim_state == 0) {
            return 1;
        }
        if (((struct actor *)act)->unknown_60c != 2 ||
            !(vector3d_distance_squared((real_point3d *)(act + 0x610), (real_point3d *)(aim_state + 0x38)) < 0.25f)) {
            // 0x4069ec: the actor variant's burst duration (+0x84), at least 60 ticks
            int16_t ticks = (int16_t)(int32_t)(*(float *)(TAG_DATA(((actor *)act)->actor_variant_tag) + 0x84) * 30.0f);

            *(int16_t *)(state + 0x2) = ticks > 0x3c ? ticks : 0x3c;
        }
        return *(int16_t *)(state + 0x2) == 0;

    case 8: // grenade
        if (check_object_index != unit_index || aim_state == 0) {
            return 1;
        }
        if (aim_state[0x49]) {
            int16_t ticks = OBJECT_DATA(check_object_index)[0x28d] != 0 ? 0x1e : 0;

            *(int16_t *)(state + 0x2) = ticks;
            return ticks == 0;
        }
        if (!unit_is_in_busy_animation_state(check_object_index)) {
            real_point3d target = *(real_point3d *)(aim_state + 0x4c);

            if (actor_commit_grenade_toss(actor_index, &target, 0xffffffff, 0xffffffff)) {
                aim_state[0x48] = 1;
            }
        }
        return *(int16_t *)(state + 0x2) == 0;

    case 0xa:   // running jump
    case 0xb: { // targeted jump
        uint8_t landed;

        if ((state[0x5] & 4) == 0) {
            return 1;
        }
        if (check_object_index == unit_index) {
            landed = act[0x15c];
        } else {
            landed = (uint8_t)unit_get_biped_specific_value(check_object_index);
        }
        if ((state[0x5] & 8) && landed) {
            *(int16_t *)(state + 0x2) = 0;
        }
        return *(int16_t *)(state + 0x2) == 0;
    }

    case 0xd: // animate: done once the unit left the custom animation state
        return OBJECT_DATA(check_object_index)[0x2a3] != 0x1c;

    case 0xe: // recording
        return recorded_animation_object_is_playing(check_object_index) == 0;

    case 0xf: // action
        if (aim_state != 0 && aim_state[0x30]) {
            return 0;
        }
        return 1;

    case 0x10: // vocalize
        return *(int16_t *)(OBJECT_DATA(check_object_index) + 0x388) != 6;

    case 0x13: // wait
        switch (entry->atom_modifier) {
        case 0:
            return ((struct actor *)act)->alert_level > 0;
        case 1:
            return ((struct actor *)act)->alert_level >= 7;
        case 2:
            if ((state[0x4] & 8) == 0) {
                state[0x4] |= 0x10;
                return 0;
            }
            state[0x4] &= 0xe7;
            return 1;
        default:
            return 1;
        }

    default:
        return 1;
    }
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
