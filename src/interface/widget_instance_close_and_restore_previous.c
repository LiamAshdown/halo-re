// widget_instance_close_and_restore_previous  (Ghidra: FUN_0049c3e0; named by types/interface.h's
// own widget_history_node note)
// address 0x49c3e0, size 214 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: types/interface.h widget_history_node comment: "0x49c3e0 pops one, reloads +0x00
// through widget_open with +0x04 as the controller argument, and hands the low half of +0x08 to
// widget_instance_select_list_index @0x49bd00." ui_widget_list_item_activate.c already fixed the
// name and register convention (EAX -> widget). The pop logic here is byte-for-byte
// list_node_pop's own body inlined a second time; reused via that call rather than duplicated.
// register convention: fixed by ui_widget_list_item_activate.c: EAX -> widget.
// blam-cc: EAX -> widget
// UNSURE: the reopen call's controller_index argument and the final selection-restore call both
// show no visible source in Ghidra beyond a stack dword carrying {controller_index (high),
// selection (low)}; EAX (list_definition) and EBX (widget) for the trailing
// widget_instance_select_list_index call are modeled from the freshly reopened widget and the
// popped node's own list_definition field, per types/interface.h's own account of this call.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern widget_history_node *ui_widget_history[3]; // 0x00718f98
extern uint8_t ui_restoring_previous_widget;      // 0x00718fcb

extern void widget_close(widget_instance *widget); // 0x497c00
extern void list_node_pop(widget_history_node *out, widget_history_node **head); // 0x499460
extern widget_instance *chimera__load_ui_widget(char *tag_path, datum_index tag_index,
    widget_instance *parent, uint16_t controller_index, datum_index history_definition,
    datum_index history_list_definition, int16_t history_selection); // 0x497a70, 7 stack args
extern void widget_instance_select_list_index(widget_instance *widget, datum_index list_definition,
                                               int32_t selection); // 0x49bd00

// blam-cc: EAX -> widget
// Closes the whole tree `widget` belongs to (climbing to its root first) and, if that
// controller slot has a saved go-back record, pops it and reopens the widget it names (with no
// go-back record of its own), restoring its saved list selection.
void widget_instance_close_and_restore_previous(widget_instance *widget)
{
    int16_t slot = (widget->controller_index == (int16_t)0xffff) ? 0 : widget->controller_index;
    widget_history_node history;
    datum_index history_definition = (datum_index)-1;
    widget_instance *root;
    widget_instance *reopened;

    if (ui_widget_history[slot] != (widget_history_node *)0) {
        list_node_pop(&history, &ui_widget_history[slot]);
        history_definition = history.definition;
    }

    root = widget;
    while (root->parent != (widget_instance *)0) {
        root = root->parent;
    }
    widget_close(root);

    if (history_definition != (datum_index)-1) {
        ui_restoring_previous_widget = 1;
        reopened = chimera__load_ui_widget((char *)0, history_definition, (widget_instance *)0,
                                            (uint16_t)history.controller_index,
                                            (datum_index)-1, (datum_index)-1, -1);
        ui_restoring_previous_widget = 0;
        if (reopened != (widget_instance *)0) {
            widget_instance_select_list_index(reopened, history.list_definition, history.selection);
        }
    }
}

#if 0
Original Ghidra decompilation (0x49c3e0):

void FUN_0049c3e0(void)

{
  int *piVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  ushort uVar5;
  int in_EAX;
  int iVar6;
  int extraout_ECX;
  undefined4 uVar7;
  undefined2 local_8;
  undefined2 uStack_6;
  undefined2 uStack_4;

  uVar7 = 0xffffffff;
  uVar5 = *(ushort *)(in_EAX + 8) & (*(ushort *)(in_EAX + 8) == 0xffff) - 1;
  piVar1 = (int *)(&DAT_00718f98)[(short)uVar5];
  if (piVar1 == (int *)0x0) {
    iVar6 = -1;
  }
  else {
    iVar6 = *piVar1;
    local_8 = (undefined2)piVar1[2];
    uStack_6 = (undefined2)((uint)piVar1[2] >> 0x10);
    (&DAT_00718f98)[(short)uVar5] = piVar1[3];
    uVar2 = piVar1[-4];
    heap_unlink_block();
    uVar7 = CONCAT22(uStack_4,uStack_6);
    *(uint *)(extraout_ECX + 0x14) = *(int *)(extraout_ECX + 0x14) - (uVar2 & 0x7fffffff);
    *(int *)(extraout_ECX + 0x1c) = *(int *)(extraout_ECX + 0x1c) + -1;
  }
  iVar4 = *(int *)(in_EAX + 0x30);
  while (iVar3 = iVar4, iVar3 != 0) {
    in_EAX = iVar3;
    iVar4 = *(int *)(iVar3 + 0x30);
  }
  widget_close(in_EAX);
  if (iVar6 != -1) {
    DAT_00718fcb = 1;
    iVar6 = chimera__load_ui_widget(0,iVar6,0,uVar7,0xffffffff,0xffffffff,0xffffffff);
    DAT_00718fcb = 0;
    if (iVar6 != 0) {
      FUN_0049bd00(CONCAT22(uStack_6,local_8));
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
