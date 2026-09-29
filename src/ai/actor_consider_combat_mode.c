// actor_consider_combat_mode  (Ghidra: actor_consider_combat_mode, already named)
// address 0x401a60, size 737 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN from objdump 0x401a60..0x401d40 (the draft called unit_get_weapon_marker_indices without the unit, flag
//   or outputs). Stack: actor_index, mode, out (the 0x38-byte consideration, zeroed, +0 stamped with the game time,
//   +4 the resulting mode). Modes 4/5 succeed for a vehicle actor (+0x15e > 1). Mode 0 turns into 1 (stalking) for a
//   use_stalking_behavior (flag 0x20000) actor at alertness (+0x6e) 5 or more that is not committed (+0x378).
//   Mode 2 (melee) needs a non-swarm actor whose unit is not dead (+0x106 bit 7) and a target prop (+0x270): with a
//   melee leap range/chance (+0x388/+0x390) a leap is rolled (always when the prop is pinned +0x130 or fired
//   +0x9c, else random < chance and beyond the leap range +0x384) and turns the mode to 3; the melee animation
//   (unit_get_weapon_marker_indices, leap flag in AL) gives the strike frame and distance (suicidal actors, flag
//   0x8000000, strike at the end with no distance); then the wait threshold (at least 4, or 1.5 without a leap)
//   bounds a move to the target (0x417910) and, once moving, the path is traced (0x4029e0) for success.
// evidence: types/ai.h actor.movement_context/swarm/alert_level/combat_alert_flag/target_unit_index;
//   types/tags.h Actor.flags bit 17 use_stalking_behavior, bit 27 suicidal_melee_attack,
//   Actor.melee_leap_range/melee_leap_chance (already-named fields; this function's own
//   "leap range gates a random-chance leap" shape confirms the field identification);
//   types/objects.h object.vitality_flags bit 0x0080.
// register convention: Ghidra shows the actor index as a float first parameter (param_1) --
//   almost certainly the actor index passed in EAX and misread as a float because the same
//   stack/register slot is reused a few lines down for an actual float (the reaction-wait
//   threshold). Split here into an actor_index parameter and a separate local for that
//   float. The consideration mode is an ordinary stack parameter; the caller's result record
//   is a pointer, register unclear (kept as a normal pointer parameter, 3rd position).
//   // blam-cc: (actor index misread as float) -> actor_index, stack -> mode, stack -> out
// UNSURE: self->target_unit_index (0x270) is indexed here into prop_data (stride 0x138),
// the same discrepancy against types/ai.h's "raw unit datum_index" reading noted in
// actor_update_melee_combat_action (0x40cdf0); kept as a prop_data index, matching the code.
// The caller out-parameter is the 0x38-byte actor_combat_consideration record; the review
// pass folded it back into types/ai.h. Only the fields this function writes are named.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include <string.h>
#include "units.h"
#include "fn_ai.h"

// TYPES (folded into types/ai.h by the review pass): local model of the 0x38-byte result record actor_consider_combat_mode and its
// siblings (0x402f80, 0x403180, 0x403630, 0x40c620...) build and pass around. Only the
// offsets this function touches are named; the rest is exactly as much as size 0x38 needs.

extern data_array *actor_data;    // 0x00880360
extern data_array *prop_data;     // 0x008802c0
extern data_array *object_data;   // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c

extern real random_real(void); // 0x4019f0


extern uint8_t unit_get_weapon_marker_indices(uint32_t unit_index, uint8_t use_alternate, uint32_t out_dx_to_key_frame,
    uint32_t out_dx_total, int16_t *out_frame_count, int16_t *out_key_frame_index); // 0x5642c0, ECX, AL, stack, EBX, EDI

uint8_t actor_consider_combat_mode(uint32_t actor_index, int16_t consideration_mode, actor_combat_consideration *out)
{
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *actor_tag = (uint8_t *)tag_instances[((struct actor *)actor)->actor_definition_tag & 0xffff].data;
    uint8_t *record = (uint8_t *)out;
    int16_t mode = consideration_mode;
    uint8_t result = 1;

    memset(out, 0, 0x38);
    *(int32_t *)record = game_time->game_time;

    if (mode == 5 || mode == 4) {
        ((struct actor_combat_consideration *)record)->mode = mode;
        return ((struct actor *)actor)->movement_context > 1;
    }
    if (mode == 2) {
        uint8_t *unit;
        uint8_t *target;
        uint8_t leap = 0;
        int16_t frame_count = 0;
        int16_t key_frame = 0;
        float dx_to_key_frame = 0.0f;
        float dx_total = 0.0f;
        float wait;
        float limit;

        result = 0;
        if (actor[6] != 0) {
            goto done;
        }
        unit = (uint8_t *)((object_header *)object_data->data)[((struct actor *)actor)->unit_index & 0xffff].data;
        if ((unit[0x106] & 0x80) != 0 || ((struct actor *)actor)->target_unit_index == k_datum_index_none) {
            goto done;
        }
        target = (uint8_t *)prop_data->data + (((struct actor *)actor)->target_unit_index & 0xffff) * 0x138;
        if (*(float *)(actor_tag + 0x388) == 0.0f || ((Actor *)actor_tag)->melee_leap_chance == 0.0f) {
            record[0xa] = 0;
        } else if (target[0x130] != 0 || *(int16_t *)(target + 0x9c) > 0) {
            record[0xa] = 1;
            leap = 1;
            mode = 3;
        } else {
            leap = random_real() < ((Actor *)actor_tag)->melee_leap_chance;
            record[0xa] = leap;
            if (*(float *)(target + 0x11c) < *(float *)(actor_tag + 0x384)) {
                leap = 0;
            } else if (leap) {
                mode = 3;
            }
        }
        if (!unit_get_weapon_marker_indices(((struct actor *)actor)->unit_index, leap, (uint32_t)&dx_to_key_frame,
                (uint32_t)&dx_total, &frame_count, &key_frame)) {
            goto done;
        }
        if ((*(uint32_t *)actor_tag & 0x8000000) != 0) {
            ((struct actor_combat_consideration *)record)->position_index = frame_count;
            ((struct actor_combat_consideration *)record)->distance_delta = 0.0f;
            record[0x30] = 1;
        } else if (key_frame == 0) {
            ((struct actor_combat_consideration *)record)->position_index = (int16_t)(frame_count / 2);
            ((struct actor_combat_consideration *)record)->distance_delta = dx_total - dx_total * 0.5f;
        } else {
            ((struct actor_combat_consideration *)record)->position_index = key_frame;
            ((struct actor_combat_consideration *)record)->distance_delta = dx_total - dx_to_key_frame;
        }
        wait = actor_get_consideration_wait_threshold(actor_index, mode, out);
        ((struct actor_combat_consideration *)record)->wait_threshold = wait;
        limit = mode == 3 ? 4.0f : 1.5f;
        if (limit > wait) {
            wait = limit;
        }
        if (actor_movement_set_destination_near_target(((struct actor *)actor)->target_unit_index, actor_index, wait)) {
            actor_movement_actions_cancel(actor_index);
            if (actor_grenade_trace_from_source(actor_index, (real_point3d *)(target + 0xc8))) {
                result = 1;
            }
        }
        goto done;
    }
    if (mode == 0 && (*(uint32_t *)actor_tag & 0x20000) != 0 && ((struct actor *)actor)->alert_level >= 5 && actor[0x378] == 0) {
        mode = 1;
    }

done:
    ((struct actor_combat_consideration *)record)->mode = mode;
    return result;
}

#if 0
Original Ghidra decompilation (0x401a60):

bool actor_consider_combat_mode(float param_1,short param_2,undefined4 *param_3)

{
  float fVar1;
  short sVar2;
  uint *puVar3;
  uint uVar4;
  bool bVar5;
  undefined4 *puVar6;
  char cVar7;
  int iVar8;
  int iVar9;
  undefined4 *puVar10;
  float10 fVar11;
  float fVar12;
  short local_18;
  float local_14;
  uint *local_10;
  short local_c;
  float local_8;
  int local_4;

  puVar6 = param_3;
  uVar4 = (uint)param_1;
  iVar8 = ((uint)param_1 & 0xffff) * 0x724;
  iVar9 = iVar8 + *(int *)(DAT_00880360 + 0x34);
  puVar3 = *(uint **)((*(uint *)(iVar8 + 0x58 + *(int *)(DAT_00880360 + 0x34)) & 0xffff) * 0x20 +
                      0x14 + DAT_0087bc14);
  puVar10 = param_3;
  for (iVar8 = 0xe; iVar8 != 0; iVar8 = iVar8 + -1) {
    *puVar10 = 0;
    puVar10 = puVar10 + 1;
  }
  *param_3 = *(undefined4 *)(DAT_006f1d6c + 0xc);
  param_3._0_1_ = true;
  if ((param_2 == 5) || (param_2 == 4)) {
    sVar2 = *(short *)(iVar9 + 0x15e);
    *(short *)(puVar6 + 1) = param_2;
    return 1 < sVar2;
  }
  if (param_2 != 2) {
    if ((((param_2 == 0) && ((*puVar3 & 0x20000) != 0)) && (4 < *(short *)(iVar9 + 0x6e))) &&
       (*(char *)(iVar9 + 0x378) == '\0')) {
      *(undefined2 *)(puVar6 + 1) = 1;
      return true;
    }
    goto LAB_00401cda;
  }
  param_3._0_1_ = false;
  bVar5 = param_3._0_1_;
  param_3._0_1_ = false;
  if (*(char *)(iVar9 + 6) != '\0') goto LAB_00401cda;
  if ((*(char *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                         (*(uint *)(iVar9 + 0x18) & 0xffff) * 0xc) + 0x106) < '\0') ||
     (*(uint *)(iVar9 + 0x270) == 0xffffffff)) {
    *(undefined2 *)(puVar6 + 1) = 2;
    return false;
  }
  iVar8 = (*(uint *)(iVar9 + 0x270) & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
  local_10 = puVar3;
  local_4 = iVar8;
  if (((float)puVar3[0xe2] == 0.0) || ((float)puVar3[0xe4] == 0.0)) {
    *(undefined1 *)((int)puVar6 + 10) = 0;
  }
  else {
    if ((*(char *)(iVar8 + 0x130) == '\0') && (*(short *)(iVar8 + 0x9c) < 1)) {
      fVar12 = random_real();
      fVar1 = (float)puVar3[0xe4];
      *(bool *)((int)puVar6 + 10) = fVar12 < fVar1;
      if ((*(float *)(iVar8 + 0x11c) < (float)puVar3[0xe1]) || (fVar12 >= fVar1)) goto LAB_00401bd9;
    }
    else {
      *(undefined1 *)((int)puVar6 + 10) = 1;
    }
    param_2 = 3;
  }
LAB_00401bd9:
  cVar7 = FUN_005642c0(&local_8,&local_14);
  param_3._0_1_ = bVar5;
  if (cVar7 != '\0') {
    if ((*local_10 & 0x8000000) == 0) {
      if (local_c == 0) {
        local_8 = local_14 * 0.5;
        local_c = local_18 / 2;
      }
      *(short *)((int)puVar6 + 0x32) = local_c;
      puVar6[0xd] = local_14 - local_8;
    }
    else {
      *(short *)((int)puVar6 + 0x32) = local_18;
      puVar6[0xd] = 0;
      *(undefined1 *)(puVar6 + 0xc) = 1;
    }
    fVar11 = (float10)FUN_004028e0();
    param_1 = (float)fVar11;
    puVar6[0xb] = (float)fVar11;
    if (param_2 == 3) {
      fVar1 = 4.0;
    }
    else {
      fVar1 = 1.5;
    }
    if (param_1 < fVar1) {
      param_1 = fVar1;
    }
    cVar7 = actor_movement_set_destination_near_target(uVar4,param_1);
    if (cVar7 != '\0') {
      actor_movement_action_cancel();
      cVar7 = FUN_004029e0();
      if (cVar7 != '\0') {
        param_3._0_1_ = true;
      }
    }
  }
LAB_00401cda:
  *(short *)(puVar6 + 1) = param_2;
  return param_3._0_1_;
}
#endif
