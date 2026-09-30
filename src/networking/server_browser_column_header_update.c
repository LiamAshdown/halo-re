// server_browser_column_header_update  (Ghidra: FUN_004b7f10, still unnamed -> renamed)
// address 0x4b7f10, size 89 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("updates one server-browser list
// column header widget's highlight/alpha state and, given a nonzero sort-direction value in
// EDX, sets the up/down sort-arrow visibility on its child widgets"); reuses the same
// selected_child/highlight_flag/alpha fields as the other server_browser_* UI files in this
// module.
// register convention: header widget in EAX (in_EAX), sort direction in EDX (in_EDX): 0 hides
// the arrow icon entirely, positive shows the up arrow, negative shows the down arrow.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

// blam-cc: header widget in EAX (in_EAX), sort direction in EDX (in_EDX)
void server_browser_column_header_update(network_ui_widget *header, int32_t sort_direction)
{
    network_ui_widget *icon;
    network_ui_widget *up_arrow;
    network_ui_widget *down_arrow;

    icon = header->first_child;
    if (header->parent->selected_child == header) {
        header->selected_child = icon;
        header->highlight_flag = 1;
    } else {
        header->selected_child = 0;
        header->highlight_flag = 0;
    }
    *(uint32_t *)&icon->alpha = 0x3f000000; // 0.5f
    if (sort_direction == 0) {
        icon->visible = 0;
        return;
    }
    up_arrow = icon->first_child;
    icon->visible = 1;
    up_arrow->highlight_flag = 1;
    up_arrow->visible = (0 < sort_direction);
    down_arrow = up_arrow->next_sibling;
    down_arrow->highlight_flag = 1;
    down_arrow->visible = (sort_direction < 0);
}

#if 0
Original Ghidra decompilation (0x4b7f10):

void FUN_004b7f10(void)

{
  int iVar1;
  int iVar2;
  int in_EAX;
  int in_EDX;

  iVar1 = *(int *)(in_EAX + 0x34);
  if (*(int *)(*(int *)(in_EAX + 0x30) + 0x38) == in_EAX) {
    *(int *)(in_EAX + 0x38) = iVar1;
    *(undefined2 *)(in_EAX + 0x58) = 1;
  }
  else {
    *(undefined4 *)(in_EAX + 0x38) = 0;
    *(undefined2 *)(in_EAX + 0x58) = 0;
  }
  *(undefined4 *)(iVar1 + 0x24) = 0x3f000000;
  if (in_EDX == 0) {
    *(undefined1 *)(iVar1 + 0x10) = 0;
    return;
  }
  iVar2 = *(int *)(iVar1 + 0x34);
  *(undefined1 *)(iVar1 + 0x10) = 1;
  *(undefined2 *)(iVar2 + 0x58) = 1;
  *(bool *)(iVar2 + 0x10) = 0 < in_EDX;
  iVar1 = *(int *)(iVar2 + 0x2c);
  *(undefined2 *)(iVar1 + 0x58) = 1;
  *(bool *)(iVar1 + 0x10) = in_EDX < 0;
  return;
}
#endif
