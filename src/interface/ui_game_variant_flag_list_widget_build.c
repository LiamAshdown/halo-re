// ui_game_variant_flag_list_widget_build  (Ghidra: FUN_004a8560, renamed)
// address 0x4a8560, size 140 bytes
// name confidence: 0.3 (chosen)   rewrite confidence: 0.4
// evidence: types/interface.h widget_instance, ui_list_item (data at +0x04) and ui_lists /
// ui_list_current (established by ui_list_widget_rebuild_rows.c).
// UNSURE: the ui_list_item::data pointer is passed to multiplayer_settings_select_list_update_item
// (declared elsewhere as taking a wchar_t*) but is ALSO dereferenced here as a struct with a
// flag byte at +0x94, so it is really a game_variant-shaped record, not a string; kept as
// void* rather than guessing a types/game.h struct that was not confirmed in this pass.
// register convention: widget as the recognized parameter (param_1).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "objects.h"
#include "units.h"
#include "fn_interface.h"

extern int32_t ui_list_current;      // 0x00692c04
extern growable_array ui_lists[3];   // 0x006b3830, element size 0x10 (ui_list_item)


extern void multiplayer_settings_select_list_update_item(widget_instance *description_widget, void *variant_data); // 0x4a5bc0, UNSURE signature, see header note

// Rebuilds this widget's rows, refreshes the linked game-variant description widget, then
// walks to the row 11 slots down (a fixed layout row) and hides/dims it when the selected
// variant's flag byte (+0x94, bit 0) is set.
void ui_game_variant_flag_list_widget_build(widget_instance *widget)
{
    int16_t combo_index;
    void *variant_data = 0;
    widget_instance *row;
    widget_instance *target;
    int32_t depth;

    ui_list_widget_rebuild_rows(widget, (void *)ui_list_default_item_format);

    combo_index = *(int16_t *)&((struct widget_instance *)widget)->text; // UNSURE offset, see ui_list_widget_rebuild_rows.c
    if (combo_index > -1 && combo_index < ui_lists[ui_list_current].count) {
        ui_list_item *entry = (ui_list_item *)ui_lists[ui_list_current].data + combo_index;
        variant_data = entry->data;
    }
    multiplayer_settings_select_list_update_item(widget->extended_description, variant_data);

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
    if ((*((const uint8_t *)variant_data + 0x94) & 1) != 0) { // no NULL guard in the original
        target->hidden = 1;
        target->scale = 0.333f;
    } else {
        target->hidden = 0;
        target->scale = 1.0f;
    }
}

#if 0
Original Ghidra decompilation (0x4a8560):

void FUN_004a8560(int param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;

  FUN_004a7db0(param_1,&LAB_004a8310);
  iVar1 = (int)*(short *)(param_1 + 0x3c);
  iVar4 = 0;
  if ((-1 < iVar1) && (iVar1 < (int)(&DAT_006b3834)[DAT_00692c04 * 3])) {
    iVar4 = *(int *)((&DAT_006b3838)[DAT_00692c04 * 3] + 4 + iVar1 * 0x10);
  }
  multiplayer_settings_select_list_update_item
            (*(undefined4 *)(*(int *)(param_1 + 0x4c) + 0x34),iVar4);
  iVar1 = *(int *)(param_1 + 0x34);
  iVar3 = 0;
  do {
    iVar2 = 0;
    if (iVar1 == 0) break;
    iVar1 = *(int *)(iVar1 + 0x2c);
    iVar3 = iVar3 + 1;
    iVar2 = iVar1;
  } while (iVar3 < 0xb);
  iVar1 = *(int *)(*(int *)(iVar2 + 0x34) + 0x2c);
  if ((*(byte *)(iVar4 + 0x94) & 1) != 0) {
    *(undefined1 *)(iVar1 + 0x12) = 1;
    *(undefined4 *)(iVar1 + 0x24) = 0x3eaa7efa;
    return;
  }
  *(undefined1 *)(iVar1 + 0x12) = 0;
  *(undefined4 *)(iVar1 + 0x24) = 0x3f800000;
  return;
}
#endif
