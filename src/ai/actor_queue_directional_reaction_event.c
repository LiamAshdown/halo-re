// actor_queue_directional_reaction_event  (Ghidra: actor_queue_directional_reaction_event, renamed)
// address 0x422270, size 725 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: types/ai.h actor.unknown_2e8[4] (0x2ec), actor.awareness_level (0x6a),
//   actor.vocalization_line/variant/state (0x544/0x546/0x548),
//   actor.vocalization_unknown_54c/550/554/558, actor.mode (0x6c), actor.mode_data.raw[3] (0x9f),
//   actor.alert_level; prop.is_unit (0x60), prop.look_point; types/tags.h
//   Actor.event_look_time_modifier[2] (0xd4/0xd8, matches the 0.5/2.0-clamped random
//   multiplier read here identically to actor_queue_perceived_sighting_dialogue @0x421c20).
//   Calls actor_record_look_at_point (0x421bc0), actor_queue_search_position (0x421af0) and
//   datum_get (0x4d0680, memory module), all already established elsewhere in this module.
//   Ghidra's own pseudocode types the ECX argument as a float compared against "-NAN" and the
//   stack argument as a float; both are decompiler artefacts. objdump against bin/halo.exe
//   (0x422270..0x422544) shows ECX is really an optional prop_index (-1/none sentinel, whose
//   bit pattern happens to equal a quiet NaN) and the stack argument is the actor_index,
//   reused later in the function as an unrelated float scratch local once its value has been
//   consumed -- ordinary decompiler stack-slot reuse, not a real dual-typed parameter.
// register convention: EAX -> direction (nullable real_vector3d*), ECX -> target_prop_index
//   (nullable), stack -> actor_index.
//   // blam-cc: EAX -> direction, ECX -> target_prop_index, stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "fn_ai.h"
#include "fn_math.h"
#include <string.h>

extern double sqrt(double x); // FSQRT
extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0
extern tag_instance *tag_instances; // 0x0087bc14


extern void * datum_get(datum_index handle, data_array *array); // 0x4d0680

// Per-target-status/vitality-grade variant table indexed by (alert_level >= 4), 2 entries.
// Shares the DAT_00655654 table cited generically; no independent name established.
extern int16_t actor_dialogue_variant_table_b[]; // 0x00655654

// blam-cc: EAX -> direction, ECX -> target_prop_index, stack -> actor_index
// Queues category-3 combat dialogue and a matching look-at/search-position update, driven
// either by an explicit unit prop (when it is a unit) or, failing that, a supplied direction
// vector normalized in place. Bails out with no effect if neither source is usable, or if the
// actor is not alert enough / has already queued too many vocalizations / is mid-vocalization
// itself, or (for the explicit-prop path) the prop has since gone away.
void actor_queue_directional_reaction_event(const real_vector3d *direction, datum_index target_prop_index, datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    prop *target = 0;
    real_vector3d normalized;
    int have_direction = 0;
    const void *look_source = 0;
    int16_t kind;
    uint32_t payload;
    Actor *actor_tag;
    float wait_scale;
    float min_scale, max_scale;
    int32_t ticks;

    if (target_prop_index != (datum_index)k_datum_index_none) {
        target = &((prop *)prop_data->data)[target_prop_index & 0xffff];
    } else if (direction != 0) {
        float mag2 = direction->k * direction->k + direction->j * direction->j + direction->i * direction->i;
        if (mag2 > 0.25f) {
            float inv = -1.0f / (float)sqrt((double)mag2);
            normalized.i = inv * direction->i;
            normalized.j = inv * direction->j;
            normalized.k = inv * direction->k;
            have_direction = 1;
            look_source = &normalized;
        }
    }
    if (target != 0) {
        look_source = &target->look_point;
    }

    self->unknown_2e8[4] = 1;

    if ((target == 0 || target->is_unit != 0) && self->awareness_level < 3) {
        actor_record_look_at_point(actor_index, (const uint32_t *)look_source, 5, target_prop_index);
        actor_queue_search_position(actor_index, 0, 5, (real_vector3d *)look_source,
                                    0xffffffff, 0, 90, target_prop_index, 150, 0);
    }

    if (target_prop_index != (datum_index)k_datum_index_none) {
        kind = 1;
        payload = target_prop_index;
    } else {
        if (!have_direction) {
            return;
        }
        kind = 4;
        payload = 0; // UNSURE: three dwords (normalized.i/j/k) are copied individually below;
                     // this slot exists only to mirror the {kind, payload} shape of the other
                     // branch and is never itself read back.
    }

    actor_tag = (Actor *)(tag_instances[self->actor_definition_tag & 0xffff].data);

    if (self->awareness_level > 1 && self->vocalization_line < 12 &&
        (self->mode != 11 || self->mode_data.raw[3] != 0) &&
        (kind != 1 || datum_get(payload, prop_data) != 0)) {
        wait_scale = (self->awareness_level < 3 || self->alert_level == 0) ? 5.0f : 2.5f;

        if (actor_tag->event_look_time_modifier[0] != 0.0f || actor_tag->event_look_time_modifier[1] != 0.0f) {
            min_scale = (actor_tag->event_look_time_modifier[0] <= 0.5f) ? 0.5f : actor_tag->event_look_time_modifier[0];
            max_scale = (actor_tag->event_look_time_modifier[1] <= 2.0f) ? actor_tag->event_look_time_modifier[1] : 2.0f;
            wait_scale = random_real_range(min_scale, max_scale) * wait_scale;
        }

        ticks = (int32_t)(wait_scale * 30.0f + 0.5f); // ROUND
        if (ticks > 0x7fff) {
            ticks = 0x7fff;
        }

        self->vocalization_state = (int16_t)ticks;
        self->vocalization_variant = actor_dialogue_variant_table_b[self->alert_level >= 4];
        self->vocalization_line = 11;
        if (target_prop_index != (datum_index)k_datum_index_none) {
            self->vocalization_unknown_54c = target_prop_index;
            self->vocalization_unknown_550 = 0;
            self->vocalization_unknown_554 = 0;
        } else {
            memcpy(&self->vocalization_unknown_54c, &normalized, sizeof(real_vector3d));
        }
    }
}

#if 0
Original Ghidra decompilation (0x422270):

void FUN_00422270(float param_1)

{
  short sVar1;
  undefined2 uVar2;
  bool bVar3;
  short sVar4;
  float *in_EAX;
  float in_ECX;
  int iVar5;
  int iVar6;
  int iVar7;
  float fVar8;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  undefined4 local_10;
  float local_c;
  float local_8;
  float local_4;

  iVar7 = ((uint)param_1 & 0xffff) * 0x724;
  iVar6 = *(int *)(DAT_00880360 + 0x34) + iVar7;
  iVar5 = 0;
  bVar3 = false;
  if ((((in_ECX == -NAN) ||
       (iVar5 = ((uint)in_ECX & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34), iVar5 == 0)) &&
      (in_EAX != (float *)0x0)) &&
     (fVar8 = in_EAX[2] * in_EAX[2] + in_EAX[1] * in_EAX[1] + *in_EAX * *in_EAX, 0.25 < fVar8)) {
    bVar3 = true;
    fVar8 = -1.0 / SQRT(fVar8);
    local_1c = fVar8 * *in_EAX;
    local_18 = fVar8 * in_EAX[1];
    local_14 = fVar8 * in_EAX[2];
  }
  *(undefined1 *)(iVar6 + 0x2ec) = 1;
  if (((iVar5 == 0) || (*(char *)(iVar5 + 0x60) != '\0')) && (*(short *)(iVar6 + 0x6a) < 3)) {
    FUN_00421bc0();
    FUN_00421af0(0xffffffff,0,0x5a);
  }
  if (in_ECX == -NAN) {
    if (!bVar3) {
      return;
    }
    local_c = local_1c;
    sVar4 = 4;
    local_8 = local_18;
    local_4 = local_14;
  }
  else {
    sVar4 = 1;
    local_c = in_ECX;
  }
  iVar6 = *(int *)(DAT_00880360 + 0x34) + iVar7;
  sVar1 = *(short *)(iVar6 + 0x6a);
  iVar5 = *(int *)((*(uint *)(*(int *)(DAT_00880360 + 0x34) + 0x58 + iVar7) & 0xffff) * 0x20 + 0x14
                  + DAT_0087bc14);
  local_10 = CONCAT22(local_10._2_2_,sVar4);
  if (((1 < sVar1) && (*(short *)(iVar6 + 0x544) < 0xc)) &&
     (((*(short *)(iVar6 + 0x6c) != 0xb || (*(char *)(iVar6 + 0x9f) != '\0')) &&
      ((sVar4 != 1 || (iVar7 = datum_get(), iVar7 != 0)))))) {
    param_1 = 2.5;
    if ((sVar1 < 3) || (*(short *)(iVar6 + 0x6e) == 0)) {
      param_1 = 5.0;
    }
    if ((*(float *)(iVar5 + 0xd4) != 0.0) || (*(float *)(iVar5 + 0xd8) != 0.0)) {
      if (*(float *)(iVar5 + 0xd4) <= 0.5) {
        local_20 = 0.5;
      }
      else {
        local_20 = *(float *)(iVar5 + 0xd4);
      }
      if (*(float *)(iVar5 + 0xd8) <= 2.0) {
        local_24 = *(float *)(iVar5 + 0xd8);
      }
      else {
        local_24 = 2.0;
      }
      fVar8 = random_real_range(local_20,local_24);
      param_1 = fVar8 * param_1;
    }
    iVar5 = (int)ROUND(param_1 * 30.0);
    if (0x7fff < iVar5) {
      iVar5 = 0x7fff;
    }
    uVar2 = *(undefined2 *)(&DAT_00655654 + (uint)(3 < *(short *)(iVar6 + 0x6e)) * 2);
    *(short *)(iVar6 + 0x548) = (short)iVar5;
    *(undefined2 *)(iVar6 + 0x546) = uVar2;
    *(undefined2 *)(iVar6 + 0x544) = 0xb;
    *(undefined4 *)(iVar6 + 0x54c) = local_10;
    *(float *)(iVar6 + 0x550) = local_c;
    *(float *)(iVar6 + 0x554) = local_8;
    *(float *)(iVar6 + 0x558) = local_4;
  }
  return;
}

Ground truth from objdump (bin/halo.exe @ 0x422270..0x422544) confirming the ECX/stack roles
and the exact register setup for the two truncated calls:

  422275: mov ebp,[esp+0x34]         ; ebp = actor_index (the real stack argument)
  422286: mov ebx,eax                ; ebx = direction pointer (EAX on entry)
  422291: mov edi,ecx                ; edi = target_prop_index (ECX on entry)
  ... (prop lookup / direction normalize as in the C above) ...
  422338: mov ebx,[esp+0x3c]         ; reload actor_index
  42233c: push edi                   ; -> data (actor_record_look_at_point)
  42233d: mov ecx,esi                ; point = esi (prop.look_point or normalized vector or NULL)
  42233f: mov edx,5                  ; priority = 5
  422344: mov eax,ebx                ; actor_index
  422346: call actor_record_look_at_point
  42234b: push 0x0 / push 0x96 / push edi / push 0x5a / push 0x0 / push 0xffffffff
  422359: xor ecx,ecx                ; position = NULL
  42235b: mov edx,5                  ; priority = 5
  422360: mov eax,ebx
  422362: call actor_queue_search_position
  422367: add esp,0x1c               ; cleans up both calls' stack args (1 + 6 dwords) at once
  ...
  422402: mov edx,[esp+0x2c]         ; prop_index (kind==1 path)
  422406: mov esi,ds:0x8802c0        ; prop_data (the data_array*, not ->data)
  42240c: call datum_get             ; EDX = handle, ESI = array
#endif
