// widget_list_scroll_window  (Ghidra: FUN_004a7400; named by types/interface.h's widget_instance
// note, "widget_list_scroll_window @0x4a7400 reads (short)(w + 0x40) as the selected index and
// (ushort)(w + 0x48) as the wrap count")
// address 0x4a7400, size 163 bytes
// name confidence: 0.45 (from types/interface.h)   rewrite confidence: 0.7
// evidence: types/interface.h widget_instance (focused_child, first_child, next_sibling,
// selection_index, item_count all match).
// register convention: output int32_t[3] in EAX (in_EAX), widget in ECX (in_ECX), both
// unresolved register reads.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

// Computes the previous/current/next item indices for a 3-wide scrolling list widget, wrapping
// around item_count, writing -1 for any slot that ends up out of range. Which neighbor of
// selection_index is treated as "current" depends on whether the widget's focused_child is
// still its first row or has scrolled to the second row.
// blam-cc: EAX -> out, ECX -> widget
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void widget_list_scroll_window(int32_t out[3], widget_instance *widget)
{
    int32_t item_count = (uint16_t)widget->item_count;
    int16_t selection = widget->selection_index;

    if (widget->focused_child == widget->first_child) {
        out[0] = selection;
        out[1] = selection + 1;
        if (out[1] == item_count) {
            out[1] = 0;
        }
    } else {
        if (widget->focused_child != widget->first_child->next_sibling) {
            out[2] = selection;
            out[1] = selection - 1;
            if (out[1] < 0) out[1] = item_count - 1;
            out[0] = out[1] - 1;
            if (out[0] < 0) out[0] = item_count - 1;
            goto clamp;
        }
        out[1] = selection;
        out[0] = selection - 1;
        if (out[0] < 0) out[0] = item_count - 1;
    }
    out[2] = out[1] + 1;
    if (out[2] == item_count) {
        out[2] = 0;
    }
clamp:
    if (item_count <= out[0]) out[0] = -1;
    if (item_count <= out[1]) out[1] = -1;
    if (item_count <= out[2]) out[2] = -1;
}

#if 0
Original Ghidra decompilation (0x4a7400):

void FUN_004a7400(void)

{
  short sVar1;
  int *in_EAX;
  int in_ECX;
  uint uVar2;
  int iVar3;

  if (*(int *)(in_ECX + 0x38) == *(int *)(in_ECX + 0x34)) {
    sVar1 = *(short *)(in_ECX + 0x40);
    *in_EAX = (int)sVar1;
    uVar2 = (int)sVar1 + 1;
    in_EAX[1] = uVar2;
    if (uVar2 == *(ushort *)(in_ECX + 0x48)) {
      in_EAX[1] = 0;
    }
  }
  else {
    iVar3 = (int)*(short *)(in_ECX + 0x40);
    if (*(int *)(in_ECX + 0x38) != *(int *)(*(int *)(in_ECX + 0x34) + 0x2c)) {
      in_EAX[2] = iVar3;
      in_EAX[1] = iVar3 + -1;
      if (iVar3 + -1 < 0) {
        in_EAX[1] = *(ushort *)(in_ECX + 0x48) - 1;
      }
      *in_EAX = in_EAX[1] + -1;
      if (in_EAX[1] + -1 < 0) {
        *in_EAX = *(ushort *)(in_ECX + 0x48) - 1;
      }
      goto LAB_004a7477;
    }
    in_EAX[1] = iVar3;
    *in_EAX = iVar3 + -1;
    if (iVar3 + -1 < 0) {
      *in_EAX = *(ushort *)(in_ECX + 0x48) - 1;
    }
  }
  in_EAX[2] = in_EAX[1] + 1U;
  if (in_EAX[1] + 1U == (uint)*(ushort *)(in_ECX + 0x48)) {
    in_EAX[2] = 0;
  }
LAB_004a7477:
  if ((int)(uint)*(ushort *)(in_ECX + 0x48) <= *in_EAX) {
    *in_EAX = -1;
  }
  if ((int)(uint)*(ushort *)(in_ECX + 0x48) <= in_EAX[1]) {
    in_EAX[1] = -1;
  }
  if ((int)(uint)*(ushort *)(in_ECX + 0x48) <= in_EAX[2]) {
    in_EAX[2] = -1;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
