// list_node_prepend  (Ghidra: FUN_00499430; named by types/interface.h's own widget_history_node
// note)
// address 0x499430, size 46 bytes
// name confidence: 0.6   rewrite confidence: 0.5
// evidence: types/interface.h documents this address as list_node_prepend and gives the full
// field layout of widget_history_node (definition/list_definition/selection/controller_index/next) that this
// function allocates and fills from a 3-dword template, pushing it onto a caller-supplied
// singly-linked list head.
// register convention: template record (3 dwords: definition, controller_index, selection) in
// ESI (unaff_ESI, a pointer), list head address in EDI (unaff_EDI), both unresolved register
// reads recovered only by field-shape (matches widget_history_node exactly).
// blam-cc: ESI -> template_record, EDI -> head
// UNSURE: heap_allocate's call here shows no visible arguments; modeled as allocating exactly
// sizeof(widget_history_node), matching this file's own struct.
// FIXED (register inputs, objdump): ESI (read at 0x499444) was already a C parameter
//   (template_record), but the "blam-cc" note said "ESI -> template" (missing the "_record"
//   suffix), so the checker's name matching missed it; corrected the wording.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_memory.h"

extern heap *widget_memory_pool; // 0x006926c4


// blam-cc: ESI -> template_record, EDI -> head
// Allocates a widget_history_node from the widget heap, copies definition/list_definition/
// selection/controller_index from `template`, and pushes it onto the singly-linked list at `*head`.
void list_node_prepend(widget_history_node *template_record, widget_history_node **head)
{
    widget_history_node *node =
        (widget_history_node *)heap_allocate(sizeof(widget_history_node), widget_memory_pool);

    if (node != (widget_history_node *)0) {
        node->definition = template_record->definition;
        node->list_definition = template_record->list_definition;
        node->selection = template_record->selection;
        node->controller_index = template_record->controller_index;
        node->next = *head;
        *head = node;
    }
}

#if 0
Original Ghidra decompilation (0x499430):

void FUN_00499430(void)

{
  undefined4 *puVar1;
  undefined4 *unaff_ESI;
  undefined4 *unaff_EDI;

  puVar1 = (undefined4 *)heap_allocate();
  if (puVar1 != (undefined4 *)0x0) {
    *puVar1 = *unaff_ESI;
    puVar1[1] = unaff_ESI[1];
    puVar1[2] = unaff_ESI[2];
    puVar1[3] = *unaff_EDI;
    *unaff_EDI = puVar1;
  }
  return;
}
#endif
