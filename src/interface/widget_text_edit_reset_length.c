// widget_text_edit_reset_length  (Ghidra: FUN_0044c5b0, renamed per types/interface.h)
// address 0x44c5b0, size 36 bytes
// name confidence: 0.3   rewrite confidence: 0.7
// evidence: types/interface.h text_edit_state comment group; recomputes strlen(text) into the
// cursor field and clears selection_anchor to -1, matching "Recomputes and caches a text-edit
// control's string length and clears its selection after the underlying string changed" from
// out/phase4/interface_functions.md.
// register convention: state pointer in ESI (unaff_ESI).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void widget_text_edit_clamp_selection(text_edit_state *state); // 0x44c780, this module

// blam-cc: state in ESI (unaff_ESI)
// Re-clamps the control, then re-derives its cursor from the current string length and drops
// any selection: called whenever the underlying string was replaced out from under the editor.
void widget_text_edit_reset_length(text_edit_state *state)
{
    widget_text_edit_clamp_selection(state);

    state->cursor = (int16_t)strlen(state->text);
    state->selection_anchor = -1;
}

#if 0
Original Ghidra decompilation (0x44c5b0):

void FUN_0044c5b0(void)

{
  char cVar1;
  short sVar2;
  char *pcVar3;
  undefined4 *unaff_ESI;

  FUN_0044c780();
  pcVar3 = (char *)*unaff_ESI;
  sVar2 = (short)pcVar3;
  do {
    cVar1 = *pcVar3;
    pcVar3 = pcVar3 + 1;
  } while (cVar1 != '\0');
  *(short *)((int)unaff_ESI + 6) = (short)pcVar3 - (sVar2 + 1);
  *(undefined2 *)(unaff_ESI + 2) = 0xffff;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
