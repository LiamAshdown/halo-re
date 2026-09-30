// ui_tab_group_sync_grouped  (Ghidra: FUN_004a4d30, renamed)
// renamed from FUN_004a4d30 in the naming pass
// address 0x4a4d30, size 204 bytes, callers=0 in this build
// name confidence: 0.3   rewrite confidence: 0.2
// evidence: functions.md: "Synchronizes a tab/list group's selected index into a target widget
// using special-cased index groupings." Same sibling-index walk as FUN_004a4cb0/FUN_004a4cf0, but
// groups the focused child's index (1..3 -> one display value, 5..6 -> another, 7 -> a third, 0/4
// falling through) before writing it, and separately hides/shows the first and fifth children.
// register convention: cdecl, the one recognized stack parameter (widget).
// UNSURE: set_profile_name's implicit widget/EBX argument, same as the sibling refresh functions.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"
#include <string.h>

extern uint8_t profile_globals_block[0x60a4]; // 0x00712dd8


void ui_tab_group_sync_grouped(widget_instance *widget)
{
    widget_instance *child = widget->first_child;
    int16_t index = 0;
    widget_instance *display;
    int16_t group_offset;
    widget_instance *cursor;
    int16_t position;

    if (child != (widget_instance *)0) {
        while (child != widget->focused_child) {
            child = child->next_sibling;
            index = index + 1;
            if (child == (widget_instance *)0) {
                break;
            }
        }
    }

    display = widget->extended_description->first_child;
    switch (index) {
    case 1:
    case 2:
    case 3:
        display->background_bitmap_frame = 0;
        group_offset = -1;
        goto apply;
    case 5:
    case 6:
        display->background_bitmap_frame = 0;
        group_offset = -2;
        break;
    case 7:
        display->background_bitmap_frame = 2;
        group_offset = -2;
        break;
    default:
        goto sync_visibility;
    }
apply:
    if ((int16_t)(index + group_offset) != -1) {
        display->next_sibling->selection_index = index + group_offset;
    }

sync_visibility:
    position = 0;
    for (cursor = widget->first_child; cursor != (widget_instance *)0; cursor = cursor->next_sibling) {
        if (position == 0 || position == 4) {
            cursor->hidden = 1;
        }
        position = position + 1;
    }

    {
        uint8_t profile_copy[0x2000];

        memcpy(profile_copy, profile_globals_block, sizeof(profile_copy));
        set_profile_name(widget, (const uint16_t *)(profile_copy + 2));
    }
}

#if 0
Original Ghidra decompilation (0x4a4d30):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_004a4d30(int param_1)

{
  short sVar1;
  short sVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 *puVar5;
  undefined4 local_2008;
  undefined4 uStack_c;

  uStack_c = 0x4a4d40;
  iVar3 = *(int *)(param_1 + 0x34);
  sVar2 = 0;
  if (iVar3 != 0) {
    sVar2 = 0;
    do {
      if (iVar3 == *(int *)(param_1 + 0x38)) break;
      iVar3 = *(int *)(iVar3 + 0x2c);
      sVar2 = sVar2 + 1;
    } while (iVar3 != 0);
  }
  switch(sVar2) {
  case 1:
  case 2:
  case 3:
    *(undefined2 *)(*(int *)(*(int *)(param_1 + 0x4c) + 0x34) + 0x58) = 0;
    sVar1 = -1;
    goto LAB_004a4d98;
  default:
    goto switchD_004a4d69_caseD_4;
  case 5:
  case 6:
    *(undefined2 *)(*(int *)(*(int *)(param_1 + 0x4c) + 0x34) + 0x58) = 0;
    break;
  case 7:
    *(undefined2 *)(*(int *)(*(int *)(param_1 + 0x4c) + 0x34) + 0x58) = 2;
  }
  sVar1 = -2;
LAB_004a4d98:
  if ((short)(sVar2 + sVar1) != -1) {
    *(short *)(*(int *)(*(int *)(*(int *)(param_1 + 0x4c) + 0x34) + 0x2c) + 0x40) = sVar2 + sVar1;
  }
switchD_004a4d69_caseD_4:
  sVar2 = 0;
  for (iVar3 = *(int *)(param_1 + 0x34); iVar3 != 0; iVar3 = *(int *)(iVar3 + 0x2c)) {
    if ((sVar2 == 0) || (sVar2 == 4)) {
      *(undefined1 *)(iVar3 + 0x12) = 1;
    }
    sVar2 = sVar2 + 1;
  }
  puVar4 = &DAT_00712dd8;
  puVar5 = &local_2008;
  for (iVar3 = 0x7ff; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar5 = *puVar4;
    puVar4 = puVar4 + 1;
    puVar5 = puVar5 + 1;
  }
  set_profile_name((int)&local_2008 + 2);
  return;
}
#endif
