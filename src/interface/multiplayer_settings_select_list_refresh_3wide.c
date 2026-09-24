// multiplayer_settings_select_list_refresh_3wide  (Ghidra:
// multiplayer_settings_select_list_refresh_3wide, already named)
// address 0x4a5ff0, size 263 bytes, callers=0 in this build
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: rewritten from objdump 0x4a5ff0..0x4a60f7 in the phase-4 review. The first
// rewrite called widget_list_scroll_window and ui_variant_carousel_slot_cache_populate
// without their register arguments and read uninitialised ids.
//   The three visible list positions come from widget_list_scroll_window (EAX out[3], ECX
// widget); each maps to its variant id through the list items (-1 stays -1), and the ids go
// to ui_variant_carousel_slot_cache_populate (EBX ids, stack 3). For each visible position
// up to the first -1, the matching child row (walked by next_sibling, stopping at NULL) gets
// multiplayer_settings_select_list_update_item with the body (+4) of the carousel slot
// holding that id; an id with no slot is skipped. With an extended description, the
// profile globals block (0x00712dd8, 0x7ff dwords) is copied to the stack and its name (+2)
// shown there (set_profile_name, EBX widget).
// register convention: cdecl, the one stack parameter (widget).

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern variant_carousel_slot variant_carousel_slots[3]; // 0x00879d60
extern uint8_t profile_globals_block[0x60a4];           // 0x00712dd8

extern void widget_list_scroll_window(int32_t out[3], widget_instance *widget); // 0x4a7400, blam-cc: EAX out, ECX widget
extern void ui_variant_carousel_slot_cache_populate(int32_t *candidate_ids, int32_t count); // 0x4a7570, blam-cc: EBX candidate_ids
extern void multiplayer_settings_select_list_update_item(widget_instance *widget, const uint16_t *record); // 0x4a5bc0
extern void set_profile_name(widget_instance *widget, const uint16_t *name_source); // 0x49c710, blam-cc: EBX widget

void multiplayer_settings_select_list_refresh_3wide(widget_instance *widget)
{
    int32_t window[3];
    int32_t ids[3];
    int32_t i;

    widget_list_scroll_window(window, widget);
    for (i = 0; i < 3; i++) {
        ids[i] = window[i] == -1 ? -1 : ((int32_t *)widget->list_items)[window[i]];
    }
    ui_variant_carousel_slot_cache_populate(ids, 3);

    for (i = 0; i < 3 && window[i] != -1; i++) {
        widget_instance *row = widget->first_child;
        int32_t id = ((int32_t *)widget->list_items)[window[i]];
        int32_t depth;
        int32_t slot;

        for (depth = 0; depth < i && row != 0; depth++) {
            row = row->next_sibling;
        }
        for (slot = 0; slot < 3; slot++) {
            if (variant_carousel_slots[slot].id == id) {
                multiplayer_settings_select_list_update_item(row, (const uint16_t *)variant_carousel_slots[slot].unknown);
                break;
            }
        }
    }

    if (widget->extended_description != 0) {
        uint8_t profile_copy[0x7ff * 4];

        memcpy(profile_copy, profile_globals_block, sizeof(profile_copy));
        set_profile_name(widget->extended_description, (const uint16_t *)(profile_copy + 2));
    }
}

#if 0
Original Ghidra decompilation (0x4a5ff0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void multiplayer_settings_select_list_refresh_3wide(int param_1)

{
  int iVar1;
  int *piVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  int local_2014 [3];
  undefined1 local_2006 [8186];
  undefined4 uStack_c;

  uStack_c = 0x4a6000;
  FUN_004a7400();
  FUN_004a7570(3);
  iVar5 = 0;
  do {
    if (local_2014[iVar5] == -1) break;
    iVar3 = *(int *)(param_1 + 0x34);
    iVar1 = 0;
    iVar4 = iVar3;
    if (0 < iVar5) {
      do {
        iVar4 = 0;
        if (iVar3 == 0) break;
        iVar3 = *(int *)(iVar3 + 0x2c);
        iVar1 = iVar1 + 1;
        iVar4 = iVar3;
      } while (iVar1 < iVar5);
    }
    iVar3 = 0;
    piVar2 = &DAT_00879d60;
    do {
      if (*piVar2 == *(int *)(*(int *)(param_1 + 0x44) + local_2014[iVar5] * 4)) {
        multiplayer_settings_select_list_update_item(iVar4,&DAT_00879d64 + iVar3 * 0x27);
        break;
      }
      piVar2 = piVar2 + 0x27;
      iVar3 = iVar3 + 1;
    } while ((int)piVar2 < 0x879f34);
    iVar5 = iVar5 + 1;
  } while (iVar5 < 3);
  if (*(int *)(param_1 + 0x4c) != 0) {
    puVar6 = &DAT_00712dd8;
    puVar7 = (undefined4 *)&stack0xffffdff8;
    for (iVar5 = 0x7ff; iVar5 != 0; iVar5 = iVar5 + -1) {
      *puVar7 = *puVar6;
      puVar6 = puVar6 + 1;
      puVar7 = puVar7 + 1;
    }
    set_profile_name(local_2006);
  }
  return;
}
#endif
