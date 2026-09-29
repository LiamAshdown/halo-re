// actor_queue_recognized_target_dialogue  (Ghidra: actor_queue_recognized_target_dialogue, renamed)
// address 0x422550, size 556 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: types/ai.h actor.awareness_level/vocalization_*/mode/mode_data/combat_status;
//   prop.is_unit/is_vault/is_parented/unknown_54/58/5c; types/tags.h
//   Actor.event_look_time_modifier[2] (0xd4/0xd8, shared with actor_queue_sighted_target_
//   dialogue @0x421c20 and actor_queue_directional_reaction_event @0x422270). Same shape as
//   0x421c20's first half but simpler: the target here is validated directly via
//   datum_get(target_prop_index, prop_data) with no separate raw-pointer pre-check, and there
//   is no trailing visibility-cone / broadcast section. Confirmed against bin/halo.exe
//   (0x422550..0x422544 [sic, wraps into the next function's tail at 0x42264f]) that EDX/ESI
//   at the datum_get call are target_prop_index / prop_data, matching the already-established
//   datum_get signature.
// register convention: EAX -> actor_index, stack -> target_prop_index.
//   // blam-cc: EAX -> actor_index, stack -> target_prop_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "game.h"

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c
extern data_array *prop_data;       // 0x008802c0

extern real random_real_range(real min, real max); // 0x401050
extern void * datum_get(datum_index handle, data_array *array); // 0x4d0680

// Variant table paired with the two used elsewhere in this family; indexed by (combat_status>=4).
extern int16_t actor_dialogue_variant_table_c[]; // 0x0065563c

// blam-cc: EAX -> actor_index, stack -> target_prop_index
// Validates the target prop via datum_get and, if it is a unit (or a vault the actor is
// alert enough to notice), refreshes its re-notice timer as actor_queue_sighted_target_
// dialogue does, then queues category-5 dialogue with a randomized duration derived the same
// way, storing the raw target handle as the vocalization payload.
void actor_queue_recognized_target_dialogue(datum_index actor_index, datum_index target_prop_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    Actor *actor_tag = (Actor *)(tag_instances[self->actor_definition_tag & 0xffff].data);

    if (self->awareness_level > 1 && self->vocalization_line < 6 &&
        (self->mode != 11 || self->mode_data.raw[3] != 0)) {
        int16_t recent = self->vocalization_unknown_3e8;
        prop *target = (prop *)datum_get(target_prop_index, prop_data);

        if (target != 0) {
            if ((target->enemy == 0 && target->dead == 0) ||
                (target->dead != 0 && self->awareness_level > 2)) {
                if (recent > 6) {
                    return;
                }
                if (target->is_parented == 0 && target->last_attention_time != -1 &&
                    (int32_t)game_time->game_time < target->last_attention_time + 600) {
                    return;
                }
                target->last_attention_time = (int32_t)game_time->game_time;
                target->interest_satisfied = (target->interest_satisfied <= target->interest)
                                          ? target->interest
                                          : target->interest_satisfied;
            }

            {
                float wait_scale = (self->awareness_level < 3 || self->combat_status == 0) ? 1.4f : 0.7f;

                if (actor_tag->event_look_time_modifier[0] != 0.0f || actor_tag->event_look_time_modifier[1] != 0.0f) {
                    float min_scale = (actor_tag->event_look_time_modifier[0] <= 0.5f) ? 0.5f : actor_tag->event_look_time_modifier[0];
                    float max_scale = (actor_tag->event_look_time_modifier[1] <= 2.0f) ? actor_tag->event_look_time_modifier[1] : 2.0f;
                    wait_scale = random_real_range(min_scale, max_scale) * wait_scale;
                }

                int32_t ticks = (int32_t)(wait_scale * 30.0f + 0.5f); // ROUND
                if (ticks > 0x7fff) {
                    ticks = 0x7fff;
                }

                self->vocalization_state = (int16_t)ticks;
                self->vocalization_variant = actor_dialogue_variant_table_c[self->combat_status >= 4];
                self->vocalization_line = 5;
                self->vocalization_unknown_54c = 1; // kind = 1 (explicit target)
                self->vocalization_unknown_550 = target_prop_index;
                self->vocalization_unknown_554 = 0;
                self->vocalization_unknown_558 = 0;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x422550):

void FUN_00422550(undefined4 param_1)

{
  undefined4 uVar1;
  short sVar2;
  short sVar3;
  undefined2 uVar4;
  uint in_EAX;
  int iVar5;
  int iVar6;
  int iVar7;
  float fVar8;
  float local_1c;
  float local_18;
  float local_14;
  undefined4 local_10;
  undefined4 local_8;
  undefined4 local_4;

  iVar5 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  sVar2 = *(short *)(iVar5 + 0x6a);
  iVar7 = *(int *)((*(uint *)(iVar5 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_10 = CONCAT22(local_10._2_2_,1);
  if ((((1 < sVar2) && (*(short *)(iVar5 + 0x544) < 6)) &&
      ((sVar3 = *(short *)(iVar5 + 1000), *(short *)(iVar5 + 0x6c) != 0xb ||
       (*(char *)(iVar5 + 0x9f) != '\0')))) && (iVar6 = datum_get(), iVar6 != 0)) {
    if (((*(char *)(iVar6 + 0x60) == '\0') && (*(char *)(iVar6 + 0x127) == '\0')) ||
       ((*(char *)(iVar6 + 0x127) != '\0' && (2 < sVar2)))) {
      if (6 < sVar3) {
        return;
      }
      if (((*(char *)(iVar6 + 0x12e) == '\0') && (*(int *)(iVar6 + 0x5c) != -1)) &&
         (*(int *)(DAT_006f1d6c + 0xc) < *(int *)(iVar6 + 0x5c) + 600)) {
        return;
      }
      *(int *)(iVar6 + 0x5c) = *(int *)(DAT_006f1d6c + 0xc);
      if (*(float *)(iVar6 + 0x58) <= *(float *)(iVar6 + 0x54)) {
        uVar1 = *(undefined4 *)(iVar6 + 0x54);
      }
      else {
        uVar1 = *(undefined4 *)(iVar6 + 0x58);
      }
      *(undefined4 *)(iVar6 + 0x58) = uVar1;
    }
    local_1c = 0.7;
    if ((*(short *)(iVar5 + 0x6a) < 3) || (*(short *)(iVar5 + 0x6e) == 0)) {
      local_1c = 1.4;
    }
    if ((*(float *)(iVar7 + 0xd4) != 0.0) || (*(float *)(iVar7 + 0xd8) != 0.0)) {
      if (*(float *)(iVar7 + 0xd4) <= 0.5) {
        local_14 = 0.5;
      }
      else {
        local_14 = *(float *)(iVar7 + 0xd4);
      }
      if (*(float *)(iVar7 + 0xd8) <= 2.0) {
        local_18 = *(float *)(iVar7 + 0xd8);
      }
      else {
        local_18 = 2.0;
      }
      fVar8 = random_real_range(local_14,local_18);
      local_1c = fVar8 * local_1c;
    }
    iVar7 = (int)ROUND(local_1c * 30.0);
    if (0x7fff < iVar7) {
      iVar7 = 0x7fff;
    }
    uVar4 = *(undefined2 *)(&DAT_0065563c + (uint)(3 < *(short *)(iVar5 + 0x6e)) * 2);
    *(short *)(iVar5 + 0x548) = (short)iVar7;
    *(undefined2 *)(iVar5 + 0x546) = uVar4;
    *(undefined2 *)(iVar5 + 0x544) = 5;
    *(undefined4 *)(iVar5 + 0x54c) = local_10;
    *(undefined4 *)(iVar5 + 0x550) = param_1;
    *(undefined4 *)(iVar5 + 0x554) = local_8;
    *(undefined4 *)(iVar5 + 0x558) = local_4;
  }
  return;
}
#endif
