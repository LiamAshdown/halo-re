// widget_instance_is_top_of_stack  (Ghidra: FUN_00499cb0; named by types/interface.h's
// widget_instance note)
// address 0x499cb0, size 69 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: types/interface.h documents this address: "The focus stack that
// widget_instance_is_top_of_stack @0x499cb0 walks is the same tree read upward, checking at
// each step that parent->focused_child is the node it came from."
// register convention: widget in EAX (in_EAX), unresolved register read.
// blam-cc: EAX -> widget

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

// blam-cc: EAX -> widget
// True if `widget` is reachable from the root purely by following focused_child (i.e. it is the
// widget actually on top / receiving input), or if any ancestor along the way is a list type
// (spinner_list/column_list), which counts as "focused" regardless of the exact child.
uint8_t widget_instance_is_top_of_stack(widget_instance *widget)
{
    widget_instance *cursor = widget->parent; // Ghidra's iVar3
    uint8_t is_focused;
    widget_instance *ancestor; // Ghidra's iVar1

    if (cursor == (widget_instance *)0) {
        return 1;
    }
    is_focused = (cursor->focused_child == widget);
    if (is_focused) {
        return 1;
    }
    do {
        ancestor = cursor->parent;
        if (ancestor != (widget_instance *)0) {
            uint8_t is_list;

            if (ancestor->focused_child != cursor) {
                return 0;
            }
            is_list = (ancestor->widget_type == 2 || ancestor->widget_type == 3);
            is_focused = is_focused | is_list;
        }
        cursor = ancestor;
    } while (ancestor != (widget_instance *)0);
    return is_focused;
}

#if 0
Original Ghidra decompilation (0x499cb0):

bool FUN_00499cb0(void)

{
  int iVar1;
  int in_EAX;
  byte bVar2;
  int iVar3;
  bool bVar4;

  iVar3 = *(int *)(in_EAX + 0x30);
  if (iVar3 == 0) {
    return true;
  }
  bVar4 = *(int *)(iVar3 + 0x38) == in_EAX;
  if (bVar4) {
    return true;
  }
  do {
    iVar1 = *(int *)(iVar3 + 0x30);
    if (iVar1 != 0) {
      if (*(int *)(iVar1 + 0x38) != iVar3) {
        return false;
      }
      if ((*(short *)(iVar1 + 0xe) == 2) || (*(short *)(iVar1 + 0xe) == 3)) {
        bVar2 = 1;
      }
      else {
        bVar2 = 0;
      }
      bVar4 = (bool)(bVar4 | bVar2);
    }
    iVar3 = iVar1;
  } while (iVar1 != 0);
  return bVar4;
}
#endif
