// ui_controls_options_populate_from_profile  (Ghidra: FUN_004a0050, renamed)
// renamed from FUN_004a0050 in the naming pass
// address 0x4a0050, size 492 bytes, callers=0 in this build
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: functions.md: "Populates the controller/options menu's list_head widgets (control
// scheme, invert, sensitivity, etc.) with the values from the currently loaded player profile."
// types/interface.h names 0x00714e7c selected_saved_item and 0x00714e80 saved_item_working_copy.
// register convention: cdecl, the one recognized stack parameter (widget).
// UNSURE (significant): Ghidra could not emulate an indirect jump inside this function ("Could
// not emulate address calculation ... Treating indirect jump as call") and separately renders the
// `selected_saved_item`/`saved_item_working_copy` gate as `uVar4 & 0x714e80` / `& 0x714e00` --
// numeric literals matching those globals' own addresses rather than symbol references, which
// reads as Ghidra having lost the real source pattern here. This file instead implements the far
// more sensible reading consistent with those globals' documented meaning: "if the selected saved
// item is a profile (not a variant) and a working copy exists, populate the widgets from it,
// else return 0" -- NOT a literal byte-for-byte transcription of the raw AND-with-address form,
// which would be nonsensical as real code. The five repeated "find the first spinner_list among
// this row's children" walks, and the field offsets read from the working-copy record (0x80,
// 0x7c, 0x58, 0x34, 0x78), are preserved as Ghidra shows them. The jump table at 0x4a0268/
// 0x4a0283 (25 entries, byte-index into a function-pointer table) is declared as opaque extern
// data exactly as Ghidra printed it, since its contents are not visible to this rewrite; this is
// the single least-confident piece of this whole session's work and should be revisited with an
// objdump/hex-dump pass.

// Phase-4 s2 review: the gate is the sbb/not/and select of the working copy buffer at 0x00714e80
// (a record, not a pointer): the record is non-NULL only when the low nibble of
// selected_saved_item is 1 (a variant). The earlier rewrite had the test inverted and
// read 0x00714e80 as a pointer.
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern int32_t selected_saved_item;   // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself

extern void *jump_table_004a0268[25]; // 0x4a0268, TYPES-GAP, UNSURE: see file header
extern uint8_t jump_table_index_004a0283[25]; // 0x4a0283, TYPES-GAP, UNSURE: see file header

// Finds the first spinner_list among `row`'s children.
static widget_instance *find_row_control(widget_instance *row)
{
    widget_instance *control;

    for (control = row->first_child; control != (widget_instance *)0 && control->widget_type != 2;
         control = control->next_sibling) {
    }
    return control;
}

uint32_t ui_controls_options_populate_from_profile(widget_instance *widget)
{
    uint8_t *record;
    widget_instance *row;
    widget_instance *control;
    int32_t field;

    if ((selected_saved_item & 0xf) != 1) { // the record is used only while a variant (nibble 1) is selected
        return 0;
    }
    record = saved_item_working_copy;

    row = widget->first_child;
    control = find_row_control(row);
    field = *(int32_t *)(record + 0x80);
    control->selection_index = (field == 1 || field == 2) ? (int16_t)field : 0;

    row = row->next_sibling;
    control = find_row_control(row);
    field = *(int32_t *)(record + 0x7c);
    control->selection_index = (field == 1 || field == 2) ? (int16_t)field : 0;

    row = row->next_sibling;
    control = find_row_control(row);
    field = *(int32_t *)(record + 0x58);
    if ((uint32_t)(field - 1) < 0x19) {
        uint8_t index = jump_table_index_004a0283[field];
        uint32_t (*handler)(void) = (uint32_t (*)(void))jump_table_004a0268[index];
        return handler();
    }
    control->selection_index = 0;

    row = row->next_sibling;
    control = find_row_control(row);
    control->selection_index = (record[0x34] == 0) ? 1 : 0;

    row = row->next_sibling;
    control = find_row_control(row);
    field = *(int32_t *)(record + 0x78);
    switch (field) {
    case 18000: control->selection_index = 1; return 1;
    case 27000: control->selection_index = 2; return 1;
    case 36000: control->selection_index = 3; return 1;
    case 45000: control->selection_index = 4; return 1;
    case 54000: control->selection_index = 5; return 1;
    case 81000: control->selection_index = 6; return 1;
    default: control->selection_index = 0; return 1;
    }
}

#if 0
Original Ghidra decompilation (0x4a0050):

uint FUN_004a0050(int param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;

  uVar4 = ~-(uint)((DAT_00714e7c & 0xf) != 1);
  uVar5 = uVar4 & 0x714e80;
  if (uVar5 == 0) {
    return uVar4 & 0x714e00;
  }
  iVar1 = *(int *)(param_1 + 0x34);
  for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  iVar3 = *(int *)(uVar5 + 0x80);
  if (iVar3 == 0) {
LAB_004a00c7:
    *(undefined2 *)(iVar2 + 0x40) = 0;
  }
  else if (iVar3 == 1) {
    *(undefined2 *)(iVar2 + 0x40) = 1;
  }
  else {
    if (iVar3 != 2) goto LAB_004a00c7;
    *(undefined2 *)(iVar2 + 0x40) = 2;
  }
  iVar1 = *(int *)(iVar1 + 0x2c);
  for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  iVar3 = *(int *)(uVar5 + 0x7c);
  if (iVar3 != 0) {
    if (iVar3 == 1) {
      *(undefined2 *)(iVar2 + 0x40) = 1;
      goto LAB_004a0101;
    }
    if (iVar3 == 2) {
      *(undefined2 *)(iVar2 + 0x40) = 2;
      goto LAB_004a0101;
    }
  }
  *(undefined2 *)(iVar2 + 0x40) = 0;
LAB_004a0101:
  iVar1 = *(int *)(iVar1 + 0x2c);
  for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  if (*(int *)(uVar5 + 0x58) - 1U < 0x19) {
                    /* WARNING: Could not emulate address calculation at 0x004a011d */
                    /* WARNING: Treating indirect jump as call */
    uVar4 = (*(code *)(&PTR_LAB_004a0268)[(byte)(&DAT_004a0283)[*(int *)(uVar5 + 0x58)]])();
    return uVar4;
  }
  *(undefined2 *)(iVar2 + 0x40) = 0;
  iVar1 = *(int *)(iVar1 + 0x2c);
  for (iVar2 = *(int *)(iVar1 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 2));
      iVar2 = *(int *)(iVar2 + 0x2c)) {
  }
  if (*(char *)(uVar5 + 0x34) == '\0') {
    *(undefined2 *)(iVar2 + 0x40) = 1;
  }
  else {
    *(undefined2 *)(iVar2 + 0x40) = 0;
  }
  for (iVar1 = *(int *)(*(int *)(iVar1 + 0x2c) + 0x34);
      (iVar1 != 0 && (*(short *)(iVar1 + 0xe) != 2)); iVar1 = *(int *)(iVar1 + 0x2c)) {
  }
  iVar2 = *(int *)(uVar5 + 0x78);
  if (iVar2 < 0x8ca1) {
    if (iVar2 == 36000) {
      *(undefined2 *)(iVar1 + 0x40) = 3;
      return 1;
    }
    if (iVar2 != 0) {
      if (iVar2 == 18000) {
        *(undefined2 *)(iVar1 + 0x40) = 1;
        return 1;
      }
      if (iVar2 == 27000) {
        *(undefined2 *)(iVar1 + 0x40) = 2;
        return 1;
      }
    }
  }
  else {
    if (iVar2 == 45000) {
      *(undefined2 *)(iVar1 + 0x40) = 4;
      return 1;
    }
    if (iVar2 == 54000) {
      *(undefined2 *)(iVar1 + 0x40) = 5;
      return 1;
    }
    if (iVar2 == 81000) {
      *(undefined2 *)(iVar1 + 0x40) = 6;
      return 1;
    }
  }
  *(undefined2 *)(iVar1 + 0x40) = 0;
  return 1;
}
#endif
