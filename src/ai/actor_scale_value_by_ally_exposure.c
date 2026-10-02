// actor_scale_value_by_ally_exposure  (Ghidra: actor_scale_value_by_ally_exposure; named from out/phase2/results/ai_02.json)
// address 0x420c90, size 322 bytes
// name confidence: 0.4   rewrite confidence: 0.9
// evidence: out/phase2/results/ai_02.json -- walks the actor's perceived-prop list counting
//   allies (matching actor.type) that are either exposed or already alert/aiming (via the
//   owning ally actor's unknown_308/mode/mode_data fields), then scales *value down as the
//   exposed-ally count grows and clamps it to [0, 2x], zeroing it out if too many allies are
//   already alert.
// register convention: EAX -> actor_index; param_1 (float*) is Ghidra's recognized stack
//   parameter, the in/out value to scale.
//   // blam-cc: EAX -> actor_index, stack -> value
//
// UNSURE: the original's return value is `(uint)param_1 & 0xffffff00` on every path that takes
// a branch -- the low byte of the *pointer itself*, not a real status code -- or the literal 1
// on the early-bail path. This is decompiler noise from a function whose real return type is
// void; the rewrite returns nothing and only mutates *value, matching the actual behavior.
// UNSURE: ally_actor+0xa8 falls inside types/ai.h's opaque actor.mode_data.raw union (0x9c..0x11f)
// and has no individual field name; accessed as a raw offset from the actor base.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0

// blam-cc: EAX -> actor_index, stack -> value
// FIXED (objdump 0x420d44..0x420dd1): returns AL = 1, leaving the value alone, when 2+ allies are already
//   alerted (0x420d4e jg with EAX = 1); the scaling paths return 0 (xor al,al). The draft returned void.
// Adjusts a caller-supplied probability/weight downward based on how many nearby allies of the
// same type are already exposed or engaged, to avoid redundant reactions.
uint8_t actor_scale_value_by_ally_exposure(datum_index actor_index, float *value)
{
    actor *self;
    datum_index prop_index;
    prop *target;
    actor *ally;
    int16_t exposed_count;
    int16_t alert_count;
    float scale;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    prop_index = self->first_prop;
    exposed_count = 0;
    alert_count = 0;

    while (prop_index != k_datum_index_none) {
        target = (prop *)((uint8_t *)prop_data->data + (prop_index & 0xffff) * sizeof(prop));
        prop_index = target->next_in_actor;

        if (((1 < target->state && target->state < 4) && target->enemy == 0) &&
            (target->actor_type == self->type && target->owner_actor_index != k_datum_index_none)) {
            ally = (actor *)((uint8_t *)actor_data->data + (target->owner_actor_index & 0xffff) * sizeof(actor));

            if (ally->pending_panic_type < 1 &&
                (ally->mode != 4 || *(int16_t *)((uint8_t *)ally + 0xa8) < 1)) {
                if (target->owner_stalled != 0) {
                    exposed_count++;
                }
            } else {
                alert_count++;
            }
        }
    }

    if (alert_count > 1) {
        return 1;
    }
    {
        if (exposed_count < 2) {
            scale = (float)(1 - exposed_count) * 0.5f + 1.0f;
        } else {
            scale = 1.0f - (float)(exposed_count - 1) * 0.25f;
        }
        if (scale < 0.0f) {
            *value = *value * 0.0f;
            return 0;
        }
        if (2.0f < scale) {
            scale = 2.0f;
        }
        *value = scale * *value;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x420c90):

uint FUN_00420c90(float *param_1)

{
  float fVar1;
  short sVar2;
  uint in_EAX;
  int iVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  short sVar7;

  iVar3 = (in_EAX & 0xffff) * 0x724;
  uVar5 = *(uint *)(iVar3 + 0x50 + *(int *)(DAT_00880360 + 0x34));
  sVar7 = 0;
  sVar2 = 0;
  while (uVar5 != 0xffffffff) {
    iVar4 = (uVar5 & 0xffff) * 0x138;
    uVar5 = *(uint *)(iVar4 + 8 + *(int *)(DAT_008802c0 + 0x34));
    iVar4 = iVar4 + *(int *)(DAT_008802c0 + 0x34);
    if ((((1 < *(short *)(iVar4 + 0x24)) && (*(short *)(iVar4 + 0x24) < 4)) &&
        (*(char *)(iVar4 + 0x60) == '\0')) &&
       ((*(short *)(iVar4 + 0x10) == *(short *)(iVar3 + *(int *)(DAT_00880360 + 0x34) + 4) &&
        (*(uint *)(iVar4 + 0x1c) != 0xffffffff)))) {
      iVar6 = (*(uint *)(iVar4 + 0x1c) & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
      if ((*(short *)(iVar6 + 0x308) < 1) &&
         ((*(short *)(iVar6 + 0x6c) != 4 || (*(short *)(iVar6 + 0xa8) < 1)))) {
        if (*(char *)(iVar4 + 300) != '\0') {
          sVar7 = sVar7 + 1;
        }
      }
      else {
        sVar2 = sVar2 + 1;
      }
    }
  }
  uVar5 = 1;
  if (sVar2 < 2) {
    if (sVar7 < 2) {
      fVar1 = (float)(1 - sVar7) * 0.5 + 1.0;
    }
    else {
      fVar1 = 1.0 - (float)(sVar7 + -1) * 0.25;
    }
    if (fVar1 < 0.0) {
      *param_1 = *param_1 * 0.0;
      return (uint)param_1 & 0xffffff00;
    }
    if (2.0 < fVar1) {
      fVar1 = 2.0;
    }
    *param_1 = fVar1 * *param_1;
    uVar5 = (uint)param_1 & 0xffffff00;
  }
  return uVar5;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
