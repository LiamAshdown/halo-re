// ui_level_carousel_refresh  (Ghidra: FUN_004a4ee0, renamed)
// renamed from FUN_004a4ee0 in the naming pass
// address 0x4a4ee0, size 115 bytes, callers=0 in this build
// name confidence: 0.3   rewrite confidence: 0.75
// evidence: rewritten in the phase-4 review from objdump -d 0x4a4ee0..0x4a4f52. It copies saved
// profile slot 0 (0x1ffc bytes from 0x00712dd8) to the stack and shows its name through
// set_profile_name (EBX widget->extended_description, stack name at record + 2), asks
// widget_list_scroll_window @0x4a7400 (EAX out, ECX widget) for the three visible item indices,
// and hands each index that is not -1, with the matching row (first_child advanced i times), to
// ui_level_carousel_row_refresh (ECX row, EAX level index), which fills one level_select_entry row.
// The earlier rewrite overlapped the index array with the profile copy, passed the widget
// instead of its extended description, dropped both register arguments of 0x4a7400 and handed
// ui_level_carousel_row_refresh the loop counter instead of the index.
// register convention: cdecl, the one stack parameter (widget).
// reconciled: R56 0x00712dd8 uint8_t saved_profile_records[3][0x2004] -> saved_games.h saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles] (one 0x2004-byte slot; a second would overlap 0x00714dde); same bytes copied/read

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include <string.h>

extern saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles]; // 0x00712dd8, saved_games.h

extern void set_profile_name(widget_instance *widget, const uint16_t *name_source); // 0x49c710, blam-cc: EBX widget
extern void widget_list_scroll_window(int32_t out[3], widget_instance *widget); // 0x4a7400, blam-cc: EAX out, ECX widget
extern void ui_level_carousel_row_refresh(widget_instance *widget, int32_t slot_index); // 0x4a4e20, blam-cc: ECX widget, EAX index

void ui_level_carousel_refresh(widget_instance *widget)
{
    int32_t visible[3];
    uint8_t profile_record[0x1ffc];
    int32_t i;

    memcpy(profile_record, &profile_globals_block[0].profile, sizeof(profile_record));
    set_profile_name(widget->extended_description, (const uint16_t *)(profile_record + 2));
    widget_list_scroll_window(visible, widget);

    for (i = 0; i < 3; i++) {
        widget_instance *row;
        int32_t depth;

        if (visible[i] == -1) {
            return;
        }
        row = widget->first_child;
        for (depth = 0; depth < i && row != (widget_instance *)0; depth++) {
            row = row->next_sibling;
        }
        ui_level_carousel_row_refresh(row, visible[i]);
    }
}

#if 0
Original Ghidra decompilation (0x4a4ee0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_004a4ee0(int param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  int local_2014 [3];
  undefined1 local_2006 [8186];
  undefined4 uStack_c;

  uStack_c = 0x4a4ef0;
  puVar4 = &DAT_00712dd8;
  puVar5 = (undefined4 *)&stack0xffffdff8;
  for (iVar1 = 0x7ff; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar5 = *puVar4;
    puVar4 = puVar4 + 1;
    puVar5 = puVar5 + 1;
  }
  set_profile_name(local_2006);
  FUN_004a7400();
  iVar1 = 0;
  do {
    if (local_2014[iVar1] == -1) {
      return;
    }
    iVar2 = *(int *)(param_1 + 0x34);
    iVar3 = 0;
    if (0 < iVar1) {
      do {
        if (iVar2 == 0) break;
        iVar2 = *(int *)(iVar2 + 0x2c);
        iVar3 = iVar3 + 1;
      } while (iVar3 < iVar1);
    }
    FUN_004a4e20();
    iVar1 = iVar1 + 1;
    if (2 < iVar1) {
      return;
    }
  } while( true );
}
#endif
