// widget_relink_focus_by_tag_id  (Ghidra: FUN_0049bb60, renamed)
// renamed from FUN_0049bb60 in the naming pass
// address 0x49bb60, size 55 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: climbs from `widget` to the topmost ancestor with no parent (the root of the tree
// `widget` belongs to), searches that root's subtree for a widget whose definition equals
// `child_definition` via widget_find_by_tag_id, and if found relinks it into the focus chain via
// widget_instance_relink_focus. Matches functions.md's "finds the topmost ancestor of the current widget and, if
// it matches the supplied lookup key, updates the modal widget-stack linkage."
// register convention: already fixed by three already-rewritten callers (widget_list_select_next.c,
// widget_list_select_previous.c) as EAX -> widget, stack -> child_definition.
// blam-cc: EAX -> widget, stack -> child_definition
// UNSURE: the call to widget_instance_relink_focus passes no visible arguments in Ghidra; EAX still holds the
// widget just returned by widget_find_by_tag_id (the found node) at that point and nothing else
// is live, so both of widget_instance_relink_focus's arguments (widget, child) are modeled as that same found
// node -- self-climb-to-root, then relink itself as the new focus.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern widget_instance *widget_find_by_tag_id(widget_instance *widget, datum_index tag_id); // 0x499950
extern void widget_instance_relink_focus(widget_instance *widget, widget_instance *child); // 0x49bba0

// blam-cc: EAX -> widget, stack -> child_definition
// Climbs from `widget` to the root of its tree, then relinks the root's descendant tagged
// `child_definition` (if any) into the focus chain.
void widget_relink_focus_by_tag_id(widget_instance *widget, datum_index child_definition)
{
    widget_instance *root = widget;
    widget_instance *found;

    while (root->parent != (widget_instance *)0) {
        root = root->parent;
    }
    found = widget_find_by_tag_id(root, child_definition);
    if (found != (widget_instance *)0) {
        widget_instance_relink_focus(found, found);
    }
}

#if 0
Original Ghidra decompilation (0x49bb60):

void FUN_0049bb60(undefined4 param_1)

{
  int iVar1;
  int in_EAX;
  int iVar2;

  iVar2 = *(int *)(in_EAX + 0x30);
  while (iVar1 = iVar2, iVar1 != 0) {
    in_EAX = iVar1;
    iVar2 = *(int *)(iVar1 + 0x30);
  }
  iVar2 = widget_find_by_tag_id(in_EAX,param_1);
  if (iVar2 != 0) {
    FUN_0049bba0();
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
