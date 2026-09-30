// actor_queue_point_reaction_dialogue  (Ghidra: actor_queue_point_reaction_dialogue, renamed)
// address 0x422780, size 432 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: types/ai.h actor.awareness_level/vocalization_*/mode/mode_data/alert_level/
//   vocalization_unknown_3e8; types/tags.h Actor.event_look_time_modifier[2] (0xd4/0xd8),
//   the same pair used by every sibling dialogue-queue function in this range (0x421c20,
//   0x422270, 0x422550, 0x422930, 0x422c00, 0x422ec0). Calls random_real_range (0x401050),
//   already established.
// register convention: EAX -> point (real_point3d*), ECX -> actor_index.
//   // blam-cc: EAX -> point, ECX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "fn_ai.h"
#include "fn_math.h"

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14


// Variant table paired with the sighted/recognized/directional/danger/flee/seen families.
extern int16_t actor_dialogue_variant_table_g[]; // 0x0065562c

// blam-cc: EAX -> point, ECX -> actor_index
// If the actor's awareness level is not exactly 1, and it is alert enough / hasn't queued
// too many vocalizations / isn't mid-vocalization itself / hasn't recently had a category-1
// event, queues category-1 dialogue with a randomized duration (scaled by vitality grade and
// the Actor tag's event_look_time_modifier range), carrying `point` as its payload.
void actor_queue_point_reaction_dialogue(const real_point3d *point, datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->awareness_level != 1) {
        Actor *actor_tag = (Actor *)(tag_instances[self->actor_definition_tag & 0xffff].data);

        if (self->awareness_level > 1 && self->vocalization_line < 2 &&
            (self->mode != 11 || self->mode_data.raw[3] != 0) &&
            self->vocalization_unknown_3e8 < 7) {
            float wait_scale = (self->awareness_level < 3 || self->alert_level == 0) ? 2.6f : 1.3f;

            if (actor_tag->event_look_time_modifier[0] != 0.0f || actor_tag->event_look_time_modifier[1] != 0.0f) {
                float min_scale = (actor_tag->event_look_time_modifier[0] <= 0.5f) ? 0.5f : actor_tag->event_look_time_modifier[0];
                float max_scale = (actor_tag->event_look_time_modifier[1] <= 2.0f) ? actor_tag->event_look_time_modifier[1] : 2.0f;
                wait_scale = random_real_range(min_scale, max_scale) * wait_scale;
            }

            {
                int32_t ticks = (int32_t)(wait_scale * 30.0f + 0.5f); // ROUND
                if (ticks > 0x7fff) {
                    ticks = 0x7fff;
                }

                self->vocalization_variant = actor_dialogue_variant_table_g[self->alert_level >= 4];
                self->vocalization_line = 1;
                self->vocalization_state = (int16_t)ticks;
                self->vocalization_unknown_54c = 3; // kind = 3 (point)
                self->vocalization_unknown_550 = *(uint32_t *)&point->x;
                self->vocalization_unknown_554 = *(uint32_t *)&point->y;
                self->vocalization_unknown_558 = *(uint32_t *)&point->z;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x422780):

void FUN_00422780(void)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 *in_EAX;
  uint in_ECX;
  int iVar5;
  float fVar6;
  float local_1c;
  float local_18;
  float local_14;
  undefined4 local_10;

  iVar1 = (in_ECX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if (*(short *)(iVar1 + 0x6a) != 1) {
    uVar2 = in_EAX[2];
    uVar3 = in_EAX[1];
    uVar4 = *in_EAX;
    iVar5 = *(int *)((*(uint *)(iVar1 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    local_10 = CONCAT22(local_10._2_2_,3);
    if ((((1 < *(short *)(iVar1 + 0x6a)) && (*(short *)(iVar1 + 0x544) < 2)) &&
        ((*(short *)(iVar1 + 0x6c) != 0xb || (*(char *)(iVar1 + 0x9f) != '\0')))) &&
       (*(short *)(iVar1 + 1000) < 7)) {
      local_1c = 1.3;
      if ((*(short *)(iVar1 + 0x6a) < 3) || (*(short *)(iVar1 + 0x6e) == 0)) {
        local_1c = 2.6;
      }
      if ((*(float *)(iVar5 + 0xd4) != 0.0) || (*(float *)(iVar5 + 0xd8) != 0.0)) {
        if (*(float *)(iVar5 + 0xd4) <= 0.5) {
          local_14 = 0.5;
        }
        else {
          local_14 = *(float *)(iVar5 + 0xd4);
        }
        if (*(float *)(iVar5 + 0xd8) <= 2.0) {
          local_18 = *(float *)(iVar5 + 0xd8);
        }
        else {
          local_18 = 2.0;
        }
        fVar6 = random_real_range(local_14,local_18);
        local_1c = fVar6 * local_1c;
      }
      iVar5 = (int)ROUND(local_1c * 30.0);
      if (0x7fff < iVar5) {
        iVar5 = 0x7fff;
      }
      *(undefined2 *)(iVar1 + 0x546) =
           *(undefined2 *)(&DAT_0065562c + (uint)(3 < *(short *)(iVar1 + 0x6e)) * 2);
      *(undefined2 *)(iVar1 + 0x544) = 1;
      *(short *)(iVar1 + 0x548) = (short)iVar5;
      *(undefined4 *)(iVar1 + 0x54c) = local_10;
      *(undefined4 *)(iVar1 + 0x550) = uVar4;
      *(undefined4 *)(iVar1 + 0x554) = uVar3;
      *(undefined4 *)(iVar1 + 0x558) = uVar2;
    }
  }
  return;
}
#endif
