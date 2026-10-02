// ui_tab_group_sync_9wide  (Ghidra: FUN_004a62d0, renamed)
// renamed from FUN_004a62d0 in the naming pass
// address 0x4a62d0, size 143 bytes, callers=0 in this build
// name confidence: 0.3   rewrite confidence: 0.3
// evidence: functions.md: "Synchronizes a 9-option tab group's selected index, disabling specific
// options based on network-availability flags." Same sibling-index walk as FUN_004a4cb0.c/
// FUN_004a4cf0.c (bound 8, i.e. 9 options), followed by hiding option 0 when not split-screen and
// option 2 when no network adapters are present. types/interface.h names 0x00718fc9 ui_split_screen.
// register convention: cdecl, the one recognized stack parameter (widget).
// reconciled: R78 0x006b1844 input_gamepad_count(_dword) -> input.h int32_t input_device_count; the WORD readers keep their int16 width through an (int16_t) cast

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t ui_split_screen;                 // 0x00718fc9
extern int32_t input_device_count; // 0x006b1844, input.h (0..8 connected input devices)

void ui_tab_group_sync_9wide(widget_instance *widget)
{
    widget_instance *child = widget->first_child;
    int16_t index = 0;
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
        if (index > 8 || index == -1) {
            goto sync_visibility;
        }
    }
    {
        widget_instance *display = widget->extended_description->first_child;

        display->selection_index = index;
        display->next_sibling->background_bitmap_frame = (index != 8) ? index : 6;
    }

sync_visibility:
    position = 0;
    for (cursor = widget->first_child; cursor != (widget_instance *)0; cursor = cursor->next_sibling) {
        if (position == 0) {
            if (ui_split_screen == 0) {
                cursor->hidden = 1;
                cursor->scale = 0.333f;
                if (widget->focused_child == cursor) {
                    widget->focused_child = cursor->next_sibling;
                }
            } else {
                cursor->hidden = 0;
                cursor->scale = 1.0f;
            }
        } else if (position == 2) {
            if ((int16_t)input_device_count != 0) {
                cursor->hidden = 0;
                cursor->scale = 1.0f;
            } else {
                cursor->hidden = 1;
                cursor->scale = 0.333f;
            }
        }
        position = position + 1;
    }
}

#if 0
Original Ghidra decompilation (0x4a62d0):

void FUN_004a62d0(int param_1)

{
  short sVar1;
  int iVar2;
  short sVar3;

  iVar2 = *(int *)(param_1 + 0x34);
  sVar3 = 0;
  if (iVar2 != 0) {
    do {
      if (iVar2 == *(int *)(param_1 + 0x38)) break;
      iVar2 = *(int *)(iVar2 + 0x2c);
      sVar3 = sVar3 + 1;
    } while (iVar2 != 0);
    if ((8 < sVar3) || (sVar3 == -1)) goto LAB_004a6318;
  }
  iVar2 = *(int *)(*(int *)(param_1 + 0x4c) + 0x34);
  *(short *)(iVar2 + 0x40) = sVar3;
  sVar1 = 6;
  if (sVar3 != 8) {
    sVar1 = sVar3;
  }
  *(short *)(*(int *)(iVar2 + 0x2c) + 0x58) = sVar1;
LAB_004a6318:
  iVar2 = *(int *)(param_1 + 0x34);
  sVar3 = 0;
  do {
    if (iVar2 == 0) {
      return;
    }
    if (sVar3 == 0) {
      if (DAT_00718fc9 == '\0') {
        *(undefined1 *)(iVar2 + 0x12) = 1;
        *(undefined4 *)(iVar2 + 0x24) = 0x3eaa7efa;
        if (*(int *)(param_1 + 0x38) == iVar2) {
          *(undefined4 *)(param_1 + 0x38) = *(undefined4 *)(iVar2 + 0x2c);
        }
      }
      else {
LAB_004a636a:
        *(undefined1 *)(iVar2 + 0x12) = 0;
        *(undefined4 *)(iVar2 + 0x24) = 0x3f800000;
      }
    }
    else if (sVar3 == 2) {
      if ((short)DAT_006b1844 != 0) goto LAB_004a636a;
      *(undefined1 *)(iVar2 + 0x12) = 1;
      *(undefined4 *)(iVar2 + 0x24) = 0x3eaa7efa;
    }
    iVar2 = *(int *)(iVar2 + 0x2c);
    sVar3 = sVar3 + 1;
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
