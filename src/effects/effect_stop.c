// effect_stop  (Ghidra: FUN_00450b20; named per out/phase4/effects_types_notes.md, which refers
// to this address directly: "effect_stop 0x450b20 (bits 2, 3, 5)")
// address 0x450b20, size 189 bytes
// name confidence: 0.35   rewrite confidence: 0.9 (VERIFIED against objdump 0x450b20..0x450bd8; stop event + 1 FIXED)
// evidence: types/effects.h effect_flags (_effect_looping_bit, _effect_stopping_bit,
// _effect_finished_bit, _effect_stop_immediately_bit) and effect (0x02 flags); types/tags.h
// Effect (loop_stop_event 0x06, events TagReflexive 0x34); src/memory/datum_get.c is the same
// validate-index-and-salt check this function opens with.
// register convention: effect handle in EAX (in_EAX); the "stop immediately, without playing the
// stop event" flag is Ghidra's own recognised stack parameter (param_1).
//   // blam-cc: EAX -> effect_handle, stack -> stop_immediately

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern data_array *effect_data;     // 0x0087abdc
extern tag_instance *tag_instances; // 0x0087bc14

extern void *datum_get(datum_index handle, data_array *array); // 0x4d0680,
    // blam-cc: EDX -> handle, ESI -> array
extern void effect_delete(datum_index effect_handle); // 0x450be0, this module
extern void effect_start_event(datum_index effect_handle, int16_t event_index); // 0x451660,
    // this module; blam-cc: EAX -> effect_handle, EDI -> event_index

// Stops a looping effect: deletes it outright if it was never looping, otherwise plays its
// loop_stop_event (or, if there is none, marks it finished immediately). `stop_immediately`
// records whether the stop event itself should skip straight to its own end.
void effect_stop(datum_index effect_handle, uint8_t stop_immediately)
{
    effect *self = (effect *)datum_get(effect_handle, effect_data);

    if (self != 0) {
        Effect *tag = (Effect *)tag_instances[(uint16_t)self->definition_index].data;

        if ((self->flags & _effect_looping_bit) == 0) {
            effect_delete(effect_handle);
            return;
        }

        if (stop_immediately == 0) {
            self->flags = self->flags & ~_effect_stop_immediately_bit;
        } else {
            self->flags = self->flags | _effect_stop_immediately_bit;
        }

        if (tag->loop_stop_event < 0 || (int32_t)tag->events.count <= tag->loop_stop_event + 1) {
            self->flags = self->flags | _effect_finished_bit;
            return;
        }

        effect_start_event(effect_handle, (int16_t)(tag->loop_stop_event + 1)); // FIXED (0x450bba): EDI = loop_stop_event + 1
        self->flags = self->flags | _effect_stopping_bit;
    }
}

#if 0
Original Ghidra decompilation (0x450b20):

void FUN_00450b20(char param_1)

{
  int iVar1;
  short sVar2;
  int in_EAX;
  ushort uVar3;
  short sVar4;
  int iVar5;

  if (((in_EAX != -1) && (sVar2 = (short)in_EAX, -1 < sVar2)) &&
     (sVar2 < *(short *)(DAT_0087abdc + 0x20))) {
    iVar5 = (int)*(short *)(DAT_0087abdc + 0x22) * (int)sVar2;
    sVar2 = *(short *)(iVar5 + *(int *)(DAT_0087abdc + 0x34));
    iVar5 = iVar5 + *(int *)(DAT_0087abdc + 0x34);
    if ((sVar2 != 0) && ((sVar4 = (short)((uint)in_EAX >> 0x10), sVar4 == 0 || (sVar2 == sVar4)))) {
      iVar1 = *(int *)((*(uint *)(iVar5 + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      uVar3 = *(ushort *)(iVar5 + 2);
      if ((uVar3 & 2) == 0) {
        particle_system_delete_450be0(in_EAX);
        return;
      }
      if (param_1 == '\0') {
        uVar3 = uVar3 & 0xffdf;
      }
      else {
        uVar3 = uVar3 | 0x20;
      }
      *(ushort *)(iVar5 + 2) = uVar3;
      sVar2 = *(short *)(iVar1 + 6);
      if ((sVar2 < 0) || (*(int *)(iVar1 + 0x34) <= sVar2 + 1)) {
        *(byte *)(iVar5 + 2) = *(byte *)(iVar5 + 2) | 8;
        return;
      }
      FUN_00451660();
      *(byte *)(iVar5 + 2) = *(byte *)(iVar5 + 2) | 4;
    }
  }
  return;
}
#endif
