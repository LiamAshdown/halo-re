// ui_controls_populate_input_row  (Ghidra: FUN_004a22e0, renamed)
// renamed from FUN_004a22e0 in the naming pass
// address 0x4a22e0, size 424 bytes, callers=0 in this build
// name confidence: 0.3   rewrite confidence: 0.2
// evidence: functions.md: "Populates a row of settings list-widgets (sensitivity-like 0-10 values
// and enable flags) from a controller/profile byte block." Same "find the first spinner_list
// among this row's children" walk repeated seven times as FUN_004a20f0/FUN_004a0050.
// register convention: widget in EAX (in_EAX), profile/controller record in EDI (unaff_EDI), both
// unresolved register reads. // blam-cc: EAX -> widget, EDI -> profile_record
// UNSURE / TYPES-GAP: DAT_007252e0 and DAT_00746120 are two capability-looking byte flags (gating
// a vibration-shaped row -- writes a float scale of either 1.0 or ~0.333 and toggles hidden on
// the row widget itself) not documented anywhere read this session. Field offsets 0xb78..0xb7f
// within profile_record were not cross-referenced against any known controller-settings layout.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t directsound_initialized; // 0x007252e0, TYPES-GAP
extern uint8_t directsound_eax_available; // 0x00746120, TYPES-GAP

static widget_instance *find_row_control(widget_instance *row)
{
    widget_instance *control;

    for (control = row->first_child; control != (widget_instance *)0 && control->widget_type != 2;
         control = control->next_sibling) {
    }
    return control;
}

// blam-cc: EAX -> widget, EDI -> profile_record
void ui_controls_populate_input_row(widget_instance *widget, const uint8_t *profile_record)
{
    widget_instance *row = widget->first_child;
    widget_instance *control;
    uint8_t value;

    control = find_row_control(row);
    value = profile_record[0xb78];
    control->selection_index = (value < 0xb) ? value : 10;

    row = row->next_sibling;
    control = find_row_control(row);
    value = profile_record[0xb79];
    control->selection_index = (value < 0xb) ? value : 10;

    row = row->next_sibling;
    control = find_row_control(row);
    value = profile_record[0xb7a];
    control->selection_index = (value < 0xb) ? value : 10;

    row = row->next_sibling;
    control = find_row_control(row);
    if (directsound_initialized == 0 || directsound_eax_available == 0) {
        row->hidden = 1;
        row->scale = 0.333f; // 0x3eaa7efa
        control->selection_index = 0;
    } else {
        control->selection_index = (profile_record[0xb7c] != 0) ? 1 : 0;
        row->hidden = 0;
        row->scale = 1.0f;
    }

    row = row->next_sibling;
    control = find_row_control(row);
    value = profile_record[0xb7d];
    control->selection_index = (value < 3) ? value : 2;

    row = row->next_sibling;
    control = find_row_control(row);
    control->selection_index =
        (profile_record[0xb7b] != 0 && directsound_initialized != 0 && directsound_eax_available != 0)
            ? 1 : 0;

    row = row->next_sibling;
    control = find_row_control(row);
    value = profile_record[0xb7f];
    control->selection_index = (value > 2) ? 2 : value;
}

#if 0
Original Ghidra decompilation (0x4a22e0):

void FUN_004a22e0(void)

{
  int iVar1;
  int iVar2;
  int in_EAX;
  undefined2 uVar3;
  ushort uVar4;
  int unaff_EDI;

  iVar1 = *(int *)(in_EAX + 0x34);
  for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  if (*(byte *)(unaff_EDI + 0xb78) < 0xb) {
    uVar4 = (ushort)*(byte *)(unaff_EDI + 0xb78);
  }
  else {
    uVar4 = 10;
  }
  *(ushort *)(iVar2 + 0x40) = uVar4;
  iVar1 = *(int *)(iVar1 + 0x2c);
  for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  if (*(byte *)(unaff_EDI + 0xb79) < 0xb) {
    uVar4 = (ushort)*(byte *)(unaff_EDI + 0xb79);
  }
  else {
    uVar4 = 10;
  }
  *(ushort *)(iVar2 + 0x40) = uVar4;
  iVar1 = *(int *)(iVar1 + 0x2c);
  for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  if (*(byte *)(unaff_EDI + 0xb7a) < 0xb) {
    uVar4 = (ushort)*(byte *)(unaff_EDI + 0xb7a);
  }
  else {
    uVar4 = 10;
  }
  *(ushort *)(iVar2 + 0x40) = uVar4;
  iVar1 = *(int *)(iVar1 + 0x2c);
  for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  if ((DAT_007252e0 == '\0') || (DAT_00746120 == '\0')) {
    *(undefined1 *)(iVar1 + 0x12) = 1;
    *(undefined4 *)(iVar1 + 0x24) = 0x3eaa7efa;
    *(undefined2 *)(iVar2 + 0x40) = 0;
  }
  else {
    *(ushort *)(iVar2 + 0x40) = (ushort)(*(char *)(unaff_EDI + 0xb7c) != '\0');
    *(undefined1 *)(iVar1 + 0x12) = 0;
    *(undefined4 *)(iVar1 + 0x24) = 0x3f800000;
  }
  iVar1 = *(int *)(iVar1 + 0x2c);
  for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  if (*(byte *)(unaff_EDI + 0xb7d) < 3) {
    uVar4 = (ushort)*(byte *)(unaff_EDI + 0xb7d);
  }
  else {
    uVar4 = 2;
  }
  *(ushort *)(iVar2 + 0x40) = uVar4;
  iVar1 = *(int *)(iVar1 + 0x2c);
  for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  if (((*(char *)(unaff_EDI + 0xb7b) == '\0') || (DAT_007252e0 == '\0')) || (DAT_00746120 == '\0'))
  {
    uVar3 = 0;
  }
  else {
    uVar3 = 1;
  }
  *(undefined2 *)(iVar2 + 0x40) = uVar3;
  for (iVar1 = *(int *)(*(int *)(iVar1 + 0x2c) + 0x34);
      (iVar1 != 0 && (*(short *)(iVar1 + 0xe) != 2)); iVar1 = *(int *)(iVar1 + 0x2c)) {
  }
  if (2 < *(byte *)(unaff_EDI + 0xb7f)) {
    *(undefined2 *)(iVar1 + 0x40) = 2;
    return;
  }
  *(ushort *)(iVar1 + 0x40) = (ushort)*(byte *)(unaff_EDI + 0xb7f);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
