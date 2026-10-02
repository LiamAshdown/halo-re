// widget_instance_find_root  (Ghidra: FUN_00498e10, unnamed)
// address 0x498e10, size 17 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: walks widget->parent (offset 0x30) until it hits NULL; Ghidra's decompile shows the
// walk cursor itself going to NULL at exit (an empty-bodied for-loop with no separate "previous"
// variable), which cannot be the real intended result since every caller of a parent-chain walk
// wants the topmost ancestor, not NULL -- modeled as returning the last non-NULL node reached,
// the natural "find the root of this widget's tree" reading.
// register convention: widget in EAX (in_EAX), unresolved register read.
// blam-cc: EAX -> widget
// UNSURE: sole caller (FUN_0049d2e0 @0x49d2e0) is outside this session's range, so the return
// value's real use is not cross-checked here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

// blam-cc: EAX -> widget
// Walks up the parent chain and returns the topmost ancestor (the widget itself if it has no
// parent).
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
widget_instance *widget_instance_find_root(widget_instance *widget)
{
    while (widget->parent != (widget_instance *)0) {
        widget = widget->parent;
    }
    return widget;
}

#if 0
Original Ghidra decompilation (0x498e10):

void FUN_00498e10(void)

{
  int iVar1;
  int in_EAX;

  for (iVar1 = *(int *)(in_EAX + 0x30); iVar1 != 0; iVar1 = *(int *)(iVar1 + 0x30)) {
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
