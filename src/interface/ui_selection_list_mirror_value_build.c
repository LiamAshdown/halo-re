// ui_selection_list_mirror_value_build  (Ghidra: FUN_004a6810, renamed)
// address 0x4a6810, size 105 bytes
// name confidence: 0.3 (chosen)   rewrite confidence: 0.55
// evidence: types/interface.h widget_instance (extended_description, first_child, next_sibling,
// selection_index, background_bitmap_frame all match); the "selected combo index" reuse of
// widget_instance+0x3c is the same one established in ui_list_widget_rebuild_rows.c.
// Calls ui_list_widget_rebuild_rows with LAB_004a8310, a tiny stub inside ui_list_widget_rebuild_rows's own
// code (0x4a7db0..~0x4a8305) that this pass did not analyze separately since Ghidra does not
// list it as its own function; treated as an opaque default item-format callback.
// UNSURE: set_profile_name (0x49c710, out of this module's range) writes its formatted string
// through an inherited EBX register this pack could not resolve to a specific widget; the call
// is preserved exactly as compiled. UNSURE: the exact byte count copied from the profile record
// (0x7ff dwords, one dword short of the full 0x2004 byte record) is kept as Ghidra shows it.
// register convention: widget as the recognized parameter (param_1).
// reconciled: R56 0x00712dd8 uint8_t saved_profile_records[3][0x2004] -> saved_games.h saved_player_profile_slot saved_player_profile_slots[k_maximum_local_player_profiles] (one 0x2004-byte slot; a second would overlap 0x00714dde); same bytes copied/read

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include <string.h>

extern saved_player_profile_slot saved_player_profile_slots[k_maximum_local_player_profiles]; // 0x00712dd8, saved_games.h
                                                  // (see types/interface.h player_control_settings note)
extern void ui_list_widget_rebuild_rows(widget_instance *widget, ui_list_item_format_function format_item); // 0x4a7db0
extern uint8_t ui_list_default_item_format(void *item_buffer, int32_t item_index, void *list_items); // 0x4a8310, UNSURE: not analyzed separately
extern void set_profile_name(widget_instance *widget, const uint16_t *name_source); // 0x49c710, blam-cc: EBX widget

// Rebuilds this widget's list rows, then mirrors its cached "selected combo index" (offset
// 0x3c) into a widget three levels under extended_description, keeping that paired display in
// sync with the selection.
void ui_selection_list_mirror_value_build(widget_instance *widget)
{
    uint8_t profile_record[0x1ffc]; // one dword short of the full 0x2004 byte record, as Ghidra shows
    int16_t selected_value; // uVar1
    widget_instance *target; // iVar2

    memcpy(profile_record, &saved_player_profile_slots[0].profile, sizeof(profile_record));
    set_profile_name(widget->extended_description->first_child, (const uint16_t *)(profile_record + 2));

    selected_value = *(int16_t *)((uint8_t *)widget + 0x3c); // UNSURE offset, see header
    target = widget->extended_description->first_child->next_sibling->first_child;
    target->selection_index = selected_value;
    target->next_sibling->background_bitmap_frame = selected_value;

    ui_list_widget_rebuild_rows(widget, (void *)ui_list_default_item_format);
}

#if 0
Original Ghidra decompilation (0x4a6810):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_004a6810(int param_1)

{
  undefined2 uVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  undefined4 local_2008;
  undefined4 uStack_c;

  uStack_c = 0x4a6820;
  FUN_004a7db0(param_1,&LAB_004a8310);
  puVar3 = &DAT_00712dd8;
  puVar4 = &local_2008;
  for (iVar2 = 0x7ff; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = *puVar3;
    puVar3 = puVar3 + 1;
    puVar4 = puVar4 + 1;
  }
  set_profile_name((int)&local_2008 + 2);
  uVar1 = *(undefined2 *)(param_1 + 0x3c);
  iVar2 = *(int *)(*(int *)(*(int *)(*(int *)(param_1 + 0x4c) + 0x34) + 0x2c) + 0x34);
  *(undefined2 *)(iVar2 + 0x40) = uVar1;
  *(undefined2 *)(*(int *)(iVar2 + 0x2c) + 0x58) = uVar1;
  return;
}
#endif
