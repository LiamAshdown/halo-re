// widget_focus_next_child  (Ghidra: FUN_0049c080, renamed in the phase-4 review)
// address 0x49c080, size 112 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: functions.md: "Advances a list widget's current selection to the next selectable
// sibling, wrapping to the first child when the end of the list is reached." widget_instance_
// handle_input_event.c (already rewritten, outside this range) already fixed the register
// convention (EDX -> widget) but its own header comment labels this address "focus to previous
// child" -- the OPPOSITE of what this function's own body does: it walks focused_child->next_
// sibling (offset 0x2c) forward, wrapping to first_child, not focused_child->previous_sibling.
// widget_focus_previous_child (0x49c0f0) is the one that walks previous_sibling. This looked like a genuine
// swapped label in that already-completed file, flagged here rather than corrected there (out of
// this session's range); a follow-up pass should fix widget_instance_handle_input_event.c's two
// comments (and, if callers were also swapped, its two call sites). Phase-4 review: the two
// extern comments are fixed; the call sites were right, they call by address.
// register convention: fixed by widget_instance_handle_input_event.c: EDX -> widget.
// blam-cc: EDX -> widget

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#include "fn_interface.h"

extern tag_instance *tag_instances; // 0x0087bc14

// blam-cc: EDX -> widget
// Starting just after the currently focused child (or at the first child if none is focused),
// scans forward through the sibling ring -- wrapping past the last child back to the first -- for
// a child that is not hidden and is either a list itself or has at least one game_data_input
// binding, or whose container `widget` is itself a spinner_list/column_list, and focuses the
// first one found. Does nothing if the ring has no such candidate.
void widget_focus_next_child(widget_instance *widget)
{
    widget_instance *current = widget->focused_child;
    widget_instance *candidate;

    if (current != (widget_instance *)0 && current->next_sibling != (widget_instance *)0) {
        candidate = current->next_sibling;
    } else {
        candidate = widget->first_child;
    }
    if (candidate == (widget_instance *)0) {
        return;
    }

    while (candidate != current) {
        UIWidgetDefinition *tag = (UIWidgetDefinition *)tag_instances[candidate->definition & 0xffff].data;

        if ((tag->game_data_inputs.count > 0 || (tag->flags & 1) != 0 ||
             widget->widget_type == 2 || widget->widget_type == 3) &&
            candidate->hidden == 0) {
            widget->focused_child = candidate;
            return;
        }
        candidate = candidate->next_sibling;
        if (candidate == (widget_instance *)0) {
            candidate = widget->first_child;
            if (candidate == (widget_instance *)0) {
                return;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x49c080):

void FUN_0049c080(void)

{
  uint *puVar1;
  int iVar2;
  uint *puVar3;
  int in_EDX;

  puVar1 = *(uint **)(in_EDX + 0x38);
  if (((puVar1 != (uint *)0x0) && (puVar3 = (uint *)puVar1[0xb], puVar3 != (uint *)0x0)) ||
     (puVar3 = *(uint **)(in_EDX + 0x34), puVar3 != (uint *)0x0)) {
    while (puVar3 != puVar1) {
      iVar2 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      if ((((0 < *(int *)(iVar2 + 0x54)) || ((*(byte *)(iVar2 + 0x2c) & 1) != 0)) ||
          ((*(short *)(in_EDX + 0xe) == 2 || (*(short *)(in_EDX + 0xe) == 3)))) &&
         (*(char *)((int)puVar3 + 0x12) == '\0')) {
        *(uint **)(in_EDX + 0x38) = puVar3;
        return;
      }
      puVar3 = (uint *)puVar3[0xb];
      if ((puVar3 == (uint *)0x0) && (puVar3 = *(uint **)(in_EDX + 0x34), puVar3 == (uint *)0x0)) {
        return;
      }
    }
  }
  return;
}
#endif
