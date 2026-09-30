// widget_instance_select_list_index  (Ghidra: FUN_0049bd00, already named by
// interface_tick.c's phase-4 rewrite and cross-referenced by types/interface.h's
// widget_history_node comment)
// address 0x49bd00, size 200 bytes
// name confidence: 0.6   rewrite confidence: 0.4
// evidence: functions.md: "Moves the widget-stack's current selection to a specific list index
// (or, for a negative index, re-validates and relinks the existing selection), updating the
// target's stored selection field." types/interface.h's widget_history_node note: "on a pop the
// definition is reopened with controller_index as argument 4, then 0x49bd00 gets list_definition
// in EAX and the dword at 0x08 on the stack (it reads only the low word) to restore the
// selection." Sole caller interface_tick.c already fixed the signature/register convention.
// register convention: fixed by interface_tick.c: EAX -> list_definition, EBX -> widget, stack ->
// selection (only the low 16 bits are read). // blam-cc: EAX -> list_definition, EBX -> widget,
// stack -> selection
// UNSURE: widget_instance_relink_focus's call (both branches) is made with no visible arguments; the only value
// in scope at each call site is the widget just found/matched, so both of its arguments (widget,
// child) are modeled as that same widget, matching the same self-climb-and-relink pattern used at
// 0x49bb60's call to it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#include "fn_interface.h"

extern tag_instance *tag_instances; // 0x0087bc14


// blam-cc: EAX -> list_definition, EBX -> widget, stack -> selection
// Finds `widget`'s descendant tagged `list_definition`. With a negative `selection`, re-focuses
// that descendant if it is still input-eligible and is not itself a multi-item spinner_list.
// With a non-negative `selection`, walks the descendant's children to that index (bailing out
// entirely if any child visited along the way, including the target, is a multi-item
// spinner_list), focuses it, and if its parent is itself a list, records the index as the
// parent's own selection_index.
void widget_instance_select_list_index(widget_instance *widget, datum_index list_definition,
                                        int32_t selection)
{
    widget_instance *target;

    if (list_definition == (datum_index)-1) {
        return;
    }
    target = widget_find_by_tag_id(widget, list_definition);
    if (target == (widget_instance *)0) {
        return;
    }

    if ((int16_t)selection < 0) {
        if (widget_instance_is_input_eligible(target) != 0) {
            UIWidgetDefinition *tag = (UIWidgetDefinition *)tag_instances[target->definition & 0xffff].data;

            if (target->widget_type != 2 || tag->child_widgets.count < 2) {
                widget_instance_relink_focus(target, target);
            }
        }
    } else {
        widget_instance *cursor = target->first_child;
        int16_t index = 0;

        for (;;) {
            UIWidgetDefinition *tag;

            if (cursor == (widget_instance *)0) {
                return;
            }
            tag = (UIWidgetDefinition *)tag_instances[cursor->definition & 0xffff].data;
            if (cursor->widget_type == 2 && tag->child_widgets.count > 1) {
                return;
            }
            if (index == (int16_t)selection) {
                break;
            }
            cursor = cursor->next_sibling;
            index = index + 1;
        }
        widget_instance_relink_focus(cursor, cursor);
        if (cursor->parent != (widget_instance *)0 &&
            (cursor->parent->widget_type == 2 || cursor->parent->widget_type == 3)) {
            cursor->parent->selection_index = index;
        }
    }
}

#if 0
Original Ghidra decompilation (0x49bd00):

void FUN_0049bd00(short param_1)

{
  uint uVar1;
  char cVar2;
  int in_EAX;
  uint *puVar3;
  int iVar4;

  if ((in_EAX != -1) && (puVar3 = (uint *)widget_find_by_tag_id(), puVar3 != (uint *)0x0)) {
    if (param_1 < 0) {
      cVar2 = FUN_00499c40();
      if ((cVar2 != '\0') &&
         ((*(short *)((int)puVar3 + 0xe) != 2 ||
          (*(int *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x3e0) < 2)))) {
        FUN_0049bba0();
        return;
      }
    }
    else {
      puVar3 = (uint *)puVar3[0xd];
      iVar4 = 0;
      while( true ) {
        if (puVar3 == (uint *)0x0) {
          return;
        }
        if ((*(short *)((int)puVar3 + 0xe) == 2) &&
           (1 < *(int *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x3e0))) {
          return;
        }
        if (iVar4 == param_1) break;
        puVar3 = (uint *)puVar3[0xb];
        iVar4 = iVar4 + 1;
      }
      FUN_0049bba0();
      uVar1 = puVar3[0xc];
      if ((uVar1 != 0) && ((*(short *)(uVar1 + 0xe) == 2 || (*(short *)(uVar1 + 0xe) == 3)))) {
        *(short *)(uVar1 + 0x40) = (short)iVar4;
        return;
      }
    }
  }
  return;
}
#endif
