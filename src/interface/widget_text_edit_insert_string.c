// widget_text_edit_insert_string  (Ghidra: FUN_0044c640, renamed per types/interface.h)
// address 0x44c640, size 310 bytes
// name confidence: 0.35   rewrite confidence: 0.65
// evidence: types/interface.h text_edit_state comment group; out/phase4/interface_functions.md
// "Inserts (or replaces the current selection with) a C string into a widget text-edit buffer,
// honoring its maximum length." Every pointer-difference expression in the Ghidra output
// collapses to strlen(text)+strlen(insert_str) once the +1/-1 terms (which come from each
// strlen loop advancing one byte past the terminating NUL) are cancelled.
// register convention: state in ESI (unaff_ESI), insert_str in EAX (in_EAX; briefly reused to
// carry state into widget_text_edit_get_selection, then reloaded with the string by the
// compiler -- invisible in the decompilation but not observable from this function's own
// behaviour). // blam-cc: state=ESI, insert_str=EAX

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
extern uint32_t widget_text_edit_get_selection(text_edit_state *state, int16_t *out_start, int16_t *out_end); // 0x44c5e0, this module
extern void text_clamp_byte_length_to_character_boundary(char *string, int16_t *length); // 0x557720, module text (foreign); see widget_text_edit_clamp_selection.c for the UNSURE note on its register convention.

// With no active selection: inserts insert_str at the cursor (shifting the tail right), but
// only if the resulting string still fits under maximum_length; the cursor advances past the
// inserted text. With an active selection: unconditionally replaces the selected range with
// insert_str (no maximum_length check in this path, matching the original), moves the cursor to
// the end of the inserted text, and clears the selection. Either way, the cursor is finally
// re-snapped off any double-byte character boundary it might now split.
void widget_text_edit_insert_string(text_edit_state *state, char *insert_str)
{
    int16_t sel_start;
    int16_t sel_end;
    uint32_t has_selection;
    char *insertion_point;
    char *tail_source;
    int32_t insert_len;
    int32_t tail_len;

    has_selection = widget_text_edit_get_selection(state, &sel_start, &sel_end);

    if (!has_selection) {
        insert_len = (int32_t)strlen(insert_str);
        if ((int32_t)strlen(state->text) + insert_len < (int32_t)state->maximum_length) {
            insertion_point = state->text + state->cursor;
            tail_len = (int32_t)strlen(insertion_point);
            memmove(insertion_point + insert_len, insertion_point, (size_t)(tail_len + 1));
            while (*insert_str != '\0') {
                state->text[state->cursor] = *insert_str;
                state->cursor = state->cursor + 1;
                insert_str = insert_str + 1;
            }
        }
    } else {
        tail_source = state->text + sel_end;
        tail_len = (int32_t)strlen(tail_source);
        insert_len = (int32_t)strlen(insert_str);
        memmove(state->text + sel_start + insert_len, tail_source, (size_t)(tail_len + 1));
        state->cursor = sel_start;
        state->selection_anchor = -1;
        while (*insert_str != '\0') {
            state->text[state->cursor] = *insert_str;
            state->cursor = state->cursor + 1;
            insert_str = insert_str + 1;
        }
    }

    text_clamp_byte_length_to_character_boundary(state->text, &state->cursor);
}

#if 0
Original Ghidra decompilation (0x44c640):

void FUN_0044c640(void)

{
  char cVar1;
  char *in_EAX;
  char *pcVar2;
  char *pcVar3;
  char *pcVar4;
  int *unaff_ESI;
  char *local_8;
  short local_4 [2];

  cVar1 = FUN_0044c5e0(local_4,&local_8);
  if (cVar1 == '\0') {
    pcVar2 = (char *)*unaff_ESI;
    pcVar3 = pcVar2;
    do {
      cVar1 = *pcVar3;
      pcVar3 = pcVar3 + 1;
    } while (cVar1 != '\0');
    pcVar4 = in_EAX;
    do {
      cVar1 = *pcVar4;
      pcVar4 = pcVar4 + 1;
    } while (cVar1 != '\0');
    if (pcVar3 + (int)(pcVar4 + (-(int)(in_EAX + 1) - (int)(pcVar2 + 1))) <
        (char *)(int)(short)unaff_ESI[1]) {
      pcVar2 = pcVar2 + *(short *)((int)unaff_ESI + 6);
      pcVar3 = pcVar2;
      do {
        cVar1 = *pcVar3;
        pcVar3 = pcVar3 + 1;
      } while (cVar1 != '\0');
      pcVar4 = in_EAX;
      do {
        cVar1 = *pcVar4;
        pcVar4 = pcVar4 + 1;
      } while (cVar1 != '\0');
      _memmove(pcVar2 + ((int)pcVar4 - (int)(in_EAX + 1)),pcVar2,
               (size_t)(pcVar3 + (1 - (int)(pcVar2 + 1))));
      cVar1 = *in_EAX;
      while (cVar1 != '\0') {
        *(char *)((int)*(short *)((int)unaff_ESI + 6) + *unaff_ESI) = cVar1;
        *(short *)((int)unaff_ESI + 6) = *(short *)((int)unaff_ESI + 6) + 1;
        pcVar2 = in_EAX + 1;
        in_EAX = in_EAX + 1;
        cVar1 = *pcVar2;
      }
    }
  }
  else {
    local_8 = (char *)((int)(short)local_8 + *unaff_ESI);
    pcVar2 = local_8;
    do {
      cVar1 = *pcVar2;
      pcVar2 = pcVar2 + 1;
    } while (cVar1 != '\0');
    pcVar3 = in_EAX;
    do {
      cVar1 = *pcVar3;
      pcVar3 = pcVar3 + 1;
    } while (cVar1 != '\0');
    _memmove(pcVar3 + (((int)local_4[0] + *unaff_ESI) - (int)(in_EAX + 1)),local_8,
             (size_t)(pcVar2 + (1 - (int)(local_8 + 1))));
    *(short *)((int)unaff_ESI + 6) = local_4[0];
    *(undefined2 *)(unaff_ESI + 2) = 0xffff;
    cVar1 = *in_EAX;
    if (cVar1 != '\0') {
      do {
        *(char *)((int)*(short *)((int)unaff_ESI + 6) + *unaff_ESI) = cVar1;
        *(short *)((int)unaff_ESI + 6) = *(short *)((int)unaff_ESI + 6) + 1;
        cVar1 = in_EAX[1];
        in_EAX = in_EAX + 1;
      } while (cVar1 != '\0');
      FUN_00557720();
      return;
    }
  }
  FUN_00557720();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
