// widget_column_list_sync_selected  (Ghidra: FUN_0049c040, renamed)
// renamed from FUN_0049c040 in the naming pass
// address 0x49c040, size 55 bytes
// name confidence: 0.35   rewrite confidence: 0.55
// evidence: types/interface.h's widget_instance note groups this with 0x49c000/0x49c080/0x49c0f0
// as focused_child upkeep. widget_instance_handle_input_event.c already fixed this function's
// register convention as "column_list upkeep": updates every background-bitmap-frame-2 child's
// background_bitmap_frame to match whether it is the currently focused child (the column_list
// counterpart of FUN_0049c000, minus the "seed focus when nothing is focused" step).
// register convention: fixed by widget_instance_handle_input_event.c: ECX -> widget.
// blam-cc: ECX -> widget

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

// blam-cc: ECX -> widget
void widget_column_list_sync_selected(widget_instance *widget)
{
    widget_instance *child;

    for (child = widget->first_child; child != (widget_instance *)0; child = child->next_sibling) {
        if (child == widget->focused_child) {
            if (child->background_bitmap_frames == 2) {
                child->background_bitmap_frame = 1;
            }
        } else if (child->background_bitmap_frames == 2) {
            child->background_bitmap_frame = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x49c040):

void FUN_0049c040(void)

{
  int iVar1;
  int in_ECX;

  for (iVar1 = *(int *)(in_ECX + 0x34); iVar1 != 0; iVar1 = *(int *)(iVar1 + 0x2c)) {
    if (iVar1 == *(int *)(in_ECX + 0x38)) {
      if (*(short *)(iVar1 + 0x5e) == 2) {
        *(undefined2 *)(iVar1 + 0x58) = 1;
      }
    }
    else if (*(short *)(iVar1 + 0x5e) == 2) {
      *(undefined2 *)(iVar1 + 0x58) = 0;
    }
  }
  return;
}
#endif
