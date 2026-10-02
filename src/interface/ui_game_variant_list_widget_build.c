// ui_game_variant_list_widget_build  (Ghidra: FUN_004a84d0, renamed)
// address 0x4a84d0, size 140 bytes
// name confidence: 0.3 (chosen)   rewrite confidence: 0.55
// evidence: types/interface.h widget_instance, ui_list_item (data at +0x04) and ui_lists /
// ui_list_current (established by ui_list_widget_rebuild_rows.c); the callee
// multiplayer_settings_select_list_update_item's own second parameter is a wchar_t*, matching
// ui_list_item::data holding a game-variant description string here.
// UNSURE: set_profile_name writes through an inherited EBX this pack could not resolve; call
// preserved exactly as compiled.
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
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles]; // 0x00712dd8, saved_games.h
extern int32_t ui_list_current;      // 0x00692c04
extern growable_array ui_lists[3];   // 0x006b3830, element size 0x10 (ui_list_item)
extern void ui_list_widget_rebuild_rows(widget_instance *widget, ui_list_item_format_function format_item); // 0x4a7db0
extern uint8_t ui_list_default_item_format(void *item_buffer, int32_t item_index, void *list_items); // 0x4a8310, UNSURE: not analyzed separately
extern void set_profile_name(widget_instance *widget, const uint16_t *name_source); // 0x49c710, blam-cc: EBX widget
extern void multiplayer_settings_select_list_update_item(widget_instance *description_widget, const uint16_t *variant_description); // 0x4a5bc0

// Rebuilds this widget's rows, then refreshes the linked game-variant description widget from
// the currently selected ui_list entry's data pointer.
void ui_game_variant_list_widget_build(widget_instance *widget)
{
    uint8_t profile_record[0x1ffc];
    int16_t combo_index;
    const uint16_t *variant_description = 0;

    ui_list_widget_rebuild_rows(widget, (ui_list_item_format_function)((void *)ui_list_default_item_format));

    memcpy(profile_record, &profile_globals_block[0].profile, sizeof(profile_record));
    set_profile_name(widget->extended_description->first_child, (const uint16_t *)(profile_record + 2));

    combo_index = *(int16_t *)&((struct widget_instance *)widget)->text; // UNSURE offset, see ui_list_widget_rebuild_rows.c
    if (combo_index > -1 && combo_index < ui_lists[ui_list_current].count) {
        ui_list_item *entry = (ui_list_item *)ui_lists[ui_list_current].data + combo_index;
        variant_description = (const uint16_t *)entry->data;
    }

    multiplayer_settings_select_list_update_item(
        widget->extended_description->first_child->next_sibling, variant_description);
}

#if 0
Original Ghidra decompilation (0x4a84d0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_004a84d0(int param_1)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  undefined4 local_2008;
  undefined4 uStack_c;

  uStack_c = 0x4a84e0;
  FUN_004a7db0(param_1,&LAB_004a8310);
  puVar3 = &DAT_00712dd8;
  puVar4 = &local_2008;
  for (iVar1 = 0x7ff; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar4 = *puVar3;
    puVar3 = puVar3 + 1;
    puVar4 = puVar4 + 1;
  }
  set_profile_name((int)&local_2008 + 2);
  iVar1 = (int)*(short *)(param_1 + 0x3c);
  uVar2 = 0;
  if ((-1 < iVar1) && (iVar1 < (int)(&DAT_006b3834)[DAT_00692c04 * 3])) {
    uVar2 = *(undefined4 *)((&DAT_006b3838)[DAT_00692c04 * 3] + 4 + iVar1 * 0x10);
  }
  multiplayer_settings_select_list_update_item
            (*(undefined4 *)(*(int *)(*(int *)(param_1 + 0x4c) + 0x34) + 0x2c),uVar2);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
