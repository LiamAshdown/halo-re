// unit_play_default_reaction_sound  (Ghidra: unit_play_default_reaction_sound)
// address 0x561030, size 261 bytes
// name confidence: 0.3 (phase2 candidate)   rewrite confidence: 0.25
// evidence: types/units.h unit_speech (built here as a stack literal: priority 6, scream_type
//   -1, sound_tag = the caller's sound, tail_ticks 0x18, the rest -1/0); unit_data
//   .current_speech.suppress_line_record (0x388 + 0x1a = 0x3a2), .current_speech.ai_line_index
//   (0x388 + 0x16 = 0x39e), .speech_sound_handle (0x400), .speech_started (0x3f4),
//   .speech_delay_ticks (0x3f8). unit_animation_change_priority_check (0x560d00),
//   unit_commit_speech (0x560f20), ai_communication_record_line_played (0x42f9e0).
// register convention: unit index in EAX, sound tag in EDX, sound handle in a stack param.
//   // blam-cc: param_1 (EAX) -> unit_index, param_2 (EDX) -> sound_tag, param_3 (stack) ->
//   //   sound_handle
// UNSURE: `param_1 = param_2;` overwrites the EAX-mapped local before unit_animation_change_priority_check and
//   unit_commit_speech are called with zero visible arguments, which most likely means EAX/other
//   registers are reused for their in/out parameters rather than the unit index changing
//   mid-function. Modelled here as unit_animation_change_priority_check(unit_index, 6, 0,
//   0, &dialogue_index, &chain) with dialogue_index seeded from sound_tag (matching
//   `&param_1` receiving the overwritten param_1), and unit_commit_speech(unit_index, &line, 3)
//   -- the mode that matches the pending_speech-clearing special case relevant to a reaction
//   line interrupting whatever else was queued.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

extern int32_t unit_animation_change_priority_check(uint32_t unit_index, int16_t requested_priority,
                                                      uint8_t allow_repeat, uint32_t *out_unknown_3f0,
                                                      int16_t *dialogue_index, int32_t *chain_value); // 0x560d00
extern int32_t unit_commit_speech(uint32_t unit_index, const unit_speech *source, int16_t mode); // 0x560f20
extern void ai_communication_record_line_played(uint32_t line_id, int16_t ai_line_index, uint32_t unknown); // 0x42f9e0

void unit_play_default_reaction_sound(uint32_t unit_index, datum_index sound_tag, datum_index sound_handle) // blam-cc: see file header
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    int16_t dialogue_index = (int16_t)sound_tag; // UNSURE: see file header
    int32_t chain = -1;
    unit_animation_change_priority_check(unit_index, 6, 0, 0, &dialogue_index, &chain);

    unit_speech line = {0};
    line.priority = 6;
    line.scream_type = -1;
    line.sound_tag = sound_tag;
    line.tail_ticks = 0x18;
    line.unknown_10 = -1;
    line.unknown_14 = -1;
    line.ai_line_index = -1;
    line.unknown_18 = -1;

    unit_commit_speech(unit_index, &line, 3); // UNSURE: see file header

    unit->speech_sound_handle = sound_handle;
    unit->speech_started = 1;
    unit->speech_delay_ticks = 0;
    if (unit->current_speech.suppress_line_record == 0) {
        ai_communication_record_line_played(6, unit->current_speech.ai_line_index, (uint32_t)-1);
    }
}

#if 0
Original Ghidra decompilation (0x561030):

void FUN_00561030(uint param_1,undefined4 param_2,undefined4 param_3)

{
  int iVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined2 local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  param_1 = param_2;
  local_34 = 0xffffffff;
  FUN_00560d00(6,0,0,&local_34,&param_1);
  puVar3 = &local_30;
  for (iVar2 = 0xc; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  local_14 = 0;
  local_10 = 0;
  local_c = 0;
  local_8 = 0;
  local_2c = param_2;
  local_4 = 0;
  local_30._0_2_ = 6;
  local_30._2_2_ = 0xffff;
  local_24 = 0x18;
  local_20 = 0xffffffff;
  local_1c = 0xffffffff;
  local_18 = 0xffff;
  FUN_00560f20();
  *(undefined4 *)(iVar1 + 0x400) = param_3;
  *(undefined1 *)(iVar1 + 0x3f4) = 1;
  *(undefined2 *)(iVar1 + 0x3f8) = 0;
  if (*(char *)(iVar1 + 0x3a2) == '\0') {
    ai_communication_record_line_played(6,*(undefined2 *)(iVar1 + 0x39e),0xffffffff);
  }
  return;
}
#endif
