// actor_recompute_grenade_eligibility  (Ghidra: actor_recompute_grenade_eligibility; named for this rewrite)
// address 0x42f260, size 268 bytes
// name confidence: 0.35   rewrite confidence: 0.85
// evidence: phase-4 summary ("recomputes and caches whether an actor currently qualifies to
// throw/react with a grenade, along with a recheck timer"); writes actor.grenade_eligible
// (0x6cc) and actor.grenade_recheck_ticks (0x6ce), both already named in types/ai.h.
// register convention: EAX -> actor_index (in_EAX, the only register Ghidra's own decompile
// shows).
// blam-cc: EAX -> actor_index
//
// UNSURE: the float feeding __ftol (Ghidra shows no visible FPU setup between the LCG step
// and the truncation) is not reconstructed; k_random_scale_65536/ticks_per_second are presumably its
// scale and offset by analogy with this module's other random-range reseeds (see
// actor_reseed_movement_pause_timer.c for the same "invisible dataflow" situation), but the
// exact formula is not confirmed. Only the two visible writes and the LCG step are faithful.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "objects.h"
#include "cache.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint32_t random_seed_global; // 0x00719cd0
extern float k_random_scale_65536; // 0x00672b84, 1.5259022e-05 = 1/65536
extern float ticks_per_second; // 0x00672ac8, 30.0

// blam-cc: EAX -> actor_index
// REWRITTEN 2026-09-28 from objdump 0x42f260..0x42f36b (the draft's formula was a placeholder). The recheck time is
// a random lerp over one of two ranges in the actor definition (actor +0x58): +0x400..+0x404 when eligible, else
// +0x3f8..+0x3fc, in seconds, times 30, plus the unit's +0x3fa ticks when its +0x388 word is positive.
void actor_recompute_grenade_eligibility(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint8_t *definition = (uint8_t *)tag_instances[*(uint32_t *)&self->actor_definition_tag & 0xffff].data;
    uint8_t eligible = (uint8_t)(self->awareness_level == 3 && self->unknown_6e > self->unknown_72);
    int16_t base_ticks = 0;
    float minimum;
    float maximum;
    float fraction;

    if (self->unit_index != (datum_index)k_datum_index_none) {
        uint8_t *unit_obj = (uint8_t *)((object_header *)object_data->data)[self->unit_index & 0xffff].data;

        if (*(int16_t *)(unit_obj + 0x388) > 0) {
            base_ticks = *(int16_t *)(unit_obj + 0x3fa);
        }
    }

    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    fraction = (float)(int32_t)(random_seed_global >> 16) * k_random_scale_65536;
    if (eligible) {
        minimum = *(float *)(definition + 0x400);
        maximum = *(float *)(definition + 0x404);
    } else {
        minimum = *(float *)(definition + 0x3f8);
        maximum = *(float *)(definition + 0x3fc);
    }

    self->grenade_eligible = eligible;
    self->grenade_recheck_ticks =
        (int16_t)(long long)((fraction * (maximum - minimum) + minimum) * ticks_per_second + (float)base_ticks);
}

#if 0
Original Ghidra decompilation (0x42f260):

void FUN_0042f260(void)

{
  int iVar1;
  undefined2 uVar2;
  uint in_EAX;
  undefined1 uVar3;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if ((*(short *)(iVar1 + 0x6a) == 3) && (*(short *)(iVar1 + 0x72) < *(short *)(iVar1 + 0x6e))) {
    uVar3 = 1;
  }
  else {
    uVar3 = 0;
  }
  random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
  *(undefined1 *)(iVar1 + 0x6cc) = uVar3;
  uVar2 = __ftol();
  *(undefined2 *)(iVar1 + 0x6ce) = uVar2;
  return;
}
#endif
