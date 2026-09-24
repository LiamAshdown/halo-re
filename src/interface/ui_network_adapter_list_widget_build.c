// ui_network_adapter_list_widget_build  (Ghidra: FUN_004a8440, renamed)
// address 0x4a8440, size 134 bytes
// name confidence: 0.3 (chosen)   rewrite confidence: 0.55
// evidence: types/interface.h widget_instance and map_list_entry (map_id at +0x04); the
// "selected combo index" reuse of widget_instance+0x3c established in
// ui_list_widget_rebuild_rows.c.
// UNSURE: set_profile_name writes through an inherited EBX this pack could not resolve; call
// preserved exactly as compiled.
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
extern map_list_entry *map_list;                 // 0x00712dcc
extern void ui_list_widget_rebuild_rows(widget_instance *widget, void *format_item); // 0x4a7db0
extern uint8_t ui_list_item_format_name_and_cache_flag(uint16_t *out_name, int32_t item_index); // 0x4a83d0
extern void set_profile_name(widget_instance *widget, const uint16_t *name_source); // 0x49c710, blam-cc: EBX widget

// Rebuilds this widget's rows with the map-name formatter, then mirrors the widget's cached
// "selected combo index" (offset 0x3c) as a map_id into the three sibling widgets under
// extended_description.
void ui_network_adapter_list_widget_build(widget_instance *widget)
{
    uint8_t profile_record[0x1ffc];
    widget_instance *target1;
    widget_instance *target2;
    widget_instance *target3;
    int16_t map_id;

    ui_list_widget_rebuild_rows(widget, (void *)ui_list_item_format_name_and_cache_flag);

    memcpy(profile_record, &saved_player_profile_slots[0].profile, sizeof(profile_record));
    set_profile_name(widget->extended_description->first_child, (const uint16_t *)(profile_record + 2));

    target1 = widget->extended_description->first_child->next_sibling->first_child;
    target2 = target1->next_sibling;
    target3 = target2->next_sibling;

    map_id = (int16_t)map_list[*(int16_t *)((uint8_t *)widget + 0x3c)].map_id; // UNSURE offset, see ui_list_widget_rebuild_rows.c
    target1->selection_index = map_id;
    target2->background_bitmap_frame = map_id;
    target3->selection_index = map_id;
}

#if 0
Original Ghidra decompilation (0x4a8440):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_004a8440(int param_1)

{
  undefined2 *puVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  undefined4 local_2008;
  undefined4 uStack_c;

  uStack_c = 0x4a8450;
  FUN_004a7db0(param_1,FUN_004a83d0);
  puVar5 = &DAT_00712dd8;
  puVar6 = &local_2008;
  for (iVar4 = 0x7ff; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar6 = *puVar5;
    puVar5 = puVar5 + 1;
    puVar6 = puVar6 + 1;
  }
  set_profile_name((int)&local_2008 + 2);
  iVar4 = *(int *)(*(int *)(*(int *)(*(int *)(param_1 + 0x4c) + 0x34) + 0x2c) + 0x34);
  iVar2 = *(int *)(iVar4 + 0x2c);
  iVar3 = *(int *)(iVar2 + 0x2c);
  puVar1 = (undefined2 *)(DAT_00712dcc + 4 + *(short *)(param_1 + 0x3c) * 0xc);
  *(undefined2 *)(iVar4 + 0x40) = *puVar1;
  *(undefined2 *)(iVar2 + 0x58) = *puVar1;
  *(undefined2 *)(iVar3 + 0x40) = *puVar1;
  return;
}
#endif
