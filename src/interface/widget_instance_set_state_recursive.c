// widget_instance_set_state_recursive  (Ghidra: FUN_00498e60, unnamed)
// address 0x498e60, size 40 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/interface.h's widget_instance note attributes this exact address to the
// `state` field (offset 0x10): "set to 1 at creation, rewritten recursively by 0x498e60".
// register convention: cdecl, both recognized stack parameters (Ghidra fully resolved the
// signature).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Sets widget->state (and every descendant's state, recursively through first_child then
// next_sibling) to the low byte of `state`.
void widget_instance_set_state_recursive(widget_instance *widget, uint8_t state)
{
    widget_instance *child = widget->first_child;

    widget->state = state;
    for (; child != (widget_instance *)0; child = child->next_sibling) {
        widget_instance_set_state_recursive(child, state);
    }
}

#if 0
Original Ghidra decompilation (0x498e60):

void FUN_00498e60(int param_1,undefined4 param_2)

{
  int iVar1;

  iVar1 = *(int *)(param_1 + 0x34);
  *(char *)(param_1 + 0x10) = (char)param_2;
  for (; iVar1 != 0; iVar1 = *(int *)(iVar1 + 0x2c)) {
    FUN_00498e60(iVar1,param_2);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
