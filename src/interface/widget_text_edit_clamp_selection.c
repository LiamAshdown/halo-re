// widget_text_edit_clamp_selection  (Ghidra: FUN_0044c780, renamed per types/interface.h)
// address 0x44c780, size 123 bytes
// name confidence: 0.35   rewrite confidence: 0.7
// evidence: types/interface.h text_edit_state comment; operates on the same {text,
// maximum_length, cursor, selection_anchor} layout as the other four text-edit functions in
// this file group (0x44c290/0x44c5b0/0x44c5e0/0x44c640), recomputing strlen(text) and clamping
// cursor into [0, len] and selection_anchor into [-1, len], collapsing the selection to -1
// when it lands on the cursor.
// register convention: state pointer is param_1 (already recognized by Ghidra as EAX/stack).

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

extern void text_clamp_byte_length_to_character_boundary(char *string, int16_t *length);
// 0x557720, module text (foreign). UNSURE: Ghidra shows zero visible arguments at every call
// site in this module (the register(s) carrying the string base and the in/out length pointer
// are never materialized in the decompilation); the (string, &length) pairing here is inferred
// from the sibling functions text_get_next_character (0x5576a0, EAX=string, ESI=&offset) and
// text_find_character_boundary (0x5576d0, explicit (char*, short*)) and from this call's job
// (documented in symbols/review_queue.txt) of not leaving an offset mid double-byte character.

// Recomputes a text-edit control's current length and clamps its cursor into [0, length] and
// its selection anchor into [-1, length], collapsing the selection to "none" (-1) if it now
// coincides with the cursor. Finally re-snaps the cursor, and the selection anchor if one
// remains, off any double-byte character boundary they might now split.
void widget_text_edit_clamp_selection(text_edit_state *state)
{
    int32_t len;
    int16_t new_cursor;
    int16_t new_selection;

    len = (int32_t)strlen(state->text);

    if (state->cursor < 0) {
        new_cursor = 0;
    } else if (state->cursor <= (int16_t)len) {
        new_cursor = state->cursor;
    } else {
        new_cursor = (int16_t)len;
    }

    if (state->selection_anchor < -1) {
        new_selection = -1;
    } else if (state->selection_anchor <= (int16_t)len) {
        new_selection = state->selection_anchor;
    } else {
        new_selection = (int16_t)len;
    }

    state->cursor = new_cursor;
    state->selection_anchor = new_selection;
    if (new_cursor == new_selection) {
        state->selection_anchor = -1;
    }

    text_clamp_byte_length_to_character_boundary(state->text, &state->cursor);
    if (state->selection_anchor != -1) {
        text_clamp_byte_length_to_character_boundary(state->text, &state->selection_anchor);
    }
}

#if 0
Original Ghidra decompilation (0x44c780):

void FUN_0044c780(undefined4 *param_1)

{
  short *psVar1;
  char cVar2;
  short sVar3;
  short sVar4;
  char *pcVar5;
  short sVar6;

  pcVar5 = (char *)*param_1;
  sVar4 = (short)pcVar5;
  do {
    cVar2 = *pcVar5;
    pcVar5 = pcVar5 + 1;
  } while (cVar2 != '\0');
  sVar3 = *(short *)((int)param_1 + 6);
  sVar4 = (short)pcVar5 - (sVar4 + 1);
  if (sVar3 < 0) {
    sVar6 = 0;
  }
  else {
    sVar6 = sVar4;
    if (sVar3 <= sVar4) {
      sVar6 = sVar3;
    }
  }
  sVar3 = *(short *)(param_1 + 2);
  psVar1 = (short *)(param_1 + 2);
  *(short *)((int)param_1 + 6) = sVar6;
  if (sVar3 < -1) {
    sVar4 = -1;
  }
  else if (sVar3 <= sVar4) {
    sVar4 = sVar3;
  }
  *psVar1 = sVar4;
  if (sVar6 == sVar4) {
    *psVar1 = -1;
  }
  FUN_00557720();
  if (*psVar1 != -1) {
    FUN_00557720();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
