// widget_text_edit_get_selection  (Ghidra: FUN_0044c5e0, renamed per types/interface.h)
// address 0x44c5e0, size 85 bytes
// name confidence: 0.35   rewrite confidence: 0.7
// evidence: types/interface.h text_edit_state comment group; out/phase4/interface_functions.md
// "Computes the clamped selection start/end offsets for a text-edit control and reports whether
// a selection is active."
// register convention: state pointer in EAX (in_EAX); out_start/out_end are Ghidra-recognized
// stack parameters (param_1, param_2). // blam-cc: state=EAX, out_start=param_1, out_end=param_2

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void widget_text_edit_clamp_selection(text_edit_state *state); // 0x44c780, this module

// Re-clamps the control, then reports its selection as an ordered [start, end) pair (start is
// the lower of cursor/selection_anchor, end the higher). Returns 0 and leaves *out_start/
// *out_end untouched when there is no active selection.
uint32_t widget_text_edit_get_selection(text_edit_state *state, int16_t *out_start, int16_t *out_end)
{
    int16_t start;
    int16_t end;

    widget_text_edit_clamp_selection(state);

    if (state->selection_anchor == -1) {
        return 0;
    }

    start = (state->cursor < state->selection_anchor) ? state->cursor : state->selection_anchor;
    *out_start = start;

    end = (state->selection_anchor <= state->cursor) ? state->cursor : state->selection_anchor;
    *out_end = end;

    return 1;
}

#if 0
Original Ghidra decompilation (0x44c5e0):

uint FUN_0044c5e0(short *param_1,short *param_2)

{
  short sVar1;
  int in_EAX;
  undefined2 extraout_var;

  FUN_0044c780();
  sVar1 = *(short *)(in_EAX + 8);
  if (sVar1 == -1) {
    return CONCAT22(extraout_var,sVar1) & 0xffffff00;
  }
  if (*(short *)(in_EAX + 6) < sVar1) {
    sVar1 = *(short *)(in_EAX + 6);
  }
  *param_1 = sVar1;
  sVar1 = *(short *)(in_EAX + 8);
  if (*(short *)(in_EAX + 8) <= *(short *)(in_EAX + 6)) {
    sVar1 = *(short *)(in_EAX + 6);
  }
  *param_2 = sVar1;
  return CONCAT31((int3)(char)((ushort)sVar1 >> 8),1);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
