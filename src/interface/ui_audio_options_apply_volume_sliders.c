// ui_audio_options_apply_volume_sliders  (Ghidra: FUN_004a76d0, renamed)
// address 0x4a76d0, size 426 bytes
// name confidence: 0.35 (chosen)   rewrite confidence: 0.4
// evidence: phase-4 summary "Applies the current volume-slider widget values live to the sound
// engine and refreshes a related widget's enabled state"; types/interface.h widget_instance
// (first_child, next_sibling, widget_type, selection_index, hidden, scale all match); calls
// widget_extended_description_sync_selection.c (0x4a66b0, already established in this pass).
// UNSURE: sound_set_master_gain (0x548590, out of range) is presumably a fourth sound_set_*_gain sibling
// (voice chat gain, by call order) but was not confirmed independently. UNSURE: the final row
// jumps row->next_sibling->next_sibling past an intervening row to reach the widget it
// hides/dims; not explained by anything in this pack.
// register convention: widget as the recognized parameter, though Ghidra mistyped it as float
// (it is cast back to int for the widget_extended_description_sync_selection call); passed
// through unchanged here as a pointer.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern void sound_set_master_gain(float gain); // 0x548590, UNSURE: presumably a sound_set_*_gain sibling, not in this module's range
extern void sound_set_effects_gain(float gain); // 0x5487b0
extern void sound_set_music_gain(float gain);   // 0x548680
extern void widget_extended_description_sync_selection(widget_instance *widget); // 0x4a66b0

// Finds the first spinner_list embedded under `row`'s children and returns its selection_index
// converted from tenths and clamped to [0, 1].
static float find_row_spinner_gain(widget_instance *row)
{
    widget_instance *spinner = row->first_child;
    float value;
    while (spinner != 0 && spinner->widget_type != uiwidgettype_spinner_list) {
        spinner = spinner->next_sibling;
    }
    value = (float)(int16_t)spinner->selection_index * 0.1f;
    if (value < 0.0f) {
        value = 0.0f;
    } else if (value > 1.0f) {
        value = 1.0f;
    }
    return value;
}

// Reads four rows of volume-slider widgets and pushes their (tenths, 0..10) values to the
// sound engine as 0..1 gains, then shows/hides a widget two siblings past the fourth row
// depending on whether that row's slider is at zero, before refreshing the paired
// extended_description widget.
void ui_audio_options_apply_volume_sliders(widget_instance *widget)
{
    widget_instance *row;
    widget_instance *spinner;
    widget_instance *target;

    row = widget->first_child;
    sound_set_master_gain(find_row_spinner_gain(row));

    row = row->next_sibling;
    sound_set_effects_gain(find_row_spinner_gain(row));

    row = row->next_sibling;
    sound_set_music_gain(find_row_spinner_gain(row));

    row = row->next_sibling;
    spinner = row->first_child;
    while (spinner != 0 && spinner->widget_type != uiwidgettype_spinner_list) {
        spinner = spinner->next_sibling;
    }
    target = row->next_sibling->next_sibling;
    if (spinner->selection_index == 0) {
        target->hidden = 1;
        target->scale = 0.333f;
    } else {
        target->hidden = 0;
        target->scale = 1.0f;
    }
    widget_extended_description_sync_selection(widget);
}

#if 0
Original Ghidra decompilation (0x4a76d0):

void FUN_004a76d0(float param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;

  iVar3 = (int)param_1;
  iVar1 = *(int *)((int)param_1 + 0x34);
  for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  param_1 = (float)(int)*(short *)(iVar2 + 0x40) * 0.1;
  if (0.0 <= param_1) {
    if (1.0 < param_1) {
      param_1 = 1.0;
    }
  }
  else {
    param_1 = 0.0;
  }
  FUN_00548590(param_1);
  iVar1 = *(int *)(iVar1 + 0x2c);
  for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  param_1 = (float)(int)*(short *)(iVar2 + 0x40) * 0.1;
  if (0.0 <= param_1) {
    if (1.0 < param_1) {
      param_1 = 1.0;
    }
  }
  else {
    param_1 = 0.0;
  }
  sound_set_effects_gain(param_1);
  iVar1 = *(int *)(iVar1 + 0x2c);
  for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  param_1 = (float)(int)*(short *)(iVar2 + 0x40) * 0.1;
  if (0.0 <= param_1) {
    if (1.0 < param_1) {
      param_1 = 1.0;
    }
  }
  else {
    param_1 = 0.0;
  }
  sound_set_music_gain(param_1);
  iVar1 = *(int *)(iVar1 + 0x2c);
  for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  iVar1 = *(int *)(*(int *)(iVar1 + 0x2c) + 0x2c);
  if (*(short *)(iVar2 + 0x40) == 0) {
    *(undefined1 *)(iVar1 + 0x12) = 1;
    *(undefined4 *)(iVar1 + 0x24) = 0x3eaa7efa;
    FUN_004a66b0(iVar3);
    return;
  }
  *(undefined1 *)(iVar1 + 0x12) = 0;
  *(undefined4 *)(iVar1 + 0x24) = 0x3f800000;
  FUN_004a66b0(iVar3);
  return;
}
#endif
