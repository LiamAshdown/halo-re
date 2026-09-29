// unit_update_animation_timers  (Ghidra: FUN_00561620)
// address 0x561620, size 700 bytes
// name confidence: 0.35 (phase2 candidate; it is the unit speech tick)   rewrite confidence: 0.85
// REWRITTEN from objdump 0x561620..0x5618db (the draft called the sound and AI communication helpers without
//   their register / stack arguments). EAX: unit. A pending dialogue variant (+0x204 bit 0x100) is chosen
//   (0x561990). Timers: +0x3e8 (reloading 22 while +0x3ea has repeats), +0x3ec (twice a tick). While a line
//   plays (+0x388 priority > 0): its delay (+0x3f8) runs out first; then, once, the sound (+0x38c) starts at
//   the "head" marker (0x543ce0, else the origin facing forward) into +0x400 and the line is gated
//   (0x42e970: CX priority, EDX the speech record +0x398, stack the unit); +0x3fc counts down; +0x3fa counts
//   the sound out (clearing +0x400 at 0); after it the event line plays once (0x42eee0) and the tail
//   (+0x3fe) runs out, zeroing +0x3fc. When +0x3fc is 0 the reaction propagates once (0x42e9c0). A finished
//   line clears the priority and a queued one (+0x3b8) is committed (0x560f20, DX 3).
// blam-cc: EAX -> unit_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *object_data;     // 0x008603b0
extern real_point3d *global_zero_vector3d_pointer; // 0x006966f8
extern real_vector3d *global_forward3d_pointer; // 0x00696718

extern void unit_choose_dialogue_variant(uint32_t unit_index); // 0x561990, EAX
extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
    uint32_t flags); // 0x4f6080
extern datum_index sound_start_at_object_marker(datum_index object_index, Point3D *position, Vector3D *forward,
    datum_index definition_index, int16_t node_index, float scale, uint32_t first_person_hint); // 0x543ce0, ESI, ECX, EAX, stack


extern int32_t unit_commit_speech(uint32_t unit_index, const void *source, int16_t mode); // 0x560f20, EAX, ECX, DX

static void count_down(uint8_t *field)
{
    int16_t value = *(int16_t *)field;

    if (value > 0) {
        *(int16_t *)field = (int16_t)(value - 1);
    }
}

void unit_update_animation_timers(uint32_t unit_index)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;

    if (obj[0x205] & 0x1) {
        unit_choose_dialogue_variant(unit_index);
        ((unit_object *)obj)->unit.flags &= 0xfffffeff;
    }
    if (((struct unit_object *)obj)->unit.unknown_3e8 > 0) {
        int16_t value = (int16_t)(((struct unit_object *)obj)->unit.unknown_3e8 - 1);

        ((struct unit_object *)obj)->unit.unknown_3e8 = value;
        if (value == 0 && ((struct unit_object *)obj)->unit.unknown_3ea > 0) {
            ((struct unit_object *)obj)->unit.unknown_3ea = (int16_t)(((struct unit_object *)obj)->unit.unknown_3ea - 1);
            ((struct unit_object *)obj)->unit.unknown_3e8 = 0x16;
        }
    }
    count_down(obj + 0x3ec);
    count_down(obj + 0x3ec);
    if (((unit_object *)obj)->unit.current_speech.priority > 0) {
        if (((unit_object *)obj)->unit.speech_delay_ticks > 0) {
            ((unit_object *)obj)->unit.speech_delay_ticks = (int16_t)(((unit_object *)obj)->unit.speech_delay_ticks - 1);
            goto tail;
        }
        if (obj[0x3f4] == 0) {
            object_marker marker;
            Point3D position;
            Vector3D forward;
            int16_t node = 0;

            if ((int16_t)object_get_node_local_transform(unit_index, "head", &marker, 1) != 0) {
                uint8_t *raw = (uint8_t *)&marker;

                position = *(Point3D *)(raw + 0x2c);
                forward = *(Vector3D *)(raw + 0x8);
                node = *(int16_t *)raw;
            } else {
                position = *(Point3D *)global_zero_vector3d_pointer;
                forward = *(Vector3D *)global_forward3d_pointer;
            }
            if (((unit_object *)obj)->unit.current_speech.sound_tag != k_datum_index_none) {
                ((unit_object *)obj)->unit.speech_sound_handle = sound_start_at_object_marker(unit_index, &position, &forward,
                    ((unit_object *)obj)->unit.current_speech.sound_tag, node, 1.0f, 0);
            }
            ai_communication_gate_line_played(((unit_object *)obj)->unit.current_speech.priority, (ai_communication_record *)(obj + 0x398),
                unit_index);
            obj[0x3f4] = 1;
        }
        count_down(obj + 0x3fc);
        if (((unit_object *)obj)->unit.speech_duration_ticks > 0) {
            int16_t value = (int16_t)(((unit_object *)obj)->unit.speech_duration_ticks - 1);

            ((unit_object *)obj)->unit.speech_duration_ticks = value;
            if (value == 0) {
                ((unit_object *)obj)->unit.speech_sound_handle = k_datum_index_none;
            }
            goto tail;
        }
        if (obj[0x3f6] == 0) {
            ai_communication_play_event_line(unit_index, (int16_t)*(uint16_t *)&((unit_object *)obj)->unit.current_speech.scream_type, 0, k_datum_index_none,
                (uint32_t *)(obj + 0x398));
            obj[0x3f6] = 1;
        }
        count_down(obj + 0x3fe);
        if (((unit_object *)obj)->unit.speech_tail_ticks == 0) {
            ((unit_object *)obj)->unit.speech_lipsync_ticks = 0;
        }
    }
tail:
    if (((unit_object *)obj)->unit.speech_lipsync_ticks == 0 && obj[0x3f5] == 0) {
        ai_propagate_communication_reaction(unit_index, (ai_communication_order *)(obj + 0x398));
        obj[0x3f5] = 1;
    }
    if (((unit_object *)obj)->unit.current_speech.priority > 0 && ((unit_object *)obj)->unit.speech_duration_ticks == 0 && ((unit_object *)obj)->unit.speech_tail_ticks == 0) {
        ((unit_object *)obj)->unit.current_speech.priority = 0;
    }
    if (((unit_object *)obj)->unit.current_speech.priority == 0 && ((unit_object *)obj)->unit.pending_speech.priority > 0) {
        unit_commit_speech(unit_index, obj + 0x3b8, 3);
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
