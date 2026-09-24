// actor_consider_combat_mode  (Ghidra: actor_consider_combat_mode, already named)
// address 0x401a60, size 737 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: types/ai.h actor.unknown_15e/swarm/unknown_6e/unknown_378/target_unit_index;
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

// TYPES (folded into types/ai.h by the review pass): local model of the 0x38-byte result record actor_consider_combat_mode and its
// siblings (0x402f80, 0x403180, 0x403630, 0x40c620...) build and pass around. Only the
// offsets this function touches are named; the rest is exactly as much as size 0x38 needs.

extern data_array *actor_data;    // 0x00880360
extern data_array *prop_data;     // 0x008802c0
extern data_array *object_data;   // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c

extern real random_real(void); // 0x4019f0
extern float actor_get_consideration_wait_threshold(uint32_t actor_index, int16_t mode, actor_combat_consideration *consideration); // 0x4028e0
extern int32_t actor_grenade_trace_from_source(uint32_t actor_index, real_point3d *target_point); // 0x4029e0
extern uint8_t actor_movement_set_destination_near_target(datum_index target_prop_index, datum_index actor_index,
                                                          float radius); // 0x417910, this module,
                                                          // blam-cc: EAX -> target_prop_index, stack -> the other two
extern void actor_movement_actions_cancel(datum_index actor_index); // 0x417a30
// 0x5642c0, a different module, not rewritten here: fills the two out-floats used below.
extern uint8_t FUN_005642c0(float *out_a, float *out_b);

// Evaluates whether the actor should switch to a new combat sub-mode (grenade, search,
// guard, engage) for consideration_mode, and if so fills in out. Returns whether the caller
// should commit to the resulting order.
uint8_t actor_consider_combat_mode(uint32_t actor_index, int16_t consideration_mode, actor_combat_consideration *out)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    Actor *actor_def = (Actor *)tag_instances[a->actor_definition_tag & 0xffff].data;
    uint8_t result;
    int16_t mode = consideration_mode;

    memset(out, 0, sizeof(*out));
    out->game_tick = game_time->game_time;
    result = 1;

    if (mode == 5 || mode == 4) {
        out->mode = mode;
        return a->unknown_15e > 1;
    }

    if (mode != 2) {
        if (mode == 0 && (actor_def->flags & (1u << 17)) != 0 /* use_stalking_behavior */ &&
            a->unknown_6e > 4 && a->unknown_378 == 0) {
            out->mode = 1;
            return 1;
        }
        goto write_mode;
    }

    {
        uint8_t committed_engage = 0; // bVar5: the outcome of the swarm/target validity gate
        result = 0;

        if (a->swarm != 0) {
            goto write_mode;
        }

        {
            object *target_obj = ((object_header *)object_data->data)[a->unit_index & 0xffff].data;
            if ((target_obj->vitality_flags & 0x0080) != 0 || a->target_unit_index == (datum_index)k_datum_index_none) {
                out->mode = 2;
                return 0;
            }
        }

        {
            prop *target_prop = &((prop *)prop_data->data)[a->target_unit_index & 0xffff];

            if (actor_def->melee_leap_range[0] == 0.0f || actor_def->melee_leap_chance == 0.0f) {
                out->grenade_eligible = 0;
            } else {
                if (target_prop->unknown_130 == 0 && target_prop->unknown_9c < 1) {
                    float roll = random_real();
                    float chance = actor_def->melee_leap_chance;
                    out->grenade_eligible = roll < chance;
                    if (target_prop->distance < actor_def->melee_leap_range[0] || roll >= chance) {
                        goto after_leap_check;
                    }
                } else {
                    out->grenade_eligible = 1;
                }
                mode = 3;
            }
        }
    after_leap_check:
        {
            float local_8, local_14;
            int16_t local_18 = 0, local_c = 0; // UNSURE: FUN_005642c0 (out-of-range) does not
                                                // visibly initialize these two shorts; Ghidra
                                                // shows them read uninitialized on some paths,
                                                // kept zero-initialized here defensively.
            uint8_t ok = FUN_005642c0(&local_8, &local_14);

            result = committed_engage;
            if (ok) {
                if ((actor_def->flags & (1u << 27)) == 0 /* suicidal_melee_attack */) {
                    if (local_c == 0) {
                        local_8 = local_14 * 0.5f;
                        local_c = local_18 / 2;
                    }
                    out->position_index = local_c;
                    out->distance_delta = local_14 - local_8;
                } else {
                    out->position_index = local_18;
                    out->distance_delta = 0.0f;
                    out->suicidal = 1;
                }

                {
                    float threshold = actor_get_consideration_wait_threshold(actor_index, mode, out);
                    float limit = (mode == 3) ? 4.0f : 1.5f;

                    out->wait_threshold = threshold;
                    if (threshold < limit) {
                        threshold = limit;
                    }
                    // 0x401ca1: EAX = actor.target_unit_index (the prop handle), then push
                    // ecx (the radius) / ebx (the actor index). Ghidra hides the EAX argument.
                    if (actor_movement_set_destination_near_target(a->target_unit_index, actor_index, threshold)) {
                        actor_movement_actions_cancel(actor_index);
                        // UNSURE: Ghidra shows this call with no visible arguments (the
                        // target point rides in ESI, inherited from earlier in this
                        // function without a traceable assignment); the actor's current
                        // target's last known position is the most plausible candidate.
                        {
                            prop *target_prop = &((prop *)prop_data->data)[a->target_unit_index & 0xffff];
                            if (actor_grenade_trace_from_source(actor_index, &target_prop->last_known_position)) {
                                result = 1;
                            }
                        }
                    }
                }
            }
        }
    }

write_mode:
    out->mode = mode;
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
