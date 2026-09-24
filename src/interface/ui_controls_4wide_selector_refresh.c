// ui_controls_4wide_selector_refresh  (Ghidra: FUN_004a4f60, renamed)
// renamed from FUN_004a4f60 in the naming pass
// address 0x4a4f60, size 126 bytes, callers=0 in this build
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: functions.md: "Refreshes a 4-item profile/slot selector, highlighting the currently
// selected entry and syncing its paired display." Same set_profile_name usage pattern as the
// sibling refresh functions this session.
// register convention: cdecl, the one recognized stack parameter (widget).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>

extern uint8_t profile_globals_block[0x60a4]; // 0x00712dd8
extern void set_profile_name(widget_instance *widget, const uint16_t *name_source); // 0x49c710

void ui_controls_4wide_selector_refresh(widget_instance *widget)
{
    int16_t selection = *(int16_t *)((uint8_t *)widget + 0x3c); // UNSURE: raw offset, see the
                                                                  // level-select files' identical note
    widget_instance *display = widget->extended_description->first_child;
    widget_instance *cursor;
    int16_t i;

    if (selection != -1) {
        display->background_bitmap_frame = selection;
        display->next_sibling->selection_index = selection;
    }

    {
        uint8_t profile_copy[0x2000];

        memcpy(profile_copy, profile_globals_block, sizeof(profile_copy));
        set_profile_name(widget, (const uint16_t *)(profile_copy + 2));
    }

    cursor = widget->first_child;
    for (i = 0; i < 4; i++) {
        cursor->background_bitmap_frame = (i == selection);
        cursor = cursor->next_sibling;
    }
}

#if 0
Original Ghidra decompilation (0x4a4f60):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_004a4f60(int param_1)

{
  short sVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  bool bVar5;
  undefined4 local_2008;
  undefined4 uStack_c;

  uStack_c = 0x4a4f70;
  sVar1 = *(short *)(param_1 + 0x3c);
  iVar2 = *(int *)(*(int *)(param_1 + 0x4c) + 0x34);
  if (sVar1 != -1) {
    *(short *)(iVar2 + 0x58) = sVar1;
    *(short *)(*(int *)(iVar2 + 0x2c) + 0x40) = sVar1;
  }
  puVar3 = &DAT_00712dd8;
  puVar4 = &local_2008;
  for (iVar2 = 0x7ff; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar4 = *puVar3;
    puVar3 = puVar3 + 1;
    puVar4 = puVar4 + 1;
  }
  set_profile_name((int)&local_2008 + 2);
  iVar2 = *(int *)(param_1 + 0x34);
  sVar1 = 0;
  do {
    bVar5 = sVar1 == *(short *)(param_1 + 0x3c);
    sVar1 = sVar1 + 1;
    *(ushort *)(iVar2 + 0x58) = (ushort)bVar5;
    iVar2 = *(int *)(iVar2 + 0x2c);
  } while (sVar1 < 4);
  return;
}
#endif
