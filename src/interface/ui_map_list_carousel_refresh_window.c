// ui_map_list_carousel_refresh_window  (Ghidra: FUN_004a6940, renamed)
// address 0x4a6940, size 160 bytes
// name confidence: 0.3 (chosen; the phase-4 auto-summary calls this "per-network-adapter" but
// the evidence points at map_list instead, see below)   rewrite confidence: 0.5
// evidence: types/interface.h map_list_entry (0x00712dcc, map_id at +0x04) and widget_instance
// (first_child, next_sibling, selection_index, background_bitmap_frame); calls
// widget_list_scroll_window (0x4a7400, already established) to get the 3-wide
// previous/current/next window, then reads map_list[index].map_id (as a 16 bit value) into
// the three matching row widgets.
// UNSURE: set_profile_name (0x49c710, out of range) writes through an inherited EBX this pack
// could not resolve; the call is preserved exactly as compiled with the customary "+2" source
// (see ui_selection_list_mirror_value_build.c for why the raw 0x7ff dword copy lines up that way).
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
extern void set_profile_name(widget_instance *widget, const uint16_t *name_source); // 0x49c710, blam-cc: EBX widget
extern void widget_list_scroll_window(int32_t out[3], widget_instance *widget); // 0x4a7400

// Recomputes the 3-wide previous/current/next window of the shared map_list into this widget's
// three visible row children (their label's selection_index and the row's own
// background_bitmap_frame all get the same 16 bit map_id).
void ui_map_list_carousel_refresh_window(widget_instance *widget)
{
    uint8_t profile_record[0x1ffc];
    int32_t window[3];
    int32_t slot;

    memcpy(profile_record, &saved_player_profile_slots[0].profile, sizeof(profile_record));
    set_profile_name(widget->extended_description, (const uint16_t *)(profile_record + 2));

    widget_list_scroll_window(window, widget);

    for (slot = 0; slot < 3; slot = slot + 1) {
        widget_instance *row;
        widget_instance *label;
        widget_instance *value;
        int16_t map_id;

        if (window[slot] == -1) {
            return;
        }

        row = widget->first_child;
        if (slot > 0) {
            widget_instance *cur = row;
            int32_t depth = 0;
            row = 0;
            do {
                row = 0;
                if (cur == 0) break;
                cur = cur->next_sibling;
                depth = depth + 1;
                row = cur;
            } while (depth < slot);
        }

        label = row->first_child->next_sibling; // row.first_child.next_sibling
        value = label->next_sibling;
        map_id = (int16_t)map_list[window[slot]].map_id;
        row->first_child->selection_index = map_id;
        label->background_bitmap_frame = map_id;
        value->selection_index = map_id;
    }
}

#if 0
Original Ghidra decompilation (0x4a6940):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_004a6940(int param_1)

{
  undefined2 *puVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  int local_2014 [3];
  undefined1 local_2006 [8186];
  undefined4 uStack_c;

  uStack_c = 0x4a6950;
  puVar6 = &DAT_00712dd8;
  puVar7 = (undefined4 *)&stack0xffffdff8;
  for (iVar4 = 0x7ff; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar7 = *puVar6;
    puVar6 = puVar6 + 1;
    puVar7 = puVar7 + 1;
  }
  set_profile_name(local_2006);
  FUN_004a7400();
  iVar4 = 0;
  do {
    if (local_2014[iVar4] == -1) {
      return;
    }
    iVar2 = *(int *)(param_1 + 0x34);
    iVar5 = 0;
    iVar3 = iVar2;
    if (0 < iVar4) {
      do {
        iVar3 = 0;
        if (iVar2 == 0) break;
        iVar2 = *(int *)(iVar2 + 0x2c);
        iVar5 = iVar5 + 1;
        iVar3 = iVar2;
      } while (iVar5 < iVar4);
    }
    iVar2 = *(int *)(*(int *)(iVar3 + 0x34) + 0x2c);
    iVar5 = *(int *)(iVar2 + 0x2c);
    puVar1 = (undefined2 *)(DAT_00712dcc + 4 + local_2014[iVar4] * 0xc);
    *(undefined2 *)(*(int *)(iVar3 + 0x34) + 0x40) = *puVar1;
    *(undefined2 *)(iVar2 + 0x58) = *puVar1;
    iVar4 = iVar4 + 1;
    *(undefined2 *)(iVar5 + 0x40) = *puVar1;
    if (2 < iVar4) {
      return;
    }
  } while( true );
}
#endif
