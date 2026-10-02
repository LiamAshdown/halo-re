// widget_list_select_previous  (Ghidra: widget_list_select_previous, already named)
// address 0x498820, size 449 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: matches the given name; mirror of widget_list_select_next (0x4986b0) with the walk
// direction reversed and, in the plain-widget case, an extra "eligible sibling" search (skips
// hidden children, and among the visible ones prefers a child with its own event handlers or
// that is itself a list) walking previous_sibling (0x28) with wraparound to the last child via
// next_sibling (0x2c).
// register convention: widget in the recognized stack parameter (Ghidra's own param_1).
// UNSURE: widget_relink_focus_by_tag_id / widget_instance_relink_focus, see widget_list_select_next.c's identical note.

// Phase-4 review: verified line by line against objdump 0x498820..0x4989e0; the focus helpers now
// carry their register arguments.

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

extern widget_instance *widget_list_get_child_by_index(widget_instance *list, int32_t index); // 0x498630
extern void widget_relink_focus_by_tag_id(widget_instance *widget, datum_index child_definition); // 0x49bb60; blam-cc: EAX -> widget, stack -> child_definition (objdump 0x498713)
extern void widget_instance_relink_focus(widget_instance *widget, widget_instance *child); // 0x49bba0, focus change; blam-cc: EAX -> widget, ECX -> child (objdump 0x49bba0: walks EAX up +0x30, tests ECX+0x12)

// Moves a list or scrollable widget's current selection to the previous item, wrapping around at
// the start; column_list and spinner_list use the same index-based reselection as
// widget_list_select_next, while the plain case searches previous_sibling for the nearest
// eligible (non-hidden, event-bearing or list-type) child, wrapping to the last child.
uint8_t widget_list_select_previous(widget_instance *widget)
{
    UIWidgetDefinition *tag = (UIWidgetDefinition *)tag_instances[widget->definition & 0xffff].data;

    if (widget->list_items != (void *)0 && widget->item_count != 0) {
        int16_t prev_index = widget->selection_index - 1;

        if (prev_index < 0) {
            prev_index = (int16_t)((uint16_t)widget->item_count - 1);
        }
        if (widget->widget_type == 3) { // column_list
            widget_instance *child = widget_list_get_child_by_index(widget, prev_index);

            if (child == (widget_instance *)0) {
                return 0;
            }
            widget_relink_focus_by_tag_id(widget, child->definition);
            widget->selection_index = prev_index;
            widget->scroll_blink = (int16_t)0xfff1;
            widget->selection_direction = 0xffff;
            return 1;
        }
        if (widget->widget_type == 2) { // spinner_list
            if (tag->child_widgets.count > 1 && widget->focused_child != widget->first_child &&
                widget->focused_child != (widget_instance *)0 &&
                widget->focused_child->previous_sibling != (widget_instance *)0) {
                widget_instance_relink_focus(widget, widget->focused_child->previous_sibling);
            }
            widget->selection_index = prev_index;
            widget->scroll_blink = (int16_t)0xfff1;
            widget->selection_direction = 0xffff;
            return 1;
        }
        goto commit;
    }

    if (widget->widget_type == 2 && (tag->flags_2 & 2) != 0 && tag->child_widgets.count == 0) {
        widget->selection_index = widget->selection_index - 1;
        if (widget->selection_index < 0) {
            widget->selection_index = widget->item_count - 1;
            widget->scroll_blink = (int16_t)0xfff1;
            widget->selection_direction = 0xffff;
            return 1;
        }
        goto commit;
    }

    {
        widget_instance *cursor;
        int32_t index;

        if (widget->focused_child == (widget_instance *)0) {
            widget_instance *tail = widget->first_child;

            index = 0;
            for (cursor = (tail != (widget_instance *)0) ? tail->next_sibling : (widget_instance *)0;
                 cursor != (widget_instance *)0; cursor = cursor->next_sibling) {
                index = index + 1;
                tail = cursor;
            }
            cursor = tail;
        } else {
            cursor = widget->focused_child->previous_sibling;
            index = widget->selection_index - 1;
            if (cursor == (widget_instance *)0) {
                widget_instance *tail = widget->first_child;

                index = 0;
                {
                    widget_instance *w;

                    for (w = (tail != (widget_instance *)0) ? tail->next_sibling : (widget_instance *)0;
                         w != (widget_instance *)0; w = w->next_sibling) {
                        index = index + 1;
                        tail = w;
                    }
                }
                cursor = tail;
            }
        }

        if (index != widget->selection_index) {
            do {
                if (cursor->hidden == 0) {
                    UIWidgetDefinition *cursor_tag =
                        (UIWidgetDefinition *)tag_instances[cursor->definition & 0xffff].data;

                    if (cursor_tag->event_handlers.count > 0 || cursor->widget_type == 2 ||
                        cursor->widget_type == 3) {
                        break;
                    }
                }
                index = index - 1;
                cursor = cursor->previous_sibling;
                if (index < 0) {
                    widget_instance *tail = widget->first_child;

                    index = 0;
                    {
                        widget_instance *w;

                        for (w = (tail != (widget_instance *)0) ? tail->next_sibling : (widget_instance *)0;
                             w != (widget_instance *)0; w = w->next_sibling) {
                            index = index + 1;
                            tail = w;
                        }
                    }
                    cursor = tail;
                }
            } while (index != widget->selection_index);
        }

        widget_relink_focus_by_tag_id(widget, cursor->definition);
        {
            widget_instance *walk = widget->first_child;
            int16_t found_index = 0;

            if (walk != (widget_instance *)0) {
                found_index = 0;
                do {
                    if (walk == widget->focused_child) {
                        break;
                    }
                    walk = walk->next_sibling;
                    found_index = found_index + 1;
                } while (walk != (widget_instance *)0);
            }
            widget->selection_index = found_index;
        }
    }

commit:
    widget->scroll_blink = (int16_t)0xfff1;
    widget->selection_direction = 0xffff;
    return 1;
}

#if 0
Original Ghidra decompilation (0x498820):

undefined4 widget_list_select_previous(uint *param_1)

{
  uint *puVar1;
  undefined4 *puVar2;
  uint *puVar3;
  uint uVar4;
  short sVar5;
  int iVar6;
  int iVar7;

  iVar6 = *(int *)((*param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((param_1[0x11] != 0) && ((ushort)param_1[0x12] != 0)) {
    iVar7 = (short)param_1[0x10] + -1;
    if (iVar7 < 0) {
      iVar7 = (ushort)param_1[0x12] - 1;
    }
    if (*(short *)((int)param_1 + 0xe) == 3) {
      puVar2 = (undefined4 *)widget_list_get_child_by_index();
      if (puVar2 == (undefined4 *)0x0) {
        return 0;
      }
      FUN_0049bb60(*puVar2);
      *(short *)(param_1 + 0x10) = (short)iVar7;
      *(undefined2 *)((int)param_1 + 0x42) = 0xfff1;
      *(undefined2 *)(param_1 + 0x15) = 0xffff;
      return 1;
    }
    if (*(short *)((int)param_1 + 0xe) == 2) {
      if (((1 < *(int *)(iVar6 + 0x3e0)) && (param_1[0xe] != param_1[0xd])) &&
         (*(int *)(param_1[0xe] + 0x28) != 0)) {
        FUN_0049bba0();
      }
      *(short *)(param_1 + 0x10) = (short)iVar7;
      *(undefined2 *)((int)param_1 + 0x42) = 0xfff1;
      *(undefined2 *)(param_1 + 0x15) = 0xffff;
      return 1;
    }
    goto LAB_004989d0;
  }
  if (((*(short *)((int)param_1 + 0xe) == 2) && ((*(byte *)(iVar6 + 0x150) & 2) != 0)) &&
     (*(int *)(iVar6 + 0x3e0) == 0)) {
    *(short *)(param_1 + 0x10) = (short)param_1[0x10] + -1;
    if ((short)param_1[0x10] < 0) {
      *(short *)(param_1 + 0x10) = (short)param_1[0x12] + -1;
      *(undefined2 *)((int)param_1 + 0x42) = 0xfff1;
      *(undefined2 *)(param_1 + 0x15) = 0xffff;
      return 1;
    }
    goto LAB_004989d0;
  }
  if (param_1[0xe] == 0) {
LAB_00498930:
    iVar6 = 0;
    puVar3 = (uint *)param_1[0xd];
    for (puVar1 = (uint *)((uint *)param_1[0xd])[0xb]; puVar1 != (uint *)0x0;
        puVar1 = (uint *)puVar1[0xb]) {
      iVar6 = iVar6 + 1;
      puVar3 = puVar1;
    }
  }
  else {
    puVar3 = *(uint **)(param_1[0xe] + 0x28);
    iVar6 = (short)param_1[0x10] + -1;
    if (puVar3 == (uint *)0x0) goto LAB_00498930;
  }
  if (iVar6 != (short)param_1[0x10]) {
    do {
      if ((*(char *)((int)puVar3 + 0x12) == '\0') &&
         (((0 < *(int *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x54) ||
           (*(short *)((int)puVar3 + 0xe) == 2)) || (*(short *)((int)puVar3 + 0xe) == 3)))) break;
      iVar6 = iVar6 + -1;
      puVar3 = (uint *)puVar3[10];
      if (iVar6 < 0) {
        iVar6 = 0;
        puVar3 = (uint *)param_1[0xd];
        for (puVar1 = (uint *)((uint *)param_1[0xd])[0xb]; puVar1 != (uint *)0x0;
            puVar1 = (uint *)puVar1[0xb]) {
          iVar6 = iVar6 + 1;
          puVar3 = puVar1;
        }
      }
    } while (iVar6 != (short)param_1[0x10]);
  }
  FUN_0049bb60(*puVar3);
  uVar4 = param_1[0xd];
  sVar5 = 0;
  if (uVar4 != 0) {
    sVar5 = 0;
    do {
      if (uVar4 == param_1[0xe]) break;
      uVar4 = *(uint *)(uVar4 + 0x2c);
      sVar5 = sVar5 + 1;
    } while (uVar4 != 0);
  }
  *(short *)(param_1 + 0x10) = sVar5;
LAB_004989d0:
  *(undefined2 *)((int)param_1 + 0x42) = 0xfff1;
  *(undefined2 *)(param_1 + 0x15) = 0xffff;
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
