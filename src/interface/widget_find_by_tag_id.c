// widget_find_by_tag_id  (Ghidra: widget_find_by_tag_id, already named)
// address 0x499950, size 64 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: matches the given name; compares widget->definition (offset 0x00) against a
// caller-supplied datum_index and, on mismatch, recurses depth-first over first_child (0x34)
// then next_sibling (0x2c), returning the first widget whose definition matches.
// register convention: cdecl, both recognized stack parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

// Recursively searches a widget tree for a node whose tag/definition matches `tag_id`, returning
// it, or NULL if none of the subtree matches.
widget_instance *widget_find_by_tag_id(widget_instance *widget, datum_index tag_id)
{
    widget_instance *found;
    widget_instance *child;

    if (widget->definition == tag_id) {
        return widget;
    }
    found = (widget_instance *)0;
    child = widget->first_child;
    while (child != (widget_instance *)0 && found == (widget_instance *)0) {
        found = child;
        if (child->definition != tag_id) {
            found = widget_find_by_tag_id(child, tag_id);
        }
        child = child->next_sibling;
    }
    return found;
}

#if 0
Original Ghidra decompilation (0x499950):

int * widget_find_by_tag_id(int *param_1,int param_2)

{
  int *piVar1;
  int *piVar2;

  piVar2 = (int *)0x0;
  if (*param_1 != param_2) {
    piVar1 = (int *)param_1[0xd];
    while ((piVar1 != (int *)0x0 && (piVar2 == (int *)0x0))) {
      piVar2 = piVar1;
      if (*piVar1 != param_2) {
        piVar2 = (int *)widget_find_by_tag_id(piVar1,param_2);
      }
      piVar1 = (int *)piVar1[0xb];
    }
    return piVar2;
  }
  return param_1;
}
#endif
