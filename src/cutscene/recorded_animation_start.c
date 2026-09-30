// recorded_animation_start  (Ghidra: recorded_animation_start, already named)
// address 0x44a930, size 342 bytes
// name confidence: 0.6   rewrite confidence: 0.9 (verified against objdump 0x44a930..0x44aa85)
// evidence: types/cutscene.h recorded_animation struct comment: "Created by 0x44a930 (EAX unit,
//   CX Scenario.recorded_animations index, stack extra flags), ticked by 0x44aa90, searched by
//   0x44acc0 / 0x44ad20." and the module header's account of the whole address run. Confirmed
//   field-by-field against types/cutscene.h recorded_animation (unit_index 0x04, ticks_remaining
//   0x08, event_ticks 0x0c, event_cursor 0x10, control_data 0x14, decoder_state 0x54, codec_index
//   0x60, flags 0x0a) and ScenarioRecordedAnimation (version 0x20, unit_control_data_version
//   0x22, length_of_animation 0x24, recorded_animation_event_stream.pointer 0x38) via
//   `objdump -d -M intel --start-address=0x44a930 --stop-address=0x44aa90 bin/halo.exe`, which
//   also resolved the two callee stack-argument batches Ghidra's decompile left implicit (both
//   player_index_from_unit_index(unit_index) and recorded_animation_find_by_object(&out_index) are single-arg
//   cdecl calls whose stack cleanup the compiler deferred into one shared `add esp,8`).
// register convention: unit_index in EAX (in_EAX), scenario recorded_animations index in CX
//   (in_CX, 16-bit); one plain stack argument, extra_flags (a uint16_t ORed into the record's
//   flags at the end). Returns a bool in AL (Ghidra's `uint` return keeps only the low byte
//   meaningful -- the upper 24 bits are leftover register content nothing reads).
//   // blam-cc: EAX -> unit_index, CX -> scenario_animation_index, stack -> extra_flags
// UNSURE: player_index_from_unit_index's return value is discarded here (called for a side effect only, before
//   the "already playing" / "find existing record" checks); its own module and purpose are not
//   established (see src/units/unit_apply_fall_damage.c's identical extern).
// UNSURE: unit_get_flag_bit6 (EAX -> unit_index, returns bool in AL) has no prior extern
//   declaration anywhere in this repo; declared fresh here from this function's own disassembly.
//   0x4f67e0 is src/objects/object_set_in_pvs_pass_flag.c, called with BL = 0 (orphan pass 4
//   review: the draft dropped that register argument).
// UNSURE: the unit_flags bit this function clears at unit + 0x204 (mask 0xffffffbf, i.e. bit
//   0x40) is not among the bits types/units.h's unit_flags enum currently names; used here as a
//   literal mask with a comment rather than inventing an enumerator name for it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "objects.h"
#include "scenario.h"
#include "cutscene.h"
#include "fn_memory.h"
#include "fn_cutscene.h"

extern Scenario *global_scenario;           // 0x00746f8c
extern data_array *recorded_animations;     // 0x006b0a10
extern data_array *object_data;             // 0x008603b0
extern recorded_animation_codec *recorded_animation_codecs_by_version[4]; // 0x00686fe8


    // blam-cc: ESI unit_index (0x44a98e: ESI still holds the unit from 0x44a934)
extern int32_t player_index_from_unit_index(uint32_t unit_index); // 0x474db0, module not established; see
    // src/units/unit_apply_fall_damage.c's identical extern
extern void unit_refresh_targeting_flag_and_weapons(datum_index unit_handle, uint8_t attaching); // 0x569bf0, units module
    // blam-cc: stack -> unit_handle, CL -> attaching
extern uint8_t unit_get_flag_bit6(datum_index unit_index); // 0x569bc0, units module, UNSURE name
    // blam-cc: EAX -> unit_index
extern void object_set_in_pvs_pass_flag(uint32_t object_index, uint8_t in_pvs); // 0x4f67e0, objects module
    // blam-cc: EAX -> object_index, BL -> in_pvs (0x44aa5e `xor bl,bl` right before the call)

// blam-cc: EAX -> unit_index, CX -> scenario_animation_index, stack -> extra_flags
// hs cutscene_recording playback start: begins (or restarts) recorded animation
// scenario_animation_index of the current scenario driving unit_index, reusing unit_index's
// existing recorded_animation record if it already has one (idle or finished), otherwise
// allocating a new one. Does nothing (returns 0) if unit_index or scenario_animation_index is
// none, the index is out of range, or the unit already has a not-yet-finished recording in
// progress. Returns 1 on success.
uint8_t recorded_animation_start(datum_index unit_index, int16_t scenario_animation_index, uint16_t extra_flags)
{
    ScenarioRecordedAnimation *def;
    recorded_animation *record;
    datum_index existing_index;
    datum_index new_index;
    unit_data *unit;
    uint8_t version;

    if (unit_index == (datum_index)k_datum_index_none) {
        return 0;
    }
    if (scenario_animation_index == -1) {
        return 0;
    }
    if ((int32_t)scenario_animation_index >= (int32_t)global_scenario->recorded_animations.count) {
        return 0;
    }

    player_index_from_unit_index((uint32_t)unit_index);
    record = recorded_animation_find_by_object(unit_index, &existing_index);
    def = (ScenarioRecordedAnimation *)global_scenario->recorded_animations.pointer + scenario_animation_index;

    if (recorded_animation_object_is_playing(unit_index) != 0) {
        return 0;
    }

    if (record == (recorded_animation *)0) {
        new_index = datum_new(recorded_animations);
        if (new_index == (datum_index)k_datum_index_none) {
            return 0;
        }
        record = &((recorded_animation *)recorded_animations->data)[new_index & 0xffff];
        if (record == (recorded_animation *)0) {
            return 0;
        }
    }

    record->unit_index = unit_index;
    record->event_ticks = 0;
    record->ticks_remaining = def->length_of_animation;
    record->event_cursor = (uint8_t *)def->recorded_animation_event_stream.pointer;
    version = (uint8_t)def->version;
    record->flags = record->flags & ~(uint16_t)_recorded_animation_flag_finished;
    record->codec_index = (int16_t)(version - 1);
    recorded_animation_codecs_by_version[record->codec_index]->begin(&record->decoder_state,
        &record->control_data, &record->event_cursor, (uint8_t)def->unit_control_data_version);

    unit_refresh_targeting_flag_and_weapons(unit_index, 1);
    if (unit_get_flag_bit6(unit_index) != 0) {
        record->flags = record->flags | _recorded_animation_flag_restore_object_flag_40;
    } else {
        record->flags = record->flags & ~(uint16_t)_recorded_animation_flag_restore_object_flag_40;
    }

    unit = (unit_data *)((uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data + k_unit_data_offset);
    unit->flags = unit->flags & ~0x00000040u; // bit not named in types/units.h unit_flags, see UNSURE above
    unit = (unit_data *)((uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data + k_unit_data_offset);
    unit->flags = unit->flags | _unit_flag_unknown_8000000;

    object_set_in_pvs_pass_flag(unit_index, 0);
    record->flags = record->flags | extra_flags;
    return 1;
}

#if 0
Original Ghidra decompilation (0x44a930):

uint recorded_animation_start(ushort param_1)

{
  uint *puVar1;
  byte bVar2;
  char cVar3;
  uint in_EAX;
  int iVar4;
  uint uVar5;
  int iVar6;
  undefined2 extraout_var;
  short in_CX;
  short sVar7;
  int iVar8;
  undefined8 uVar9;
  undefined1 local_4 [4];

  uVar5 = in_EAX & 0xffffff00;
  if (((in_EAX != 0xffffffff) && (in_CX != -1)) && ((int)in_CX < *(int *)(global_scenario + 0x36c)))
  {
    FUN_00474db0();
    iVar4 = recorded_animation_find_by_object(local_4);
    iVar8 = in_CX * 0x40 + *(int *)(global_scenario + 0x370);
    uVar5 = recorded_animation_object_is_playing();
    if ((char)uVar5 == '\0') {
      if (iVar4 != 0) {
LAB_0044a9c8:
        *(uint *)(iVar4 + 4) = in_EAX;
        *(undefined4 *)(iVar4 + 0xc) = 0;
        *(undefined2 *)(iVar4 + 8) = *(undefined2 *)(iVar8 + 0x24);
        *(undefined4 *)(iVar4 + 0x10) = *(undefined4 *)(iVar8 + 0x38);
        bVar2 = *(byte *)(iVar8 + 0x20);
        *(byte *)(iVar4 + 10) = *(byte *)(iVar4 + 10) & 0xfe;
        sVar7 = bVar2 - 1;
        *(short *)(iVar4 + 0x60) = sVar7;
        (**(code **)(&PTR_PTR_00686fe8)[sVar7])
                  (iVar4 + 0x54,iVar4 + 0x14,iVar4 + 0x10,*(undefined1 *)(iVar8 + 0x22));
        unit_refresh_targeting_flag_and_weapons();
        cVar3 = unit_get_flag_bit6();
        if (cVar3 == '\0') {
          *(byte *)(iVar4 + 10) = *(byte *)(iVar4 + 10) & 0xfb;
        }
        else {
          *(byte *)(iVar4 + 10) = *(byte *)(iVar4 + 10) | 4;
        }
        iVar8 = DAT_008603b0;
        iVar6 = (in_EAX & 0xffff) * 0xc;
        puVar1 = (uint *)(*(int *)(iVar6 + 8 + *(int *)(DAT_008603b0 + 0x34)) + 0x204);
        *puVar1 = *puVar1 & 0xffffffbf;
        puVar1 = (uint *)(*(int *)(iVar6 + 8 + *(int *)(iVar8 + 0x34)) + 0x204);
        *puVar1 = *puVar1 | 0x8000000;
        FUN_004f67e0();
        *(ushort *)(iVar4 + 10) = *(ushort *)(iVar4 + 10) | param_1;
        return CONCAT31((int3)(CONCAT22(extraout_var,param_1) >> 8),1);
      }
      uVar9 = datum_new();
      uVar5 = (uint)uVar9;
      if (uVar5 != 0xffffffff) {
        iVar4 = (uVar5 & 0xffff) * 100 + *(int *)((int)((ulonglong)uVar9 >> 0x20) + 0x34);
        uVar5 = 0;
        if (iVar4 != 0) goto LAB_0044a9c8;
      }
    }
    uVar5 = uVar5 & 0xffffff00;
  }
  return uVar5;
}
#endif
