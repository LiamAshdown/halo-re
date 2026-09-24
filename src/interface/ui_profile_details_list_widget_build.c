// ui_profile_details_list_widget_build  (Ghidra: FUN_004a8360, renamed)
// address 0x4a8360, size 102 bytes
// name confidence: 0.3 (chosen)   rewrite confidence: 0.7
// evidence: types/interface.h widget_instance (extended_description, first_child, next_sibling,
// background_bitmap_frame).
// UNSURE: ui_level_carousel_row_refresh (0x4a4e20, out of this module's range) is called with no visible
// arguments in Ghidra; its own body reads in_ECX as a widget (first_child at +0x34) and in_EAX
// as a small index into a per-controller flag table (0x0071901c), so it is declared here taking
// (widget, widget->controller_index) as the best available guess, not confirmed by a caller.
// UNSURE: set_profile_name (0x49c710, out of range) writes through an inherited EBX this pack
// could not resolve; the call is preserved exactly as compiled.
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
extern void ui_list_widget_rebuild_rows(widget_instance *widget, void *format_item); // 0x4a7db0
extern uint8_t ui_list_default_item_format(void *item_buffer, int32_t item_index, void *list_items); // 0x4a8310, UNSURE: not analyzed separately
extern void set_profile_name(widget_instance *widget, const uint16_t *name_source); // 0x49c710, blam-cc: EBX widget
extern void ui_level_carousel_row_refresh(widget_instance *widget, int32_t slot_index); // 0x4a4e20, blam-cc: ECX widget, EAX index

// Rebuilds this widget's list rows, refreshes the profile-name label, clears the description
// value's highlight, then hands off to ui_level_carousel_row_refresh to lay out the three profile-detail rows.
void ui_profile_details_list_widget_build(widget_instance *widget)
{
    uint8_t profile_record[0x1ffc];

    ui_list_widget_rebuild_rows(widget, (void *)ui_list_default_item_format);

    memcpy(profile_record, &saved_player_profile_slots[0].profile, sizeof(profile_record));
    set_profile_name(widget->extended_description->first_child, (const uint16_t *)(profile_record + 2));

    widget->extended_description->first_child->next_sibling->background_bitmap_frame = 0;
    ui_level_carousel_row_refresh(widget->extended_description->first_child->next_sibling,
                 *(int16_t *)((uint8_t *)widget + 0x3c)); // list selection, movsx [widget+0x3c]
}

#if 0
Original Ghidra decompilation (0x4a8360):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_004a8360(int param_1)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  undefined4 local_2008;
  undefined4 uStack_c;

  uStack_c = 0x4a8370;
  FUN_004a7db0(param_1,&LAB_004a8310);
  puVar2 = &DAT_00712dd8;
  puVar3 = &local_2008;
  for (iVar1 = 0x7ff; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  set_profile_name((int)&local_2008 + 2);
  *(undefined2 *)(*(int *)(*(int *)(*(int *)(param_1 + 0x4c) + 0x34) + 0x2c) + 0x58) = 0;
  FUN_004a4e20();
  return;
}
#endif
