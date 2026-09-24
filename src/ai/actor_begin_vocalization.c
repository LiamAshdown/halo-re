// actor_begin_vocalization  (Ghidra: actor_begin_vocalization, renamed)
// address 0x4142d0, size 647 bytes
// name confidence: 0.55  rewrite confidence: 0.55
// evidence: the three fields it commits are exactly the trio types/ai.h attributes to
//   actor_clear_vocalization @0x414560 -- vocalization_line (0x544), vocalization_variant
//   (0x546) and vocalization_state (0x548) -- plus the four dwords at 0x54c..0x558. The two
//   tables it indexes by line number were read out of bin/halo.exe and both hold exactly 14
//   rows, which is what fixes the line count at 14 (0..13).
// register convention: actor_index in EAX; line, variant and the context block are the
//   Ghidra-recognized stack parameters. datum_get @0x4d0680 is called with no visible
//   arguments -- see UNSURE below.
//
// UNSURE (naming, for the hook pass): the duration randomizer this reads is the actor tag
// field invader calls event_look_time_modifier (Actor+0xd4, a min / max float pair), and the
// per-line table at 0x00655660 holds seconds that are multiplied by 30 to get ticks. That
// makes the whole 0x544..0x55b record look like a timed look-at / reaction event rather than
// a spoken line, and the vocalization_ field names types/ai.h carries for it -- taken from
// actor_clear_vocalization @0x414560 -- would then be wrong. The names are left alone here
// so the module stays self-consistent; resolving it is a hook-verification question.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "ai.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *prop_data;       // 0x008802c0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c

// The two per-line tables. Both are 14 rows long: 0x00655628 ends exactly where 0x00655660
// begins, and 0x00655660 ends exactly where the unrelated float run at 0x00655698 begins.
extern float actor_vocalization_duration[14];  // 0x00655660
extern int16_t actor_vocalization_variant[14][2]; // 0x00655628, second column for unknown_6e > 3

extern real random_real_range(real min, real max); // 0x401050
extern void * datum_get(datum_index handle, data_array *array); // 0x4d0680, matches src/memory/datum_get.c


// blam-cc: EAX -> actor_index, stack -> line, variant, context
// Tries to start a vocalization line. Refuses while the actor is not alert enough, while a
// higher-numbered line is already playing, while fleeing without the mode_data flag set, and
// -- for the prop-directed lines below 8 -- while the same prop was spoken about inside the
// last 600 ticks. On success it picks a per-line duration in ticks, scaled by the actor tag
// randomization range at Actor+0xd4 / +0xd8, and commits the line, variant, duration and the
// caller context block onto the actor. Returns 1 when the line was started.
uint8_t actor_begin_vocalization(datum_index actor_index, int16_t line, int16_t variant,
                                 actor_vocalization_context *context)
{
    actor *self;
    Actor *actor_definition;
    prop *target;
    int16_t awareness;
    uint8_t urgent;
    float duration;
    float low;
    float high;
    int32_t ticks;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    awareness = self->awareness_level;
    actor_definition = (Actor *)tag_instances[self->actor_definition_tag & 0xffff].data;

    if ((awareness < 2 && line < 13) || line < self->vocalization_line) {
        return 0;
    }

    urgent = (uint8_t)(self->vocalization_unknown_3e8 > 6);

    if ((line < 13 && self->mode == _actor_mode_flee && self->mode_data[3] == 0) ||
        (urgent != 0 && line < 4)) {
        return 0;
    }

    if (context->kind == 1) {
        // UNSURE: Ghidra shows a bare datum_get(). The record it returns is read at the
        // prop offsets 0x54, 0x58, 0x5c, 0x60, 0x127 and 0x12e, so the array is prop_data
        // and the handle is the one in the context block.
        target = (prop *)datum_get(context->handle, prop_data);
        if (target == (prop *)0) {
            return 0;
        }
        if (line < 8) {
            if ((target->is_unit == 0 && target->is_vault == 0) ||
                (target->is_vault != 0 && awareness > 2)) {
                if (urgent != 0 ||
                    (((target->is_parented == 0 || line < 4) && target->unknown_5c != -1) &&
                     game_time->game_time < target->unknown_5c + 600)) {
                    return 0;
                }
                target->unknown_5c = game_time->game_time;
                if (target->unknown_58 <= target->unknown_54) {
                    target->unknown_58 = target->unknown_54;
                }
            }
        }
    }

    duration = actor_vocalization_duration[line];
    if (self->awareness_level < 3 || self->unknown_6e == 0) {
        duration = duration + duration;
    }

    if (actor_definition->event_look_time_modifier[0] != 0.0f || actor_definition->event_look_time_modifier[1] != 0.0f) {
        low = (actor_definition->event_look_time_modifier[0] <= 0.5f) ? 0.5f : actor_definition->event_look_time_modifier[0];
        high = (actor_definition->event_look_time_modifier[1] <= 2.0f) ? actor_definition->event_look_time_modifier[1] : 2.0f;
        duration = random_real_range(low, high) * duration;
    }

    ticks = (int32_t)(duration * 30.0f + 0.5f);
    if (ticks > 0x7fff) {
        ticks = 0x7fff;
    }

    if (variant == 1) {
        variant = actor_vocalization_variant[line][self->unknown_6e > 3 ? 1 : 0];
    }

    self->vocalization_line = line;
    self->vocalization_state = (int16_t)ticks;
    self->vocalization_variant = variant;
    self->vocalization_unknown_54c = ((uint32_t *)context)[0];
    self->vocalization_unknown_550 = ((uint32_t *)context)[1];
    self->vocalization_unknown_554 = ((uint32_t *)context)[2];
    self->vocalization_unknown_558 = ((uint32_t *)context)[3];
    return 1;
}

#if 0
Original Ghidra decompilation (0x4142d0):

uint FUN_004142d0(short param_1,short param_2,short *param_3)

{
  undefined4 uVar1;
  short sVar2;
  bool bVar3;
  uint in_EAX;
  int iVar4;
  int iVar5;
  undefined3 uVar8;
  uint uVar6;
  int iVar7;
  float fVar9;
  float local_c;
  float local_8;
  float local_4;

  iVar4 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  sVar2 = *(short *)(iVar4 + 0x6a);
  iVar7 = *(int *)((*(uint *)(iVar4 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (((sVar2 < 2) && (uVar6 = DAT_0087bc14, param_1 < 0xd)) ||
     (uVar6 = CONCAT22((short)(DAT_0087bc14 >> 0x10),param_1), param_1 < *(short *)(iVar4 + 0x544)))
  {
    return uVar6 & 0xffffff00;
  }
  bVar3 = 6 < *(short *)(iVar4 + 1000);
  if ((((param_1 < 0xd) && (*(short *)(iVar4 + 0x6c) == 0xb)) && (*(char *)(iVar4 + 0x9f) == '\0'))
     || ((bVar3 && (param_1 < 4)))) {
    return uVar6 & 0xffffff00;
  }
  if (*param_3 == 1) {
    iVar5 = datum_get();
    uVar6 = 0;
    if (iVar5 == 0) {
LAB_00414385:
      return uVar6 & 0xffffff00;
    }
    if (param_1 < 8) {
      uVar8 = (undefined3)((uint)iVar5 >> 8);
      uVar6 = CONCAT31(uVar8,*(char *)(iVar5 + 0x60));
      if (((*(char *)(iVar5 + 0x60) == '\0') &&
          (uVar6 = CONCAT31(uVar8,*(char *)(iVar5 + 0x127)), *(char *)(iVar5 + 0x127) == '\0')) ||
         ((uVar6 = CONCAT31((int3)(uVar6 >> 8),*(char *)(iVar5 + 0x127)),
          *(char *)(iVar5 + 0x127) != '\0' && (2 < sVar2)))) {
        if ((bVar3) ||
           ((((*(char *)(iVar5 + 0x12e) == '\0' || (param_1 < 4)) && (*(int *)(iVar5 + 0x5c) != -1))
            && (uVar6 = *(int *)(iVar5 + 0x5c) + 600, *(int *)(DAT_006f1d6c + 0xc) < (int)uVar6))))
        goto LAB_00414385;
        *(int *)(iVar5 + 0x5c) = *(int *)(DAT_006f1d6c + 0xc);
        if (*(float *)(iVar5 + 0x58) <= *(float *)(iVar5 + 0x54)) {
          uVar1 = *(undefined4 *)(iVar5 + 0x54);
        }
        else {
          uVar1 = *(undefined4 *)(iVar5 + 0x58);
        }
        *(undefined4 *)(iVar5 + 0x58) = uVar1;
      }
    }
  }
  local_c = *(float *)(&DAT_00655660 + param_1 * 4);
  if ((*(short *)(iVar4 + 0x6a) < 3) || (*(short *)(iVar4 + 0x6e) == 0)) {
    local_c = local_c + local_c;
  }
  if ((*(float *)(iVar7 + 0xd4) != 0.0) || (*(float *)(iVar7 + 0xd8) != 0.0)) {
    if (*(float *)(iVar7 + 0xd4) <= 0.5) {
      local_4 = 0.5;
    }
    else {
      local_4 = *(float *)(iVar7 + 0xd4);
    }
    if (*(float *)(iVar7 + 0xd8) <= 2.0) {
      local_8 = *(float *)(iVar7 + 0xd8);
    }
    else {
      local_8 = 2.0;
    }
    fVar9 = random_real_range(local_4,local_8);
    local_c = fVar9 * local_c;
  }
  iVar7 = (int)ROUND(local_c * 30.0);
  if (0x7fff < iVar7) {
    iVar7 = 0x7fff;
  }
  if (param_2 == 1) {
    param_2 = *(short *)(&DAT_00655628 + ((uint)(3 < *(short *)(iVar4 + 0x6e)) + param_1 * 2) * 2);
  }
  *(short *)(iVar4 + 0x544) = param_1;
  *(short *)(iVar4 + 0x548) = (short)iVar7;
  *(short *)(iVar4 + 0x546) = param_2;
  *(undefined4 *)(iVar4 + 0x54c) = *(undefined4 *)param_3;
  *(undefined4 *)(iVar4 + 0x550) = *(undefined4 *)(param_3 + 2);
  uVar1 = *(undefined4 *)(param_3 + 4);
  *(undefined4 *)(iVar4 + 0x554) = uVar1;
  *(undefined4 *)(iVar4 + 0x558) = *(undefined4 *)(param_3 + 6);
  return CONCAT31((int3)((uint)uVar1 >> 8),1);
}
#endif
