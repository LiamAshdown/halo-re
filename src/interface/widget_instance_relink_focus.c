// widget_instance_relink_focus  (Ghidra: FUN_0049bba0, renamed)
// renamed from FUN_0049bba0 in the naming pass
// address 0x49bba0, size 324 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: functions.md: "Re-links a widget instance into the modal/selection stack, preferring
// the closest visible or list-type sibling if the direct target widget is hidden." Four already-
// rewritten callers (widget_list_select_next.c, widget_list_select_previous.c,
// widget_initialize_from_tag.c, ui_widget_list_item_activate.c) already fixed this function's
// name (kept as FUN_0049bba0), signature and register convention from objdump: "EAX -> widget,
// ECX -> child, walks EAX up +0x30, tests ECX+0x12". Those four callers still use the old FUN_
// name; the naming-pass propagation script updates call sites across the module afterward.
// register convention: blam-cc: EAX -> widget, ECX -> child (fixed by prior callers)
// UNSURE: the parent-scan's "did the forward scan land back on the parent's own focused_child"
// check (`cursor == parent->focused_child`) is preserved exactly as Ghidra renders it; the intent
// (detect "no other candidate exists" before falling back to a backward scan) is inferred, not
// independently confirmed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#include "fn_interface.h"

extern tag_instance *tag_instances; // 0x0087bc14

// A sibling is a focus candidate if it is not hidden and is either itself a list
// (spinner_list/column_list) or its tag has at least one game_data_input binding.
static uint8_t widget_is_focus_candidate(widget_instance *candidate)
{
    UIWidgetDefinition *tag = (UIWidgetDefinition *)tag_instances[candidate->definition & 0xffff].data;
    return (uint8_t)(candidate->hidden == 0 &&
                      (tag->game_data_inputs.count > 0 || candidate->widget_type == 2 ||
                       candidate->widget_type == 3));
}

// blam-cc: EAX -> widget, ECX -> child
// Relinks `child` (or, if it is hidden, the closest eligible sibling -- preferring forward, then
// falling back to scanning backward when the forward scan only rediscovers the parent's current
// focus) as the focused_child all the way up the tree rooted at the topmost ancestor of `widget`.
// If the previous focus chain shared child's immediate parent, only that one link is rewritten;
// otherwise the whole old chain (from the root down) is cleared first.
void widget_instance_relink_focus(widget_instance *widget, widget_instance *child)
{
    widget_instance *root = widget;
    widget_instance *old_focus;
    widget_instance *cursor;

    while (root->parent != (widget_instance *)0) {
        root = root->parent;
    }
    old_focus = root->focused_child;

    if (child->hidden != 0) {
        widget_instance *found = (widget_instance *)0;

        for (cursor = child->next_sibling; cursor != (widget_instance *)0; cursor = cursor->next_sibling) {
            if (widget_is_focus_candidate(cursor)) {
                found = cursor;
                break;
            }
        }
        if (found == (widget_instance *)0 && child->parent != (widget_instance *)0) {
            widget_instance *parent = child->parent;

            for (cursor = parent->first_child;
                 cursor != (widget_instance *)0 && !widget_is_focus_candidate(cursor);
                 cursor = cursor->next_sibling) {
            }
            if (cursor == parent->focused_child) {
                for (cursor = child->previous_sibling; cursor != (widget_instance *)0;
                     cursor = cursor->previous_sibling) {
                    if (widget_is_focus_candidate(cursor)) {
                        found = cursor;
                        break;
                    }
                }
            } else {
                found = cursor;
            }
        }
        if (found != (widget_instance *)0) {
            child = found;
        }
    }

    if (old_focus != (widget_instance *)0) {
        if (child != (widget_instance *)0 && child->parent != (widget_instance *)0 &&
            old_focus->parent == child->parent) {
            child->parent->focused_child = child;
            return;
        }
        while (old_focus != (widget_instance *)0) {
            widget_instance *next = old_focus->focused_child;
            old_focus->parent->focused_child = (widget_instance *)0;
            old_focus = next;
        }
    }

    while (child->parent != (widget_instance *)0) {
        child->parent->focused_child = child;
        child = child->parent;
    }
}

#if 0
Original Ghidra decompilation (0x49bba0):

void FUN_0049bba0(void)

{
  uint uVar1;
  int iVar2;
  int in_EAX;
  uint *puVar3;
  uint *in_ECX;
  int iVar4;

  iVar4 = *(int *)(in_EAX + 0x30);
  while (iVar2 = iVar4, iVar2 != 0) {
    in_EAX = iVar2;
    iVar4 = *(int *)(iVar2 + 0x30);
  }
  iVar4 = *(int *)(in_EAX + 0x38);
  if (*(char *)((int)in_ECX + 0x12) == '\x01') {
    for (puVar3 = (uint *)in_ECX[0xb]; puVar3 != (uint *)0x0; puVar3 = (uint *)puVar3[0xb]) {
      if ((*(char *)((int)puVar3 + 0x12) == '\0') &&
         (((0 < *(int *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x54) ||
           (*(short *)((int)puVar3 + 0xe) == 2)) || (*(short *)((int)puVar3 + 0xe) == 3))))
      goto LAB_0049bc9b;
    }
    uVar1 = in_ECX[0xc];
    if (uVar1 != 0) {
      for (puVar3 = *(uint **)(uVar1 + 0x34);
          (puVar3 != (uint *)0x0 &&
          ((*(char *)((int)puVar3 + 0x12) != '\0' ||
           (((*(int *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x54) < 1 &&
             (*(short *)((int)puVar3 + 0xe) != 2)) && (*(short *)((int)puVar3 + 0xe) != 3))))));
          puVar3 = (uint *)puVar3[0xb]) {
      }
      if (puVar3 == *(uint **)(uVar1 + 0x38)) {
        for (puVar3 = (uint *)in_ECX[10]; puVar3 != (uint *)0x0; puVar3 = (uint *)puVar3[10]) {
          if ((*(char *)((int)puVar3 + 0x12) == '\0') &&
             (((0 < *(int *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x54) ||
               (*(short *)((int)puVar3 + 0xe) == 2)) || (*(short *)((int)puVar3 + 0xe) == 3))))
          goto LAB_0049bc97;
        }
        goto LAB_0049bc9e;
      }
    }
LAB_0049bc97:
    if (puVar3 != (uint *)0x0) {
LAB_0049bc9b:
      in_ECX = puVar3;
    }
  }
LAB_0049bc9e:
  if (iVar4 != 0) {
    if (in_ECX != (uint *)0x0) {
      if ((*(uint *)(iVar4 + 0x30) == in_ECX[0xc]) && (*(uint *)(iVar4 + 0x30) != 0)) {
        *(uint **)(in_ECX[0xc] + 0x38) = in_ECX;
        return;
      }
    }
    do {
      *(undefined4 *)(*(int *)(iVar4 + 0x30) + 0x38) = 0;
      iVar4 = *(int *)(iVar4 + 0x38);
    } while (iVar4 != 0);
  }
  uVar1 = in_ECX[0xc];
  while (uVar1 != 0) {
    *(uint **)(in_ECX[0xc] + 0x38) = in_ECX;
    in_ECX = (uint *)in_ECX[0xc];
    uVar1 = in_ECX[0xc];
  }
  return;
}
#endif
