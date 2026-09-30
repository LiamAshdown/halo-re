// controls_binding_list_refresh_rows  (Ghidra: FUN_004b4790, named in phase 4)
// address 0x4b4790, size 148 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: rewritten from objdump 0x4b4790..0x4b4823 in the phase-4 review (the first rewrite
// lost the action index, EAX of 0x4b4520). EAX is the controls list widget; the stack argument
// is the page. The device is the controls_device_label of the selection_index of the first
// child of its first child's first child; the eight rows after the two header children show
// actions page * 8 .. page * 8 + 7 through controls_binding_row_widget_update. The binary also
// pushes (row index == 0x006953e8) as a third argument, which 0x4b4520 never reads. Returns
// which child has the focus: 0 or 1 for the two headers, 2 for a row, -1 for none.
// register convention: EAX widget; one stack argument.
//   // blam-cc: widget -> EAX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern controls_device_label controls_device_labels[0x10]; // 0x006932e8


// blam-cc: widget -> EAX
int32_t controls_binding_list_refresh_rows(widget_instance *widget, int32_t page)
{
    widget_instance *header = widget->first_child;
    widget_instance *row;
    int32_t focus = -1;
    int32_t device_label;
    int32_t i;

    if (header->parent->focused_child == header) {
        focus = 0;
    }
    device_label = header->first_child->next_sibling->selection_index;
    header = header->next_sibling;
    if (header->parent->focused_child == header) {
        focus = 1;
    }
    row = header->next_sibling;
    for (i = 0; i < 8; i++) {
        controls_binding_row_widget_update(page * 8 + i, row, controls_device_labels[device_label].device_type);
        if (row->parent->focused_child == row) {
            focus = 2;
        }
        row = row->next_sibling;
    }
    return focus;
}

#if 0
Original Ghidra decompilation (0x4b4790):

undefined4 FUN_004b4790(void)

{
  undefined4 *puVar1;
  int in_EAX;
  int iVar2;
  int iVar3;
  int iVar4;
  undefined4 local_4;

  iVar2 = *(int *)(in_EAX + 0x34);
  iVar4 = 0;
  local_4 = 0xffffffff;
  if (*(int *)(*(int *)(iVar2 + 0x30) + 0x38) == iVar2) {
    local_4 = 0;
  }
  iVar3 = *(int *)(iVar2 + 0x2c);
  if (*(int *)(*(int *)(iVar3 + 0x30) + 0x38) == iVar3) {
    local_4 = 1;
  }
  iVar3 = *(int *)(iVar3 + 0x2c);
  iVar2 = *(short *)(*(int *)(*(int *)(iVar2 + 0x34) + 0x2c) + 0x40) * 0x210;
  puVar1 = (undefined4 *)(&DAT_006934f4 + iVar2);
  do {
    FUN_004b4520(iVar3,*puVar1,CONCAT31((int3)((uint)iVar2 >> 8),iVar4 == DAT_006953e8));
    iVar2 = *(int *)(*(int *)(iVar3 + 0x30) + 0x38);
    if (iVar2 == iVar3) {
      local_4 = 2;
    }
    iVar3 = *(int *)(iVar3 + 0x2c);
    iVar4 = iVar4 + 1;
  } while (iVar4 < 8);
  return local_4;
}
#endif
