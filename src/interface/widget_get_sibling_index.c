// widget_get_sibling_index  (Ghidra: widget_get_sibling_index, already named)
// address 0x498e30, size 35 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: matches the given name and types/interface.h's widget_instance note ("walks
// parent(+0x30)->first_child(+0x34) then +0x2c"); returns the widget's 0-based position among
// its parent's children, or -1 if it has no parent or is not found in the chain.
// register convention: widget in ESI (unaff_ESI), unresolved register read.
// blam-cc: ESI -> widget

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: ESI -> widget
// Returns widget's 0-based index among its parent's children (first_child, then next_sibling),
// or -1 if it has no parent or is not found while walking the sibling chain.
int32_t widget_get_sibling_index(widget_instance *widget)
{
    int32_t index = -1;

    if (widget->parent != (widget_instance *)0) {
        widget_instance *cursor = widget->parent->first_child;
        int32_t i = 0;

        if (cursor != (widget_instance *)0) {
            while (cursor != widget) {
                cursor = cursor->next_sibling;
                i = i + 1;
                if (cursor == (widget_instance *)0) {
                    return -1;
                }
            }
            index = i;
        }
    }
    return index;
}

#if 0
Original Ghidra decompilation (0x498e30):

int widget_get_sibling_index(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int unaff_ESI;

  iVar2 = -1;
  iVar3 = iVar2;
  if (*(int *)(unaff_ESI + 0x30) != 0) {
    iVar4 = *(int *)(*(int *)(unaff_ESI + 0x30) + 0x34);
    iVar1 = 0;
    if (iVar4 != 0) {
      while (iVar3 = iVar1, iVar4 != unaff_ESI) {
        iVar4 = *(int *)(iVar4 + 0x2c);
        iVar1 = iVar1 + 1;
        if (iVar4 == 0) {
          return iVar2;
        }
      }
    }
  }
  return iVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
