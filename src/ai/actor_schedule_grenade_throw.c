// actor_schedule_grenade_throw  (Ghidra: actor_schedule_grenade_throw, already named)
// address 0x402f80, size 499 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: types/ai.h actor.conversation_index/conversation_participant/awareness_level/
//   mode/vocalization_line/vocalization_variant/vocalization_state/vocalization_unknown_54c/
//   _550/_554/_558 (actor_clear_vocalization zeroes exactly the first three, which this
//   function fills); types/tags.h Actor.event_look_time_modifier (already-named field,
//   confirmed by the 0.5/2.0 clamp and *30.0 ticks-per-second conversion here).
// Despite the name (kept as-is per the naming rule: only FUN_ functions get renamed here),
// what this function actually schedules is a *vocalization* -- it resolves the actor's
// current ai_conversation participant to a speaker handle, then, if the actor is aware
// enough and not already mid-vocalization, arms vocalization_line=8/variant=5 with a
// randomized delay (vocalization_state, in ticks). Presumably line 8 is the "throwing a
// grenade" callout and this is how the callout gets timed relative to the throw, but the
// throw decision itself is not made here.
// register convention: actor index in ECX (in_ECX), the sole real parameter.
//   // blam-cc: ECX -> actor_index
// UNSURE: actor_find_prop_for_object (resolve participant -> speaker), FUN_00568f50 (called with no
// visible arguments on failure) and datum_get (called with no visible arguments) are all
// outside this session's range; declared here to match only what is directly observable at
// the call sites. datum_get's real arguments almost certainly are the just-resolved speaker
// handle, but Ghidra shows none.
//   UNSURE: actor+0x9f falls inside actor.mode_data (a per-mode union); read here as a
//   one-byte "already talking" gate for mode 11 (flee) specifically.
//   UNSURE: local_8/local_4 (written to actor.vocalization_unknown_554/_558) and the upper
//   16 bits of actor.vocalization_unknown_54c are never assigned anywhere in the
//   decompilation before being copied into the actor record -- the original appears to copy
//   uninitialized stack garbage into these three fields. Preserved as uninitialized locals
//   rather than zeroed, per the no-invented-behaviour rule.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

extern real random_real_range(real min, real max); // 0x401050
// 0x43ea80, not yet rewritten: resolves an ai_conversation participant to a speaker handle,
// or -1 if none is currently resolved.
extern datum_index actor_find_prop_for_object(datum_index object_index);
// 0x568f50, not yet rewritten (a different module), called with no visible arguments here.
extern void unit_get_primary_eye_marker_position(void);
// 0x4d0680, a generic engine helper, called with no visible arguments here (see UNSURE).
extern int32_t datum_get(void);

void actor_schedule_grenade_throw(uint32_t actor_index)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (a->conversation_index != (datum_index)k_datum_index_none && a->conversation_participant != (datum_index)k_datum_index_none) {
        int32_t speaker = actor_find_prop_for_object(a->conversation_participant);
        int16_t status;      // low 16 bits of vocalization_unknown_54c
        int16_t status_hi;   // UNSURE: upper 16 bits, never assigned in the original
        int32_t speaker_handle; // vocalization_unknown_550
        uint32_t scratch_a;  // vocalization_unknown_554, UNSURE: uninitialized in the original
        uint32_t scratch_b;  // vocalization_unknown_558, UNSURE: uninitialized in the original

        if (speaker == -1) {
            status = 3;
            unit_get_primary_eye_marker_position();
        } else {
            status = 1;
            speaker_handle = speaker;
        }

        if (a->awareness_level > 1 && a->vocalization_line < 9 &&
            (a->mode != _actor_mode_flee || *(uint8_t *)(a->mode_data + 3) != 0) &&
            (status != 1 || datum_get() != 0)) {
            Actor *actor_def = (Actor *)tag_instances[a->actor_definition_tag & 0xffff].data;
            float delay = 1.2f;
            int32_t ticks;

            if (a->awareness_level < 3 || a->unknown_6e == 0) {
                delay = 2.4f;
            }
            if (actor_def->event_look_time_modifier[0] != 0.0f || actor_def->event_look_time_modifier[1] != 0.0f) {
                float lo = actor_def->event_look_time_modifier[0] <= 0.5f ? 0.5f : actor_def->event_look_time_modifier[0];
                float hi = actor_def->event_look_time_modifier[1] <= 2.0f ? actor_def->event_look_time_modifier[1] : 2.0f;
                delay = random_real_range(lo, hi) * delay;
            }

            ticks = (int32_t)(delay * 30.0f + 0.5f); // ROUND()
            if (ticks > 0x7fff) {
                ticks = 0x7fff;
            }
            a->vocalization_state = (int16_t)ticks;
            a->vocalization_line = 8;
            a->vocalization_variant = 5;
            a->vocalization_unknown_54c = (uint32_t)(uint16_t)status | ((uint32_t)(uint16_t)status_hi << 16);
            a->vocalization_unknown_550 = speaker_handle;
            a->vocalization_unknown_554 = scratch_a;
            a->vocalization_unknown_558 = scratch_b;
        }
    }
}

#if 0
Original Ghidra decompilation (0x402f80):

void actor_schedule_grenade_throw(void)

{
  short sVar1;
  int iVar2;
  int iVar3;
  uint in_ECX;
  int iVar4;
  float fVar5;
  float local_1c;
  float local_18;
  float local_14;
  undefined4 local_10;
  int local_c;
  undefined4 local_8;
  undefined4 local_4;

  iVar4 = (in_ECX & 0xffff) * 0x724;
  if ((*(int *)(*(int *)(DAT_00880360 + 0x34) + 0x1dc + iVar4) != -1) &&
     (iVar2 = *(int *)(*(int *)(DAT_00880360 + 0x34) + iVar4 + 0x1e0), iVar2 != -1)) {
    iVar2 = FUN_0043ea80(iVar2);
    if (iVar2 == -1) {
      local_10 = CONCAT22(local_10._2_2_,3);
      FUN_00568f50();
    }
    else {
      local_10 = CONCAT22(local_10._2_2_,1);
      local_c = iVar2;
    }
    iVar2 = *(int *)(DAT_00880360 + 0x34) + iVar4;
    sVar1 = *(short *)(iVar2 + 0x6a);
    iVar4 = *(int *)((*(uint *)(*(int *)(DAT_00880360 + 0x34) + 0x58 + iVar4) & 0xffff) * 0x20 +
                     0x14 + DAT_0087bc14);
    if ((((1 < sVar1) && (*(short *)(iVar2 + 0x544) < 9)) &&
        ((*(short *)(iVar2 + 0x6c) != 0xb || (*(char *)(iVar2 + 0x9f) != '\0')))) &&
       (((short)local_10 != 1 || (iVar3 = datum_get(), iVar3 != 0)))) {
      local_1c = 1.2;
      if ((sVar1 < 3) || (*(short *)(iVar2 + 0x6e) == 0)) {
        local_1c = 2.4;
      }
      if ((*(float *)(iVar4 + 0xd4) != 0.0) || (*(float *)(iVar4 + 0xd8) != 0.0)) {
        if (*(float *)(iVar4 + 0xd4) <= 0.5) {
          local_14 = 0.5;
        }
        else {
          local_14 = *(float *)(iVar4 + 0xd4);
        }
        if (*(float *)(iVar4 + 0xd8) <= 2.0) {
          local_18 = *(float *)(iVar4 + 0xd8);
        }
        else {
          local_18 = 2.0;
        }
        fVar5 = random_real_range(local_14,local_18);
        local_1c = fVar5 * local_1c;
      }
      iVar4 = (int)ROUND(local_1c * 30.0);
      if (0x7fff < iVar4) {
        iVar4 = 0x7fff;
      }
      *(short *)(iVar2 + 0x548) = (short)iVar4;
      *(undefined2 *)(iVar2 + 0x544) = 8;
      *(undefined2 *)(iVar2 + 0x546) = 5;
      *(undefined4 *)(iVar2 + 0x54c) = local_10;
      *(int *)(iVar2 + 0x550) = local_c;
      *(undefined4 *)(iVar2 + 0x554) = local_8;
      *(undefined4 *)(iVar2 + 0x558) = local_4;
    }
  }
  return;
}
#endif
