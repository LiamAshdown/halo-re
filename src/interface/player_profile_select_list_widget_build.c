// player_profile_select_list_widget_build  (Ghidra: player_profile_select_list_widget_build,
// already named)
// address 0x4a85f0, size 307 bytes
// name confidence: 0.55 (existing Ghidra name)   rewrite confidence: 0.55
// evidence: types/interface.h widget_instance (state, background_bitmap_frame, hidden, scale,
// item_count, first_child, next_sibling, extended_description all match); ui_lists /
// ui_list_current and ui_list_item::data (established by ui_list_widget_rebuild_rows.c and
// ui_game_variant_list_widget_build.c).
// UNSURE: player_profile_details_widget_refresh (0x4a6100, out of this module's range) is
// declared taking the ui_list_item::data pointer verbatim; its own signature was not confirmed
// in this pass. UNSURE: set_profile_name writes through an inherited EBX this pack could not
// resolve; call preserved exactly as compiled.
// register convention: widget as the recognized parameter (param_1).
// reconciled: R56 0x00712dd8 uint8_t saved_profile_records[3][0x2004] -> saved_games.h saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles] (one 0x2004-byte slot; a second would overlap 0x00714dde); same bytes copied/read

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include <string.h>
#include "objects.h"
#include "units.h"
#include "fn_interface.h"

extern saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles]; // 0x00712dd8, saved_games.h
extern int32_t ui_list_current;      // 0x00692c04
extern growable_array ui_lists[3];   // 0x006b3830, element size 0x10 (ui_list_item)


// Rebuilds this widget's rows and refreshes the profile-name label, then either blanks the
// profile-details sub-tree (when the selected combo index is out of range) or refreshes it
// from the selected ui_list entry's data pointer, finally showing/hiding a fixed row 11 slots
// down depending on whether the list is empty.
void player_profile_select_list_widget_build(widget_instance *widget)
{
    uint8_t profile_record[0x1ffc];
    int32_t combo_index;
    widget_instance *row;
    widget_instance *target;
    int32_t depth;

    ui_list_widget_rebuild_rows(widget, (void *)ui_list_default_item_format);

    memcpy(profile_record, &profile_globals_block[0].profile, sizeof(profile_record));
    set_profile_name(widget->extended_description->first_child, (const uint16_t *)(profile_record + 2));

    combo_index = *(int16_t *)&((struct widget_instance *)widget)->text; // list selection, see types/interface.h
    if (combo_index < 0 || (uint16_t)widget->item_count <= combo_index) {
        widget_instance *a = widget->extended_description->first_child->next_sibling->first_child;
        widget_instance *b = a->next_sibling;
        widget_instance *c = b->next_sibling->first_child;
        widget_instance *d1 = c->next_sibling;
        widget_instance *d2 = d1->next_sibling;
        widget_instance *d3 = d2->next_sibling;
        widget_instance *d4 = d3->next_sibling;
        widget_instance *d5 = d4->next_sibling;
        widget_instance *d6 = d5->next_sibling;

        a->state = 0;
        b->background_bitmap_frame = 0x12;
        c->state = 1;
        d1->state = 0;
        d2->state = 0;
        d3->state = 0;
        d4->state = 0;
        d5->state = 0;
        d6->state = 0;
    } else {
        void *item_data = 0;
        if (combo_index < ui_lists[ui_list_current].count) {
            ui_list_item *entry = (ui_list_item *)ui_lists[ui_list_current].data + combo_index;
            item_data = entry->data;
        }
        player_profile_details_widget_refresh(widget->extended_description->first_child->next_sibling,
                                              (const uint8_t *)item_data); // EAX loaded at 0x4a863b
    }

    row = widget->first_child;
    depth = 0;
    {
        widget_instance *cur = row;
        row = 0;
        do {
            row = 0;
            if (cur == 0) break;
            cur = cur->next_sibling;
            depth = depth + 1;
            row = cur;
        } while (depth < 0x0b);
    }

    target = row->first_child->next_sibling;
    if (widget->item_count == 0) {
        target->hidden = 1;
        target->scale = 0.333f;
    } else {
        target->hidden = 0;
        target->scale = 1.0f;
    }
}

#if 0
Original Ghidra decompilation (0x4a85f0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void player_profile_select_list_widget_build(int param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  undefined4 *puVar10;
  undefined4 *puVar11;
  undefined4 uVar12;
  undefined4 local_2008;
  undefined4 uStack_c;

  uStack_c = 0x4a8600;
  FUN_004a7db0(param_1,&LAB_004a8310);
  puVar10 = &DAT_00712dd8;
  puVar11 = &local_2008;
  for (iVar8 = 0x7ff; iVar8 != 0; iVar8 = iVar8 + -1) {
    *puVar11 = *puVar10;
    puVar10 = puVar10 + 1;
    puVar11 = puVar11 + 1;
  }
  set_profile_name((int)&local_2008 + 2);
  iVar8 = (int)*(short *)(param_1 + 0x3c);
  if ((iVar8 < 0) || ((int)(uint)*(ushort *)(param_1 + 0x48) <= iVar8)) {
    iVar8 = *(int *)(*(int *)(*(int *)(*(int *)(param_1 + 0x4c) + 0x34) + 0x2c) + 0x34);
    iVar9 = *(int *)(iVar8 + 0x2c);
    iVar7 = *(int *)(*(int *)(iVar9 + 0x2c) + 0x34);
    iVar1 = *(int *)(iVar7 + 0x2c);
    iVar2 = *(int *)(iVar1 + 0x2c);
    iVar3 = *(int *)(iVar2 + 0x2c);
    iVar4 = *(int *)(iVar3 + 0x2c);
    iVar5 = *(int *)(iVar4 + 0x2c);
    iVar6 = *(int *)(iVar5 + 0x2c);
    *(undefined1 *)(iVar8 + 0x10) = 0;
    *(undefined2 *)(iVar9 + 0x58) = 0x12;
    *(undefined1 *)(iVar7 + 0x10) = 1;
    *(undefined1 *)(iVar1 + 0x10) = 0;
    *(undefined1 *)(iVar2 + 0x10) = 0;
    *(undefined1 *)(iVar3 + 0x10) = 0;
    *(undefined1 *)(iVar4 + 0x10) = 0;
    *(undefined1 *)(iVar5 + 0x10) = 0;
    *(undefined1 *)(iVar6 + 0x10) = 0;
  }
  else {
    uVar12 = 0;
    if (iVar8 < (int)(&DAT_006b3834)[DAT_00692c04 * 3]) {
      uVar12 = *(undefined4 *)((&DAT_006b3838)[DAT_00692c04 * 3] + 4 + iVar8 * 0x10);
    }
    player_profile_details_widget_refresh(uVar12);
  }
  iVar8 = *(int *)(param_1 + 0x34);
  iVar9 = 0;
  do {
    iVar7 = 0;
    if (iVar8 == 0) break;
    iVar8 = *(int *)(iVar8 + 0x2c);
    iVar9 = iVar9 + 1;
    iVar7 = iVar8;
  } while (iVar9 < 0xb);
  iVar8 = *(int *)(*(int *)(iVar7 + 0x34) + 0x2c);
  if (*(short *)(param_1 + 0x48) == 0) {
    *(undefined1 *)(iVar8 + 0x12) = 1;
    *(undefined4 *)(iVar8 + 0x24) = 0x3eaa7efa;
    return;
  }
  *(undefined1 *)(iVar8 + 0x12) = 0;
  *(undefined4 *)(iVar8 + 0x24) = 0x3f800000;
  return;
}
#endif
