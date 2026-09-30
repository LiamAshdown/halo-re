// controls_binding_row_widget_update  (Ghidra: FUN_004b4520, named in phase 4)
// address 0x4b4520, size 622 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: rewritten from objdump 0x4b4520..0x4b478d in the phase-4 review. The first rewrite
// dropped the register arguments of every call (heap_reallocate takes the old block in EAX and
// the widget heap in ESI; controls_action_display_name takes the device in EAX and the action
// name in EDI). EAX indexes the 0x18 byte action table at 0x00692fe8 (char name[0x10], +0x10
// bindable, +0x14 per device column "unbindable" bits). The row is a widget_instance: its
// first child takes the table word +0x10 as selection_index; in the gamepad layout (device 2 and
// up) the sibling after it shows the one binding of that device; otherwise the sibling after
// that is enabled and its first child and that child's next sibling show the keyboard (device
// 0) and mouse (device 1) bindings, dimmed (hidden byte +0x12 set, scale 0.333) when the table
// column bit (1 keyboard, 2 mouse) is set. The row itself is dimmed unless some binding cell
// was left undimmed. Every text slot is heap_reallocate(old, 0x40) in widget_memory_pool and
// a wcsncpy of 0x1f characters terminated at +0x3e.
// register convention: EAX action index; two stack arguments.
//   // blam-cc: action_index -> EAX

#include <wchar.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_memory.h"
#include "fn_interface.h"

extern heap *widget_memory_pool; // 0x006926c4
extern uint8_t controls_action_table[][0x18]; // 0x00692fe8: char name[0x10], int32 bindable, uint32 unbindable columns


static void controls_set_cell_text(widget_instance *cell, const uint16_t *text)
{
    uint16_t *buffer = (uint16_t *)heap_reallocate(cell->text, 0x40, widget_memory_pool);

    cell->text = buffer;
    if (buffer != 0) {
        wcsncpy((wchar_t *)buffer, (const wchar_t *)text, 0x1f); // 0x627a94
        buffer[0x1f] = 0;
    }
}

static void controls_set_cell_dimmed(widget_instance *cell, uint8_t dimmed)
{
    cell->hidden = dimmed;
    cell->scale = dimmed ? 0.333f : 1.0f; // 0x3eaa7efa and 1.0
}

// blam-cc: action_index -> EAX
void controls_binding_row_widget_update(int32_t action_index, widget_instance *row, int32_t device)
{
    static const uint16_t empty_text[1] = {0}; // 0x00660c34
    uint8_t *entry = controls_action_table[action_index];
    const char *action_name = (const char *)entry;
    int32_t bindable = *(int32_t *)(entry + 0x10);
    widget_instance *cell = row->first_child->next_sibling;
    uint8_t some_cell_active = 0;

    row->first_child->selection_index = *(int16_t *)(entry + 0x10);
    if (device >= 2) {
        cell->state = 1;
        if (bindable != 0) {
            controls_set_cell_text(cell, controls_action_display_name(device, action_name));
            some_cell_active = 1;
        } else {
            controls_set_cell_text(cell, empty_text);
        }
    } else {
        cell->state = 0;
    }

    cell = cell->next_sibling;
    if (device < 2) {
        widget_instance *keyboard = cell->first_child;
        widget_instance *mouse = keyboard->next_sibling;

        cell->state = 1;
        if (bindable != 0) {
            controls_set_cell_text(keyboard, controls_action_display_name(0, action_name));
            if ((entry[0x14] & 1) != 0) {
                controls_set_cell_dimmed(keyboard, 1);
            } else {
                controls_set_cell_dimmed(keyboard, 0);
                some_cell_active = 1;
            }
            controls_set_cell_text(mouse, controls_action_display_name(1, action_name));
            if ((entry[0x14] & 2) != 0) {
                controls_set_cell_dimmed(mouse, 1);
            } else {
                controls_set_cell_dimmed(mouse, 0);
                controls_set_cell_dimmed(row, 0);
                return;
            }
        } else {
            controls_set_cell_text(keyboard, empty_text);
            controls_set_cell_text(mouse, empty_text);
        }
    } else {
        cell->state = 0;
    }
    controls_set_cell_dimmed(row, some_cell_active ? 0 : 1);
}

#if 0
Original Ghidra decompilation (0x4b4520):

void FUN_004b4520(int param_1,int param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  bool bVar4;
  int in_EAX;
  wchar_t *pwVar5;
  wchar_t *pwVar6;
  int iVar7;

  iVar7 = in_EAX * 0x18;
  iVar1 = *(int *)(*(int *)(param_1 + 0x34) + 0x2c);
  *(undefined2 *)(*(int *)(param_1 + 0x34) + 0x40) = *(undefined2 *)(&DAT_00692ff8 + iVar7);
  bVar4 = false;
  if (param_2 < 2) {
    *(undefined1 *)(iVar1 + 0x10) = 0;
  }
  else {
    *(undefined1 *)(iVar1 + 0x10) = 1;
    if (*(int *)(&DAT_00692ff8 + iVar7) == 0) {
      pwVar5 = (wchar_t *)heap_reallocate(0x40);
      *(wchar_t **)(iVar1 + 0x3c) = pwVar5;
      if (pwVar5 != (wchar_t *)0x0) {
        _wcsncpy(pwVar5,L"",0x1f);
        *(undefined2 *)(*(int *)(iVar1 + 0x3c) + 0x3e) = 0;
      }
    }
    else {
      pwVar5 = (wchar_t *)FUN_004b44c0();
      pwVar6 = (wchar_t *)heap_reallocate(0x40);
      *(wchar_t **)(iVar1 + 0x3c) = pwVar6;
      if (pwVar6 != (wchar_t *)0x0) {
        _wcsncpy(pwVar6,pwVar5,0x1f);
        *(undefined2 *)(*(int *)(iVar1 + 0x3c) + 0x3e) = 0;
      }
      bVar4 = true;
    }
  }
  iVar1 = *(int *)(iVar1 + 0x2c);
  if (param_2 < 2) {
    iVar2 = *(int *)(iVar1 + 0x34);
    iVar3 = *(int *)(iVar2 + 0x2c);
    *(undefined1 *)(iVar1 + 0x10) = 1;
    if (*(int *)(&DAT_00692ff8 + iVar7) == 0) {
      pwVar5 = (wchar_t *)heap_reallocate(0x40);
      *(wchar_t **)(iVar2 + 0x3c) = pwVar5;
      if (pwVar5 != (wchar_t *)0x0) {
        _wcsncpy(pwVar5,L"",0x1f);
        *(undefined2 *)(*(int *)(iVar2 + 0x3c) + 0x3e) = 0;
      }
      pwVar5 = (wchar_t *)heap_reallocate(0x40);
      *(wchar_t **)(iVar3 + 0x3c) = pwVar5;
      if (pwVar5 != (wchar_t *)0x0) {
        _wcsncpy(pwVar5,L"",0x1f);
        *(undefined2 *)(*(int *)(iVar3 + 0x3c) + 0x3e) = 0;
      }
    }
    else {
      pwVar5 = (wchar_t *)FUN_004b44c0();
      pwVar6 = (wchar_t *)heap_reallocate(0x40);
      *(wchar_t **)(iVar2 + 0x3c) = pwVar6;
      if (pwVar6 != (wchar_t *)0x0) {
        _wcsncpy(pwVar6,pwVar5,0x1f);
        *(undefined2 *)(*(int *)(iVar2 + 0x3c) + 0x3e) = 0;
      }
      if (((&DAT_00692ffc)[iVar7] & 1) == 0) {
        *(undefined1 *)(iVar2 + 0x12) = 0;
        *(undefined4 *)(iVar2 + 0x24) = 0x3f800000;
        bVar4 = true;
      }
      else {
        *(undefined1 *)(iVar2 + 0x12) = 1;
        *(undefined4 *)(iVar2 + 0x24) = 0x3eaa7efa;
      }
      pwVar5 = (wchar_t *)FUN_004b44c0();
      pwVar6 = (wchar_t *)heap_reallocate(0x40);
      *(wchar_t **)(iVar3 + 0x3c) = pwVar6;
      if (pwVar6 != (wchar_t *)0x0) {
        _wcsncpy(pwVar6,pwVar5,0x1f);
        *(undefined2 *)(*(int *)(iVar3 + 0x3c) + 0x3e) = 0;
      }
      if (((&DAT_00692ffc)[iVar7] & 2) == 0) {
        *(undefined1 *)(iVar3 + 0x12) = 0;
        *(undefined4 *)(iVar3 + 0x24) = 0x3f800000;
        *(undefined1 *)(param_1 + 0x12) = 0;
        *(undefined4 *)(param_1 + 0x24) = 0x3f800000;
        return;
      }
      *(undefined1 *)(iVar3 + 0x12) = 1;
      *(undefined4 *)(iVar3 + 0x24) = 0x3eaa7efa;
    }
  }
  else {
    *(undefined1 *)(iVar1 + 0x10) = 0;
  }
  if (bVar4) {
    *(undefined1 *)(param_1 + 0x12) = 0;
    *(undefined4 *)(param_1 + 0x24) = 0x3f800000;
    return;
  }
  *(undefined1 *)(param_1 + 0x12) = 1;
  *(undefined4 *)(param_1 + 0x24) = 0x3eaa7efa;
  return;
}
#endif
