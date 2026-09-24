// widget_reopen_as_root_with_history  (Ghidra: FUN_0049c4c0, renamed)
// renamed from FUN_0049c4c0 in the naming pass
// address 0x49c4c0, size 202 bytes
// name confidence: 0.35   rewrite confidence: 0.75
// evidence: functions.md: "Loads (or re-focuses) a child widget identified by a tag id, passing
// along a type-appropriate initial value and this widget's sibling position so the loaded widget
// can restore prior state." widget_close.c and ui_widget_list_item_activate.c already reference
// this address as FUN_0049c4c0(widget, open_tag); those callers have not been updated to the new
// name here (the naming-pass propagation script updates call sites across the module afterward).
// Reopens `open_tag` as
// a brand-new root-level widget (parent NULL) on the controller slot named by the tag itself (or,
// failing a UIWidgetDefinitionFlags::always_use_tag_controller_index bit, by the same 0..4 ->
// 0,1,2,3,-1 remap chimera__load_ui_widget itself performs), with a go-back record pointing at
// widget's own tree root, widget's immediate parent's tag, and widget's own sibling index -- the
// three pieces widget_instance_select_list_index needs to refocus `widget` specifically once the
// new widget is later closed.
// register convention: fixed by the two already-rewritten callers: widget in the first
// parameter, open_tag in the second (both cdecl-recognized by Ghidra here).
// Phase-4 s2 review against objdump 0x49c4c0..0x49c589 and the jump tables at 0x49c58c /
// 0x49c5a0: tag controller_index 4 maps to widget->controller_index (+0x08; the earlier rewrite
// read +0x02) when always_use_tag_controller_index is clear and to -1 when it is set; the
// function returns the new widget from chimera__load_ui_widget in EAX, which two of its three
// callers test for NULL.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>
#include "cache.h"

extern tag_instance *tag_instances; // 0x0087bc14

extern widget_instance *chimera__load_ui_widget(char *tag_path, datum_index tag_index,
    widget_instance *parent, uint16_t controller_index, datum_index history_definition,
    datum_index history_list_definition, int16_t history_selection); // 0x497a70, 7 stack args

// Reopens `open_tag` as a new root widget, with a go-back record aimed at restoring focus to
// `widget` specifically (its tree root's definition, its parent's definition, and its own sibling
// index within that parent).
widget_instance *widget_reopen_as_root_with_history(widget_instance *widget, datum_index open_tag)
{
    UIWidgetDefinition *open_definition = (UIWidgetDefinition *)tag_instances[open_tag & 0xffff].data;
    uint16_t controller_index;
    widget_instance *ancestor;
    widget_instance *root;
    datum_index parent_definition;
    int16_t sibling_index;

    if ((open_definition->flags & 0x1000) == 0) { // always_use_tag_controller_index
        switch (open_definition->controller_index) {
        case 0: controller_index = 0; break;
        case 1: controller_index = 1; break;
        case 2: controller_index = 2; break;
        case 3: controller_index = 3; break;
        case 4:
            controller_index = (uint16_t)widget->controller_index; // mov di,[edx+0x8]
            break;
        default: // unreachable for tag values 0..4; the binary loads the widget pointer here
            controller_index = (uint16_t)(uintptr_t)widget;
            break;
        }
    } else {
        switch (open_definition->controller_index) {
        case 0: controller_index = 0; break;
        case 1: controller_index = 1; break;
        case 2: controller_index = 2; break;
        case 3: controller_index = 3; break;
        case 4: controller_index = (uint16_t)-1; break;
        default: controller_index = (uint16_t)(uintptr_t)widget; break; // unreachable, as above
        }
    }

    ancestor = widget->parent;
    if (ancestor == (widget_instance *)0) {
        parent_definition = (datum_index)-1;
        root = widget;
    } else {
        parent_definition = ancestor->definition;
        root = ancestor;
        while (root->parent != (widget_instance *)0) {
            root = root->parent;
        }
    }

    sibling_index = -1;
    if (widget->parent != (widget_instance *)0) {
        widget_instance *cursor;
        int16_t index = 0;

        sibling_index = -1;
        for (cursor = widget->parent->first_child; cursor != (widget_instance *)0;
             cursor = cursor->next_sibling) {
            if (cursor == widget) {
                sibling_index = index;
                break;
            }
            index = index + 1;
        }
    }

    return chimera__load_ui_widget((char *)0, open_tag, (widget_instance *)0, controller_index,
                             root->definition, parent_definition, sibling_index);
}

#if 0
Original Ghidra decompilation (0x49c4c0):

void FUN_0049c4c0(undefined4 *param_1,uint param_2)

{
  undefined2 uVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined4 uVar6;
  int iVar7;
  undefined4 *puVar8;

  iVar7 = *(int *)((param_2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar1 = *(undefined2 *)(iVar7 + 2);
  puVar8 = param_1;
  if ((*(uint *)(iVar7 + 0x2c) & 0x1000) == 0) {
    switch(uVar1) {
    case 0:
      goto switchD_0049c4f1_caseD_0;
    case 1:
      goto switchD_0049c4f1_caseD_1;
    case 2:
      goto switchD_0049c4f1_caseD_2;
    case 3:
      goto switchD_0049c4f1_caseD_3;
    case 4:
      puVar8 = (undefined4 *)(uint)*(ushort *)(param_1 + 2);
    }
  }
  else {
    switch(uVar1) {
    case 0:
switchD_0049c4f1_caseD_0:
      puVar8 = (undefined4 *)0x0;
      break;
    case 1:
switchD_0049c4f1_caseD_1:
      puVar8 = (undefined4 *)0x1;
      break;
    case 2:
switchD_0049c4f1_caseD_2:
      puVar8 = (undefined4 *)0x2;
      break;
    case 3:
switchD_0049c4f1_caseD_3:
      puVar8 = (undefined4 *)0x3;
      break;
    case 4:
      puVar8 = (undefined4 *)0xffffffff;
    }
  }
  puVar2 = (undefined4 *)param_1[0xc];
  puVar3 = puVar2;
  puVar5 = param_1;
  if (puVar2 != (undefined4 *)0x0) {
    do {
      puVar5 = puVar3;
      puVar3 = (undefined4 *)puVar5[0xc];
    } while ((undefined4 *)puVar5[0xc] != (undefined4 *)0x0);
    if (puVar2 != (undefined4 *)0x0) {
      uVar6 = *puVar2;
      goto LAB_0049c549;
    }
  }
  uVar6 = 0xffffffff;
LAB_0049c549:
  iVar7 = -1;
  if (puVar2 != (undefined4 *)0x0) {
    iVar4 = 0;
    for (puVar2 = (undefined4 *)puVar2[0xd];
        (iVar7 = -1, puVar2 != (undefined4 *)0x0 && (iVar7 = iVar4, puVar2 != param_1));
        puVar2 = (undefined4 *)puVar2[0xb]) {
      iVar4 = iVar4 + 1;
    }
  }
  chimera__load_ui_widget(0,param_2,0,puVar8,*puVar5,uVar6,iVar7);
  return;
}
#endif
