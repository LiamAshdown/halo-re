// actor_schedule_grenade_throw  (Ghidra: actor_schedule_grenade_throw, already named)
// address 0x402f80, size 499 bytes
// name confidence: 0.5   rewrite confidence: 0.9
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
//   UNSURE: actor+0x9f falls inside actor.mode_data.raw (a per-mode union); read here as a
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
#include <string.h>
#include "cache.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;      // 0x00880360
extern data_array *prop_data;       // 0x008802c0
extern tag_instance *tag_instances; // 0x0087bc14

extern real random_real_range(real min, real max); // 0x401050
extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index); // 0x43ea80, stack, ECX
extern void unit_get_primary_eye_marker_position(uint32_t object_index, real_point3d *out); // 0x568f50, ECX, ESI
extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680, EDX, ESI
extern int32_t fistp_round(float x); // harness/x87_shims.c

// REWRITTEN from objdump 0x402f80..0x403172 (misnamed: it schedules a look at whatever last hurt the actor). ECX:
//   actor. With a damage source (+0x1dc, object +0x1e0) the target is the actor's prop for it (kind 1) or the
//   object's eye (kind 3). An awake actor (+0x6a > 1) without a higher pending look (+0x544 <= 8), not
//   hiding in cover mode 11 without +0x9f, queues look 8 at priority 5 (+0x544 / +0x546 / +0x54c..) after
//   1.2 s (2.4 s when unalerted), scaled by a random factor in the Actor tag's [+0xd4 (>= 0.5), +0xd8 (<= 2)].
// blam-cc: ECX -> actor_index
void actor_schedule_grenade_throw(uint32_t actor_index)
{
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *actor_tag;
    datum_index source;
    uint8_t request[0x10];      // [esp+0x18]: kind word, then a prop or a point
    datum_index prop;
    float delay;
    int32_t ticks;

    if (((actor *)a)->conversation_index == k_datum_index_none) {
        return;
    }
    source = ((actor *)a)->conversation_participant;
    if (source == k_datum_index_none) {
        return;
    }
    memset(request, 0, sizeof(request));
    prop = actor_find_prop_for_object(source, actor_index);
    if (prop != k_datum_index_none) {
        *(int16_t *)request = 1;
        *(datum_index *)(request + 0x4) = prop;
    } else {
        *(int16_t *)request = 3;
        unit_get_primary_eye_marker_position(source, (real_point3d *)(request + 0x4));
    }
    actor_tag = (uint8_t *)tag_instances[((actor *)a)->actor_definition_tag & 0xffff].data;
    if (!(((actor *)a)->awareness_level > 1) || ((actor *)a)->vocalization_line > 8) {
        return;
    }
    if (((actor *)a)->mode == 0xb && !a[0x9f]) {
        return;
    }
    if (*(int16_t *)request == 1 && datum_get(*(datum_index *)(request + 0x4), prop_data) == 0) {
        return;
    }
    delay = (((actor *)a)->awareness_level < 3 || ((struct actor *)a)->combat_status == 0) ? 2.4f : 1.2f;
    if (*(float *)(actor_tag + 0xd4) != 0.0f || *(float *)(actor_tag + 0xd8) != 0.0f) {
        float lo = *(float *)(actor_tag + 0xd4) > 0.5f ? *(float *)(actor_tag + 0xd4) : 0.5f;
        float hi = *(float *)(actor_tag + 0xd8) > 2.0f ? 2.0f : *(float *)(actor_tag + 0xd8);

        delay = random_real_range(lo, hi) * delay;
    }
    ticks = fistp_round(delay * 30.0f);
    if (ticks > 0x7fff) {
        ticks = 0x7fff;
    }
    ((actor *)a)->vocalization_state = (int16_t)ticks;
    ((actor *)a)->vocalization_line = 8;
    ((actor *)a)->vocalization_variant = 5;
    memcpy(a + 0x54c, request, 0x10);
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
