// widget_focus_previous_child  (Ghidra: FUN_0049c0f0, renamed in the phase-4 review)
// address 0x49c0f0, size 168 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: functions.md: "Moves a list widget's current selection to the previous selectable
// sibling, wrapping to the last child when the start of the list is reached." This function walks
// previous_sibling (offset 0x28); see FUN_0049c080.c's header for the mirror-image function (which
// walks next_sibling) and the swapped-label note about widget_instance_handle_input_event.c.
// register convention: fixed by widget_instance_handle_input_event.c: EDX -> widget.
// blam-cc: EDX -> widget

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances; // 0x0087bc14

// blam-cc: EDX -> widget
// Mirror image of FUN_0049c080: starting just before the currently focused child (or at the last
// child if none is focused), scans backward through the sibling ring -- wrapping past the first
// child back to the last -- for an eligible candidate (same test as FUN_0049c080) and focuses it.
void widget_focus_previous_child(widget_instance *widget)
{
    widget_instance *focused = widget->focused_child;
    widget_instance *candidate;
    widget_instance *tail;
    UIWidgetDefinition *tag;

    if (focused == (widget_instance *)0) {
        if (widget->first_child == (widget_instance *)0) {
            return;
        }
        candidate = widget->first_child->previous_sibling;
        if (candidate == (widget_instance *)0) {
            candidate = widget->first_child;
        }
    } else {
        candidate = focused->previous_sibling;
        if (candidate == (widget_instance *)0) {
            tail = widget->first_child;
            if (tail == (widget_instance *)0) {
                return;
            }
            while (tail->next_sibling != (widget_instance *)0) {
                tail = tail->next_sibling;
            }
            candidate = tail;
        }
    }

    for (;;) {
        while (candidate != (widget_instance *)0) {
            if (candidate == focused) {
                return;
            }
            tag = (UIWidgetDefinition *)tag_instances[candidate->definition & 0xffff].data;
            if ((tag->game_data_inputs.count > 0 || (tag->flags & 1) != 0 ||
                 widget->widget_type == 2 || widget->widget_type == 3) &&
                candidate->hidden == 0) {
                widget->focused_child = candidate;
                return;
            }
            candidate = candidate->previous_sibling;
        }
        candidate = widget->first_child;
        if (candidate == (widget_instance *)0) {
            return;
        }
        while (candidate->next_sibling != (widget_instance *)0) {
            candidate = candidate->next_sibling;
        }
    }
}

#if 0
Original Ghidra decompilation (0x49c0f0):

void FUN_0049c0f0(void)

{
  uint *puVar1;
  uint *puVar2;
  int iVar3;
  uint *puVar4;
  int in_EDX;

  puVar1 = *(uint **)(in_EDX + 0x38);
  if (puVar1 == (uint *)0x0) {
    puVar4 = (uint *)(*(uint **)(in_EDX + 0x34))[10];
    puVar2 = *(uint **)(in_EDX + 0x34);
    if (puVar4 != (uint *)0x0) goto LAB_0049c134;
  }
  else {
    puVar4 = (uint *)puVar1[10];
    if (puVar4 != (uint *)0x0) goto LAB_0049c134;
    puVar2 = *(uint **)(in_EDX + 0x34);
    if (puVar2 == (uint *)0x0) {
      return;
    }
    for (puVar4 = (uint *)puVar2[0xb]; puVar4 != (uint *)0x0; puVar4 = (uint *)puVar4[0xb]) {
      puVar2 = puVar4;
    }
  }
  puVar4 = puVar2;
  if (puVar4 == (uint *)0x0) {
    return;
  }
LAB_0049c134:
  do {
    do {
      if (puVar4 == puVar1) {
        return;
      }
      iVar3 = *(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      if (((((0 < *(int *)(iVar3 + 0x54)) || ((*(byte *)(iVar3 + 0x2c) & 1) != 0)) ||
           (*(short *)(in_EDX + 0xe) == 2)) || (*(short *)(in_EDX + 0xe) == 3)) &&
         (*(char *)((int)puVar4 + 0x12) == '\0')) {
        *(uint **)(in_EDX + 0x38) = puVar4;
        return;
      }
      puVar4 = (uint *)puVar4[10];
    } while (puVar4 != (uint *)0x0);
    puVar4 = *(uint **)(in_EDX + 0x34);
    if (puVar4 == (uint *)0x0) {
      return;
    }
    for (puVar2 = (uint *)puVar4[0xb]; puVar2 != (uint *)0x0; puVar2 = (uint *)puVar2[0xb]) {
      puVar4 = puVar2;
    }
  } while (puVar4 != (uint *)0x0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
