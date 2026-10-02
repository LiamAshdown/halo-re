// ui_tab_group_sync_5wide  (Ghidra: FUN_004a4cf0, renamed)
// renamed from FUN_004a4cf0 in the naming pass
// address 0x4a4cf0, size 60 bytes, callers=0 in this build
// name confidence: 0.3   rewrite confidence: 0.5
// evidence: functions.md: "Synchronizes a 5-option tab/list group's selected index into a linked
// display widget." Byte-for-byte FUN_004a4cb0.c with the bound lowered from 6 to 4 (7 options to
// 5).
// register convention: cdecl, the one recognized stack parameter (widget).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void ui_tab_group_sync_5wide(widget_instance *widget)
{
    widget_instance *child = widget->first_child;
    int16_t index = 0;
    widget_instance *target;

    if (child != (widget_instance *)0) {
        while (child != widget->focused_child) {
            child = child->next_sibling;
            index = index + 1;
            if (child == (widget_instance *)0) {
                break;
            }
        }
        if (index > 4 || index == -1) {
            return;
        }
    }
    target = widget->extended_description->first_child;
    target->selection_index = index;
    target->next_sibling->background_bitmap_frame = index;
}

#if 0
Original Ghidra decompilation (0x4a4cf0):

void FUN_004a4cf0(int param_1)

{
  int iVar1;
  short sVar2;

  iVar1 = *(int *)(param_1 + 0x34);
  sVar2 = 0;
  if (iVar1 != 0) {
    do {
      if (iVar1 == *(int *)(param_1 + 0x38)) break;
      iVar1 = *(int *)(iVar1 + 0x2c);
      sVar2 = sVar2 + 1;
    } while (iVar1 != 0);
    if (4 < sVar2) {
      return;
    }
    if (sVar2 == -1) {
      return;
    }
  }
  iVar1 = *(int *)(*(int *)(param_1 + 0x4c) + 0x34);
  *(short *)(iVar1 + 0x40) = sVar2;
  *(short *)(*(int *)(iVar1 + 0x2c) + 0x58) = sVar2;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
