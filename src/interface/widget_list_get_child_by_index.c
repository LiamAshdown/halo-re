// widget_list_get_child_by_index  (Ghidra: widget_list_get_child_by_index, already named)
// address 0x498630, size 29 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: matches the given name; walks widget->first_child then next_sibling `index` times.
// register convention: widget (list) in EAX (in_EAX), index in EDX (in_EDX), both unresolved
// register reads; Ghidra shows no return, but the walked pointer is clearly the intended result
// (left in EAX at exit), matching this module's other widget-tree walkers.
// blam-cc: EAX -> list, EDX -> index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

// blam-cc: EAX -> list, EDX -> index
// Returns the Nth child of `list` (0-based), or NULL if the child list is shorter than `index`.
widget_instance *widget_list_get_child_by_index(widget_instance *list, int32_t index)
{
    widget_instance *child = list->first_child;
    int32_t i = 0;

    if (index > 0) {
        do {
            if (child == (widget_instance *)0) {
                return (widget_instance *)0;
            }
            child = child->next_sibling;
            i = i + 1;
        } while (i < index);
    }
    return child;
}

#if 0
Original Ghidra decompilation (0x498630):

void widget_list_get_child_by_index(void)

{
  int in_EAX;
  int iVar1;
  int iVar2;
  int in_EDX;

  iVar1 = *(int *)(in_EAX + 0x34);
  iVar2 = 0;
  if (0 < in_EDX) {
    do {
      if (iVar1 == 0) {
        return;
      }
      iVar1 = *(int *)(iVar1 + 0x2c);
      iVar2 = iVar2 + 1;
    } while (iVar2 < in_EDX);
  }
  return;
}
#endif
