// unit_update_animation_timers  (Ghidra: unit_update_animation_timers)
// address 0x561620, size 700 bytes
// name confidence: 0.35 (phase2 candidate)   rewrite confidence: 0.25
// evidence: types/units.h unit_data.flags (0x204, _unit_flag_permutation_dirty),
//   .unknown_3e8/.unknown_3ea/.unknown_3ec (0x3e8/0x3ea/0x3ec), .current_speech/.pending_speech
//   (.priority), .speech_delay_ticks/.speech_started/.speech_lipsync_ticks/
//   .speech_duration_ticks/.speech_finished/.speech_tail_ticks/.speech_lipsync_stopped/
//   .speech_sound_handle (0x3f8/0x3f4/0x3fc/0x3fa/0x3f6/0x3fe/0x3f5/0x400).
//   unit_choose_dialogue_variant (0x561990), unit_commit_speech (0x560f20).
// register convention: unit index in EAX.
//   // blam-cc: in_EAX -> unit_index
// UNSURE: the double decrement of unknown_3ec (two identical `if (0 < ...) --` blocks back to
//   back) is reproduced literally -- it looks like a genuine quirk of the original rather than
//   decompiler noise, since both instances have their own distinct address range in the pack.
//   object_get_node_local_transform and sound_start_at_object_marker are called with argument counts Ghidra
//   could not fully recover; the local that carries a position between them is modelled as a
//   scratch object_marker, matching the convention used in unit_fire_animation_sound_trigger.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

extern void ai_communication_gate_line_played(void);  // 0x42e970, UNSURE: no traced args
extern void ai_propagate_communication_reaction(void);  // 0x42e9c0, UNSURE: no traced args
extern void ai_communication_play_event_line(void);  // 0x42eee0, UNSURE: no traced args
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
                                                object_marker *marker, uint32_t flags); // 0x4f6080, UNSURE args
extern datum_index sound_start_at_object_marker(datum_index sound_tag, void *position, float volume, uint32_t flag); // 0x543ce0, UNSURE signature
extern void unit_choose_dialogue_variant(uint32_t unit_index); // 0x561990, UNSURE: implicit unit_index
extern int32_t unit_commit_speech(uint32_t unit_index, const unit_speech *source, int16_t mode); // 0x560f20

void unit_update_animation_timers(uint32_t unit_index) // blam-cc: in_EAX -> unit_index
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    if ((unit->flags & _unit_flag_permutation_dirty) != 0) {
        unit_choose_dialogue_variant(unit_index);
        unit->flags = unit->flags & ~(uint32_t)_unit_flag_permutation_dirty;
    }

    if (unit->unknown_3e8 > 0) {
        unit->unknown_3e8 = unit->unknown_3e8 - 1;
        if (unit->unknown_3e8 == 0 && unit->unknown_3ea > 0) {
            unit->unknown_3ea = unit->unknown_3ea - 1;
            unit->unknown_3e8 = 0x16;
        }
    }
    if (unit->unknown_3ec > 0) {
        unit->unknown_3ec = unit->unknown_3ec - 1;
    }
    if (unit->unknown_3ec > 0) { // see file header: reproduced literally, twice in the original
        unit->unknown_3ec = unit->unknown_3ec - 1;
    }

    if (unit->current_speech.priority > 0) {
        if (unit->speech_delay_ticks < 1) {
            if (unit->speech_started == 0) {
                object_marker marker = {0}; // UNSURE: scratch, see file header
                int16_t ok = (int16_t)object_get_node_local_transform(unit_index, 0, &marker, 1);
                if (unit->current_speech.sound_tag != (datum_index)-1) {
                    unit->speech_sound_handle =
                        sound_start_at_object_marker(unit->current_speech.sound_tag, ok != 0 ? (void *)&marker : 0, 1.0f, 0);
                }
                ai_communication_gate_line_played();
                unit->speech_started = 1;
            }
            if (unit->speech_lipsync_ticks > 0) {
                unit->speech_lipsync_ticks = unit->speech_lipsync_ticks - 1;
            }
            if (unit->speech_duration_ticks < 1) {
                if (unit->speech_finished == 0) {
                    ai_communication_play_event_line();
                    unit->speech_finished = 1;
                }
                if (unit->speech_tail_ticks > 0) {
                    unit->speech_tail_ticks = unit->speech_tail_ticks - 1;
                }
                if (unit->speech_tail_ticks == 0) {
                    unit->speech_lipsync_ticks = 0;
                }
            } else {
                unit->speech_duration_ticks = unit->speech_duration_ticks - 1;
                if (unit->speech_duration_ticks == 0) {
                    unit->speech_sound_handle = (datum_index)-1;
                }
            }
        } else {
            unit->speech_delay_ticks = unit->speech_delay_ticks - 1;
        }
    }

    if (unit->speech_lipsync_ticks == 0 && unit->speech_lipsync_stopped == 0) {
        ai_propagate_communication_reaction();
        unit->speech_lipsync_stopped = 1;
    }

    int16_t current_priority = unit->current_speech.priority;
    if (current_priority > 0) {
        if (unit->speech_duration_ticks == 0 && unit->speech_tail_ticks == 0) {
            unit->current_speech.priority = 0;
        }
        current_priority = unit->current_speech.priority;
    }
    if (current_priority == 0 && unit->pending_speech.priority > 0) {
        unit_commit_speech(unit_index, 0, 1); // UNSURE: promotes pending_speech; see unit_commit_speech mode 1
    }
}

#if 0
Original Ghidra decompilation (0x561620):

void FUN_00561620(void)

{
  int iVar1;
  short sVar2;
  uint in_EAX;
  undefined4 uVar3;
  undefined4 local_6c;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if ((*(uint *)(iVar1 + 0x204) & 0x100) != 0) {
    FUN_00561990();
    *(uint *)(iVar1 + 0x204) = *(uint *)(iVar1 + 0x204) & 0xfffffeff;
  }
  if (((0 < *(short *)(iVar1 + 1000)) &&
      (sVar2 = *(short *)(iVar1 + 1000) + -1, *(short *)(iVar1 + 1000) = sVar2, sVar2 == 0)) &&
     (0 < *(short *)(iVar1 + 0x3ea))) {
    *(short *)(iVar1 + 0x3ea) = *(short *)(iVar1 + 0x3ea) + -1;
    *(undefined2 *)(iVar1 + 1000) = 0x16;
  }
  if (0 < *(short *)(iVar1 + 0x3ec)) {
    *(short *)(iVar1 + 0x3ec) = *(short *)(iVar1 + 0x3ec) + -1;
  }
  if (0 < *(short *)(iVar1 + 0x3ec)) {
    *(short *)(iVar1 + 0x3ec) = *(short *)(iVar1 + 0x3ec) + -1;
  }
  if (0 < *(short *)(iVar1 + 0x388)) {
    if (*(short *)(iVar1 + 0x3f8) < 1) {
      if (*(char *)(iVar1 + 0x3f4) == '\0') {
        sVar2 = object_get_node_local_transform();
        if (sVar2 == 0) {
          local_6c = 0;
        }
        if (*(int *)(iVar1 + 0x38c) != -1) {
          uVar3 = FUN_00543ce0(*(int *)(iVar1 + 0x38c),local_6c,0x3f800000,0);
          *(undefined4 *)(iVar1 + 0x400) = uVar3;
        }
        FUN_0042e970();
        *(undefined1 *)(iVar1 + 0x3f4) = 1;
      }
      if (0 < *(short *)(iVar1 + 0x3fc)) {
        *(short *)(iVar1 + 0x3fc) = *(short *)(iVar1 + 0x3fc) + -1;
      }
      if (*(short *)(iVar1 + 0x3fa) < 1) {
        if (*(char *)(iVar1 + 0x3f6) == '\0') {
          FUN_0042eee0();
          *(undefined1 *)(iVar1 + 0x3f6) = 1;
        }
        if (0 < *(short *)(iVar1 + 0x3fe)) {
          *(short *)(iVar1 + 0x3fe) = *(short *)(iVar1 + 0x3fe) + -1;
        }
        if (*(short *)(iVar1 + 0x3fe) == 0) {
          *(undefined2 *)(iVar1 + 0x3fc) = 0;
        }
      }
      else {
        sVar2 = *(short *)(iVar1 + 0x3fa) + -1;
        *(short *)(iVar1 + 0x3fa) = sVar2;
        if (sVar2 == 0) {
          *(undefined4 *)(iVar1 + 0x400) = 0xffffffff;
        }
      }
    }
    else {
      *(short *)(iVar1 + 0x3f8) = *(short *)(iVar1 + 0x3f8) + -1;
    }
  }
  if ((*(short *)(iVar1 + 0x3fc) == 0) && (*(char *)(iVar1 + 0x3f5) == '\0')) {
    FUN_0042e9c0();
    *(undefined1 *)(iVar1 + 0x3f5) = 1;
  }
  sVar2 = *(short *)(iVar1 + 0x388);
  if (0 < sVar2) {
    if ((*(short *)(iVar1 + 0x3fa) == 0) && (*(short *)(iVar1 + 0x3fe) == 0)) {
      *(undefined2 *)(iVar1 + 0x388) = 0;
    }
    sVar2 = *(short *)(iVar1 + 0x388);
  }
  if ((sVar2 == 0) && (0 < *(short *)(iVar1 + 0x3b8))) {
    FUN_00560f20();
  }
  return;
}
#endif
