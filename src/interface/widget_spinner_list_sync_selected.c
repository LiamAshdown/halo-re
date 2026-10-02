// widget_spinner_list_sync_selected  (Ghidra: FUN_0049c000, renamed)
// renamed from FUN_0049c000 in the naming pass
// address 0x49c000, size 63 bytes
// name confidence: 0.35   rewrite confidence: 0.55
// evidence: types/interface.h's widget_instance note: "0x49c000/0x49c040/0x49c080/0x49c0f0 move
// it [focused_child] around the sibling ring." widget_instance_handle_input_event.c (already
// rewritten, outside this range) already fixed this function's signature/register convention as
// spinner_list upkeep: when the tag has exactly 3 child widgets and nothing is focused yet, seeds
// focused_child to first_child; then clears background_bitmap_frame on every child, setting it to
// 1 only on whichever child is currently focused and itself a background-bitmap-frame-2 entry.
// register convention: fixed by widget_instance_handle_input_event.c: ECX -> widget, EAX -> tag.
// blam-cc: ECX -> widget, EAX -> tag

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

// blam-cc: ECX -> widget, EAX -> tag
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void widget_spinner_list_sync_selected(widget_instance *widget, UIWidgetDefinition *tag)
{
    widget_instance *child;

    if (tag->child_widgets.count == 3 && widget->focused_child == (widget_instance *)0) {
        widget->focused_child = widget->first_child;
    }
    for (child = widget->first_child; child != (widget_instance *)0; child = child->next_sibling) {
        child->background_bitmap_frame = 0;
        if (child == widget->focused_child && child->background_bitmap_frames == 2) {
            child->background_bitmap_frame = 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x49c000):

void FUN_0049c000(void)

{
  int iVar1;
  int in_EAX;
  int in_ECX;

  if ((*(int *)(in_EAX + 0x3e0) == 3) && (*(int *)(in_ECX + 0x38) == 0)) {
    *(undefined4 *)(in_ECX + 0x38) = *(undefined4 *)(in_ECX + 0x34);
  }
  for (iVar1 = *(int *)(in_ECX + 0x34); iVar1 != 0; iVar1 = *(int *)(iVar1 + 0x2c)) {
    *(undefined2 *)(iVar1 + 0x58) = 0;
    if ((iVar1 == *(int *)(in_ECX + 0x38)) && (*(short *)(iVar1 + 0x5e) == 2)) {
      *(undefined2 *)(iVar1 + 0x58) = 1;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
