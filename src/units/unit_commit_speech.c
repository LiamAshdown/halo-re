// unit_commit_speech  (Ghidra: unit_commit_speech)
// address 0x560f20, size 264 bytes
// name confidence: 0.4 (renamed from phase2's "unit_set_animation_state"; the body is entirely
//   about the dialogue/speech queue, not animation state)   rewrite confidence: 0.35
// evidence: types/units.h unit_data.current_speech/.pending_speech (unit_speech, 0x30 bytes,
//   at 0x388/0x3b8), .speech_started/.speech_lipsync_stopped/.speech_finished (0x3f4/0x3f5/
//   0x3f6), .speech_delay_ticks/.speech_duration_ticks/.speech_lipsync_ticks/.speech_tail_ticks
//   (0x3f8/0x3fa/0x3fc/0x3fe), .speech_sound_handle (0x400).
// register convention: unit index in EAX, a source unit_speech record in ECX, a mode selector
//   in DX (1 = queue into pending_speech, >1 = commit into current_speech, otherwise no-op).
//   // blam-cc: in_EAX -> unit_index, in_ECX -> source, in_DX -> mode
// UNSURE: the return value for the "commit" path is a signed-division-by-1000 idiom Ghidra only
//   partially folded (`(longlong)iVar2 * 0x10624dd3`); no caller in this batch captures this
//   function's return value, so it is reproduced bit-for-bit rather than simplified to the
//   division it most likely represents.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

int32_t unit_commit_speech(uint32_t unit_index, const unit_speech *source, int16_t mode) // blam-cc: see file header
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    if ((obj->vitality_flags & _object_health_frozen_bit) == 0 || mode == 10) {
        if (mode > 1) {
            unit->current_speech = *source;

            if (mode == 3 && unit->pending_speech.priority > 0) {
                unit->pending_speech.priority = 0;
            }
            unit->speech_started = 0;
            unit->speech_lipsync_stopped = 0;
            unit->speech_finished = 0;
            unit->speech_tail_ticks = unit->current_speech.tail_ticks;
            unit->speech_sound_handle = (datum_index)-1;
            unit->speech_delay_ticks = unit->current_speech.delay_ticks;
            unit->speech_lipsync_ticks = unit->current_speech.lipsync_ticks;

            if (unit->current_speech.sound_tag == (datum_index)-1) {
                unit->speech_duration_ticks = 0x2d;
                return -1;
            }

            Sound *sound_tag = (Sound *)tag_instances[unit->current_speech.sound_tag & 0xffff].data;
            int32_t length = *(int32_t *)((uint8_t *)sound_tag + 0x84) * 0x1e; // UNSURE: raw Sound field
            unit->speech_duration_ticks = (int16_t)(length / 1000);
            return (int32_t)((int64_t)length * 0x10624dd3); // see file header UNSURE note
        } else if (mode == 1) {
            unit->pending_speech = *source;
        }
    }
    return (int32_t)(((unit_index & 0xffff) * 3) & 0xffff0000); // always 0; preserved literally
}

#if 0
Original Ghidra decompilation (0x560f20):

int FUN_00560f20(void)

{
  int iVar1;
  uint in_EAX;
  short *in_ECX;
  int iVar2;
  short in_DX;
  undefined4 *puVar3;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if (((*(byte *)(iVar1 + 0x106) & 4) == 0) || (*in_ECX == 10)) {
    if (1 < in_DX) {
      puVar3 = (undefined4 *)(iVar1 + 0x388);
      for (iVar2 = 0xc; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar3 = *(undefined4 *)in_ECX;
        in_ECX = in_ECX + 2;
        puVar3 = puVar3 + 1;
      }
      if ((in_DX == 3) && (0 < *(short *)(iVar1 + 0x3b8))) {
        *(undefined2 *)(iVar1 + 0x3b8) = 0;
      }
      *(undefined1 *)(iVar1 + 0x3f4) = 0;
      *(undefined1 *)(iVar1 + 0x3f5) = 0;
      *(undefined1 *)(iVar1 + 0x3f6) = 0;
      *(undefined2 *)(iVar1 + 0x3fe) = *(undefined2 *)(iVar1 + 0x394);
      *(undefined4 *)(iVar1 + 0x400) = 0xffffffff;
      *(undefined2 *)(iVar1 + 0x3f8) = *(undefined2 *)(iVar1 + 0x390);
      *(undefined2 *)(iVar1 + 0x3fc) = *(undefined2 *)(iVar1 + 0x392);
      if (*(uint *)(iVar1 + 0x38c) == 0xffffffff) {
        *(undefined2 *)(iVar1 + 0x3fa) = 0x2d;
        return -1;
      }
      iVar2 = *(int *)(*(int *)((*(uint *)(iVar1 + 0x38c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                      0x84) * 0x1e;
      *(short *)(iVar1 + 0x3fa) =
           ((short)(iVar2 / 1000) + (short)(iVar2 >> 0x1f)) -
           (short)((longlong)iVar2 * 0x10624dd3 >> 0x3f);
      return (int)((longlong)iVar2 * 0x10624dd3);
    }
    if (in_DX == 1) {
      puVar3 = (undefined4 *)(iVar1 + 0x3b8);
      for (iVar2 = 0xc; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar3 = *(undefined4 *)in_ECX;
        in_ECX = in_ECX + 2;
        puVar3 = puVar3 + 1;
      }
    }
  }
  return (in_EAX & 0xffff) * 3;
}
#endif
