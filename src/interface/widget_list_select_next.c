// widget_list_select_next  (Ghidra: widget_list_select_next, already named)
// address 0x4986b0, size 366 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: matches the given name; moves a list/scrollable widget's selection_index (0x40)
// forward by one, wrapping at item_count (0x48), with different bookkeeping for column_list
// (0x0d/0x0e sibling-chain walk plus widget_list_get_child_by_index) vs spinner_list (a tag
// flags_2 bit-1 gate plus child_widgets.count) vs the plain case (list_items (0x44) present).
// register convention: widget in the recognized stack parameter (Ghidra's own param_1).
// UNSURE: widget_relink_focus_by_tag_id and widget_instance_relink_focus (0x49bb60, 0x49bba0) are called with no visible
// arguments by Ghidra; modeled from call-site context only (widget_relink_focus_by_tag_id receives the newly
// selected child's own definition/tag index; widget_instance_relink_focus is called with no observable input or
// output here and is declared void(void)). types/interface.h's widget_instance note also
// references 0x49bba0 as a "hidden child" skip site, which this call does not obviously match;
// not reconciled.

// Phase-4 review: verified line by line against objdump 0x4986b0..0x49881d; fixed the
// other-list-type branch (it skips the selection_index store) and the two focus helpers, which
// take the widget in EAX (and the child in ECX for 0x49bba0).

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

extern widget_instance *widget_list_get_child_by_index(widget_instance *list, int32_t index); // 0x498630, blam-cc: EAX -> list, EDX -> index
extern void widget_relink_focus_by_tag_id(widget_instance *widget, datum_index child_definition); // 0x49bb60; blam-cc: EAX -> widget, stack -> child_definition (objdump 0x498713)
extern void widget_instance_relink_focus(widget_instance *widget, widget_instance *child); // 0x49bba0, focus change; blam-cc: EAX -> widget, ECX -> child (objdump 0x49bba0: walks EAX up +0x30, tests ECX+0x12)

// Moves a list or scrollable widget's current selection to the next item, wrapping around at the
// end; for column_list, walks the focused child's sibling chain and reopens the new child; for
// spinner_list, reselects a child by computed index when the tag allows scroll-window paging;
// otherwise, if a generated list_items array is present, just advances the stored index. Marks
// the widget dirty (0x54) and resets its scroll blink timer (0x42) whenever a selection change is
// committed.
uint8_t widget_list_select_next(widget_instance *widget)
{
    UIWidgetDefinition *tag = (UIWidgetDefinition *)tag_instances[widget->definition & 0xffff].data;

    if (widget->list_items != (void *)0 && widget->item_count != 0) {
        int16_t next_index = widget->selection_index + 1;

        if ((int32_t)(uint16_t)widget->item_count <= next_index) {
            next_index = 0;
        }
        if (widget->widget_type == 3) { // column_list
            widget_instance *child = widget_list_get_child_by_index(widget, next_index);

            if (child == (widget_instance *)0) {
                return 0;
            }
            widget_relink_focus_by_tag_id(widget, child->definition);
        } else if (widget->widget_type == 2) { // spinner_list
            if (tag->child_widgets.count > 1) {
                widget_instance *focused = widget->focused_child;

                // Matches the original's unconditional dereference of first_child->next_sibling
                // here; first_child is an invariant non-NULL given list_items/item_count are
                // already both set on this path.
                if ((focused == widget->first_child || focused == widget->first_child->next_sibling) &&
                    focused->next_sibling != (widget_instance *)0) {
                    widget_instance_relink_focus(widget, focused->next_sibling);
                    widget->selection_index = next_index;
                    widget->selection_direction = 1;
                    widget->scroll_blink = 0xf;
                    return 1;
                }
            }
        } else {
            goto commit; // objdump 0x498736 -> 0x498722 skips the selection_index store
        }
        widget->selection_index = next_index;
        goto commit;
    }

    if (widget->widget_type != 2 || (tag->flags_2 & 2) == 0 || tag->child_widgets.count != 0) {
        widget_instance *child;

        if ((widget->focused_child == (widget_instance *)0 ||
             (child = widget->focused_child->next_sibling,
              (int32_t)(widget->selection_index + 1) == (uint16_t)widget->item_count) ||
             (child = widget->focused_child->next_sibling, child == (widget_instance *)0)) &&
            (child = widget->first_child, child == (widget_instance *)0)) {
            return 0;
        }
        {
            widget_instance *cursor = widget->first_child;
            int16_t index = 0;

            widget_relink_focus_by_tag_id(widget, child->definition);
            if (cursor != (widget_instance *)0) {
                index = 0;
                do {
                    if (cursor == widget->focused_child) {
                        break;
                    }
                    cursor = cursor->next_sibling;
                    index = index + 1;
                } while (cursor != (widget_instance *)0);
            }
            widget->selection_direction = 1;
            widget->selection_index = index;
            widget->scroll_blink = 0xf;
            return 1;
        }
    }

    widget->selection_index = widget->selection_index + 1;
    if ((uint16_t)widget->selection_index == (uint16_t)widget->item_count) {
        widget->selection_direction = 1;
        widget->selection_index = 0;
        widget->scroll_blink = 0xf;
        return 1;
    }

commit:
    widget->selection_direction = 1;
    widget->scroll_blink = 0xf;
    return 1;
}

#if 0
Original Ghidra decompilation (0x4986b0):

undefined4 widget_list_select_next(uint *param_1)

{
  int iVar1;
  undefined4 *puVar2;
  uint uVar3;
  short sVar4;
  int iVar5;

  iVar1 = *(int *)((*param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (param_1[0x11] != 0) {
    if ((ushort)param_1[0x12] != 0) {
      iVar5 = (short)param_1[0x10] + 1;
      if ((int)(uint)(ushort)param_1[0x12] <= iVar5) {
        iVar5 = 0;
      }
      if (*(short *)((int)param_1 + 0xe) == 3) {
        puVar2 = (undefined4 *)widget_list_get_child_by_index();
        if (puVar2 == (undefined4 *)0x0) {
          return 0;
        }
        FUN_0049bb60(*puVar2);
      }
      else {
        if (*(short *)((int)param_1 + 0xe) != 2) goto LAB_00498722;
        if (1 < *(int *)(iVar1 + 0x3e0)) {
          uVar3 = param_1[0xe];
          if (((uVar3 == param_1[0xd]) || (uVar3 == *(uint *)(param_1[0xd] + 0x2c))) &&
             (*(int *)(uVar3 + 0x2c) != 0)) {
            FUN_0049bba0();
            *(short *)(param_1 + 0x10) = (short)iVar5;
            *(undefined2 *)(param_1 + 0x15) = 1;
            *(undefined2 *)((int)param_1 + 0x42) = 0xf;
            return 1;
          }
        }
      }
      *(short *)(param_1 + 0x10) = (short)iVar5;
      goto LAB_00498722;
    }
  }
  if (((*(short *)((int)param_1 + 0xe) != 2) || ((*(byte *)(iVar1 + 0x150) & 2) == 0)) ||
     (*(int *)(iVar1 + 0x3e0) != 0)) {
    if ((((param_1[0xe] == 0) ||
         (puVar2 = *(undefined4 **)(param_1[0xe] + 0x2c),
         (int)(short)param_1[0x10] + 1U == (uint)(ushort)param_1[0x12])) ||
        (puVar2 == (undefined4 *)0x0)) &&
       (puVar2 = (undefined4 *)param_1[0xd], puVar2 == (undefined4 *)0x0)) {
      return 0;
    }
    FUN_0049bb60(*puVar2);
    uVar3 = param_1[0xd];
    sVar4 = 0;
    if (uVar3 != 0) {
      sVar4 = 0;
      do {
        if (uVar3 == param_1[0xe]) break;
        uVar3 = *(uint *)(uVar3 + 0x2c);
        sVar4 = sVar4 + 1;
      } while (uVar3 != 0);
    }
    *(undefined2 *)(param_1 + 0x15) = 1;
    *(short *)(param_1 + 0x10) = sVar4;
    *(undefined2 *)((int)param_1 + 0x42) = 0xf;
    return 1;
  }
  *(short *)(param_1 + 0x10) = (short)param_1[0x10] + 1;
  if ((int)(short)param_1[0x10] == (uint)(ushort)param_1[0x12]) {
    *(undefined2 *)(param_1 + 0x15) = 1;
    *(undefined2 *)(param_1 + 0x10) = 0;
    *(undefined2 *)((int)param_1 + 0x42) = 0xf;
    return 1;
  }
LAB_00498722:
  *(undefined2 *)(param_1 + 0x15) = 1;
  *(undefined2 *)((int)param_1 + 0x42) = 0xf;
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
