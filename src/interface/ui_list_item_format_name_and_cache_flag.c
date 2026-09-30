// ui_list_item_format_name_and_cache_flag  (Ghidra: FUN_004a83d0, renamed)
// address 0x4a83d0, size 111 bytes
// name confidence: 0.35 (chosen)   rewrite confidence: 0.5
// evidence: types/interface.h ui_list_item (name at +0x00) and ui_lists / ui_list_current
// (established by ui_list_widget_rebuild_rows.c); map_list_entry (cache_file_exists at +0x08)
// and map_list / map_list_count.
// UNSURE: this is a ui_list_widget_rebuild_rows item-format callback (called with a third
// list_items argument this function's 2-parameter signature simply ignores, legal under cdecl),
// yet it formats the name from the CURRENT ui_lists group while reporting success from the
// UNRELATED map_list's cache_file_exists at the same index -- kept exactly as decompiled; not
// independently explained.
// register convention: Ghidra recognized both parameters directly; no unresolved registers.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"
#include <wchar.h>

extern int32_t ui_list_current;      // 0x00692c04
extern growable_array ui_lists[3];   // 0x006b3830, element size 0x10 (ui_list_item)
extern map_list_entry *map_list;     // 0x00712dcc
extern int32_t map_list_count;       // 0x00712dd0

// Formats item_index's display name out of the current ui_lists group into a 64 wide-char
// buffer, and reports map_list[item_index]'s cache_file_exists byte as the success/validity
// result (see the header note -- the two lists are not obviously related).
uint8_t ui_list_item_format_name_and_cache_flag(uint16_t *out_name, int32_t item_index)
{
    const uint16_t *source = (const uint16_t *)L"";
    uint8_t result;

    if (item_index > -1 && item_index < ui_lists[ui_list_current].count) {
        ui_list_item *entry = (ui_list_item *)ui_lists[ui_list_current].data + item_index;
        source = entry->name;
    }
    wcsncpy((wchar_t *)out_name, (const wchar_t *)source, 0x3f);
    out_name[0x3f] = 0;

    result = 0;
    if (out_name[0] != 0 && item_index > -1 && item_index < map_list_count) {
        result = map_list[item_index].cache_file_exists;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4a83d0):

undefined1 FUN_004a83d0(wchar_t *param_1,int param_2)

{
  undefined1 uVar1;
  wchar_t *_Source;

  _Source = L"";
  if ((-1 < param_2) && (param_2 < (int)(&DAT_006b3834)[DAT_00692c04 * 3])) {
    _Source = *(wchar_t **)(param_2 * 0x10 + (&DAT_006b3838)[DAT_00692c04 * 3]);
  }
  _wcsncpy(param_1,_Source,0x3f);
  param_1[0x3f] = L'\0';
  uVar1 = 0;
  if (((*param_1 != L'\0') && (uVar1 = 0, -1 < param_2)) && (param_2 < DAT_00712dd0)) {
    uVar1 = *(undefined1 *)(DAT_00712dcc + 8 + param_2 * 0xc);
  }
  return uVar1;
}
#endif
