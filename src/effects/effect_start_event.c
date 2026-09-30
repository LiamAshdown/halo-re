// effect_start_event  (Ghidra: FUN_00451660; named per out/phase4/effects_types_notes.md, which
// refers to this address directly: "effect_start_event 0x451660 and effect_update; event_duration
// is random_real_range_seeded(EffectEvent.duration_bounds)")
// address 0x451660, size 172 bytes
// name confidence: 0.5   rewrite confidence: 0.9 (VERIFIED against objdump 0x451660..0x45170b; seed choice FIXED)
// evidence: types/effects.h effect (flags, event_index 0x4e, event_time 0x50, event_duration
// 0x54); types/tags.h Effect.events, EffectEvent.delay_bounds (0x08); src/memory/datum_get.c is
// byte-for-byte the same validate-index-and-salt check this function opens with.
// register convention: effect handle in EAX (in_EAX), event index in DI (unaff_DI).
//   // blam-cc: EAX -> effect_handle, EDI -> event_index
// UNSURE: the random range this rolls into event_duration is read from EffectEvent.delay_bounds
// (tag offset 0x08), not duration_bounds (0x10) as out/phase4/effects_types_notes.md's prose
// claims -- the raw offsets (+8/+0xc relative to the event record) are used here since
// types/tags.h's EffectEvent layout is the one with verified offsetof assertions.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#include "fn_math.h"
#include "fn_effects.h"

extern data_array *effect_data;     // 0x0087abdc
extern tag_instance *tag_instances; // 0x0087bc14
extern random_seed random_seed_global; // 0x00719cd0
extern random_seed effect_random_seed; // 0x00719cd4

extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680,
    // blam-cc: EDX -> handle, ESI -> array


// Begins event `event_index` on an effect: resets its elapsed time and started flag, and rolls
// its duration from the tag's delay bounds using the deterministic global seed.
void effect_start_event(datum_index effect_handle, int16_t event_index)
{
    effect *self = (effect *)datum_get(effect_handle, effect_data);

    if (self != 0) {
        Effect *tag = (Effect *)tag_instances[(uint16_t)self->definition_index].data;

        if (event_index >= 0 && (uint32_t)event_index < tag->events.count) {
            EffectEvent *event = &((EffectEvent *)tag->events.pointer)[event_index];

            self->flags = self->flags & ~_effect_event_started_bit;
            self->event_index = event_index;
            self->event_time = 0.0f;
            // FIXED (0x4516e7..0x4516f1): tag flag 4 selects the global seed, otherwise the effect seed
            self->event_duration = random_real_range_seeded((tag->flags & 4) != 0 ? &random_seed_global
                : &effect_random_seed, event->delay_bounds[0], event->delay_bounds[1]);
        }
    }
}

#if 0
Original Ghidra decompilation (0x451660):

void FUN_00451660(void)

{
  short sVar1;
  int in_EAX;
  int iVar2;
  short *psVar3;
  int extraout_EDX;
  short unaff_DI;
  float10 fVar4;

  if (((in_EAX != -1) && (sVar1 = (short)in_EAX, -1 < sVar1)) &&
     (sVar1 < *(short *)(DAT_0087abdc + 0x20))) {
    psVar3 = (short *)((int)*(short *)(DAT_0087abdc + 0x22) * (int)sVar1 +
                      *(int *)(DAT_0087abdc + 0x34));
    if ((((*psVar3 != 0) &&
         ((sVar1 = (short)((uint)in_EAX >> 0x10), sVar1 == 0 || (*psVar3 == sVar1)))) &&
        (iVar2 = *(int *)((*(uint *)(psVar3 + 2) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
        -1 < unaff_DI)) && ((int)unaff_DI < *(int *)(iVar2 + 0x34))) {
      iVar2 = *(int *)(iVar2 + 0x38);
      *(byte *)(psVar3 + 1) = *(byte *)(psVar3 + 1) & 0xfe;
      psVar3[0x27] = unaff_DI;
      psVar3[0x28] = 0;
      psVar3[0x29] = 0;
      iVar2 = unaff_DI * 0x44 + iVar2;
      fVar4 = (float10)random_real_range_seeded
                                 (*(undefined4 *)(iVar2 + 8),*(undefined4 *)(iVar2 + 0xc));
      *(float *)(extraout_EDX + 0x54) = (float)fVar4;
    }
  }
  return;
}
#endif
