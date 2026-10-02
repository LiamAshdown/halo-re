// widget_text_edit_process_key  (Ghidra: FUN_0044c290, renamed per types/interface.h)
// address 0x44c290, size 785 bytes
// name confidence: 0.35   rewrite confidence: 0.55
// evidence: types/interface.h ui_key_event / text_edit_state comment group; out/phase4/
// interface_functions.md "Handles special edit-key codes on a widget's text buffer,
// inserting/deleting characters and shifting bytes with memmove while updating cursor/selection
// fields."
// register convention: state in EAX (in_EAX), event in the stack parameter Ghidra recognized
// (param_1). // blam-cc: state=EAX, event=stack param_1
// UNSURE: for _ui_edit_key_left_arrow/_ui_edit_key_right_arrow (0x4f/0x50) with no active selection, this
// function performs DBCS-safe backward/forward single-character cursor movement (snap to the
// nearest earlier character boundary, or advance exactly one character), not "jump to the start/
// end of the line". With an active selection and no shift, it collapses the selection to its
// near edge. That is Left/Right arrow handling, which R18 confirmed from the key table.
// UNSURE: the exact physical registers backing text_get_next_character / text_find_character_
// boundary / text_clamp_byte_length_to_character_boundary at each call site (see the note in
// widget_text_edit_clamp_selection.c); the (string, offset-pointer) pairing used below is
// inferred from each callee's own decompilation and from which call sites visibly reuse a
// stack slot (Ghidra's `&param_1` idiom) versus visibly address the real state->cursor field.
// reconciled: R18 key codes 0x4f/0x50 are left/right arrow (DIK table 0x0065bd58: DIK_LEFT -> 0x4f, DIK_RIGHT -> 0x50; home is 0x52, end 0x55): _ui_edit_key_home/_end -> _ui_edit_key_left_arrow/_right_arrow, same values, which matches the one-character movement below

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
extern uint32_t widget_text_edit_get_selection(text_edit_state *state, int16_t *out_start, int16_t *out_end); // 0x44c5e0, this module
extern void text_find_character_boundary(char *string, int16_t *offset_in_out); // 0x5576d0, module text (foreign): walks from byte 0, stepping one full (possibly double-byte) character at a time, until the running offset would reach *offset_in_out, then writes back the last complete boundary <= the original value.
extern uint16_t text_get_next_character(char *string, int16_t *offset_in_out); // 0x5576a0, module text (foreign): reads the (possibly double-byte) character at *offset_in_out and advances *offset_in_out past it.
extern void text_clamp_byte_length_to_character_boundary(char *string, int16_t *length); // 0x557720, module text (foreign)

// Applies one buffered key event to a widget's text-edit state: Home/End (collapse an existing
// selection to its near edge, or otherwise step the cursor one character backward/forward),
// Backspace/Delete (remove the selection, or the one character before/at the cursor), and plain
// printable characters (replace the selection, or insert at the cursor, honoring
// maximum_length). Every exit path re-clamps the cursor (and any surviving selection) off a
// double-byte character boundary.
void widget_text_edit_process_key(text_edit_state *state, ui_key_event *event)
{
    int16_t sel_start;
    int16_t sel_end;
    uint32_t has_selection;
    int16_t old_cursor;
    int16_t scratch_offset;
    char *src;
    char *dst;
    int32_t tail_len;

    widget_text_edit_clamp_selection(state);

    if (event->key_code != _ui_edit_key_backspace && event->key_code != _ui_edit_key_delete) {
        if (event->key_code == _ui_edit_key_left_arrow || event->key_code == _ui_edit_key_right_arrow) {
            has_selection = ((event->modifiers & 1) == 0) &&
                             widget_text_edit_get_selection(state, &sel_start, &sel_end);
            if (has_selection) {
                state->selection_anchor = -1;
                if (event->key_code != _ui_edit_key_left_arrow) {
                    state->cursor = sel_end;
                } else {
                    state->cursor = sel_start;
                }
                text_clamp_byte_length_to_character_boundary(state->text, &state->cursor);
                return;
            }

            if ((event->modifiers & 1) != 0 && state->selection_anchor == -1) {
                state->selection_anchor = state->cursor;
            }
            if (event->key_code == _ui_edit_key_left_arrow) {
                if (state->cursor > 0) {
                    text_find_character_boundary(state->text, &state->cursor);
                }
            } else {
                if ((size_t)state->cursor < strlen(state->text)) {
                    text_get_next_character(state->text, &state->cursor);
                }
            }
            if (state->selection_anchor == state->cursor) {
                state->selection_anchor = -1;
            }
        } else if (event->character > 0x1f && event->character != 0xff) {
            has_selection = widget_text_edit_get_selection(state, &sel_start, &sel_end);
            if (has_selection) {
                src = state->text + sel_end;
                tail_len = (int32_t)strlen(src);
                memmove(state->text + sel_start + 1, src, (size_t)(tail_len + 1));
                state->cursor = sel_start;
                state->selection_anchor = -1;
                state->text[sel_start] = (char)event->character;
                state->cursor = state->cursor + 1;
                text_clamp_byte_length_to_character_boundary(state->text, &state->cursor);
                return;
            }
            if ((int32_t)strlen(state->text) < (int32_t)state->maximum_length) {
                dst = state->text + state->cursor;
                tail_len = (int32_t)strlen(dst);
                memmove(dst + 1, dst, (size_t)(tail_len + 1));
                state->text[state->cursor] = (char)event->character;
                state->cursor = state->cursor + 1;
                text_clamp_byte_length_to_character_boundary(state->text, &state->cursor);
                return;
            }
        }
        text_clamp_byte_length_to_character_boundary(state->text, &state->cursor);
        return;
    }

    has_selection = widget_text_edit_get_selection(state, &sel_start, &sel_end);
    if (has_selection) {
        src = state->text + sel_end;
        tail_len = (int32_t)strlen(src);
        memmove(state->text + sel_start, src, (size_t)(tail_len + 1));
        state->cursor = sel_start;
        state->selection_anchor = -1;
        text_clamp_byte_length_to_character_boundary(state->text, &state->cursor);
        return;
    }

    if (event->key_code == _ui_edit_key_backspace) {
        old_cursor = state->cursor;
        if (old_cursor < 1) {
            text_clamp_byte_length_to_character_boundary(state->text, &state->cursor);
            return;
        }
        text_find_character_boundary(state->text, &state->cursor); // moves state->cursor back to the previous character boundary
        src = state->text + old_cursor;
        tail_len = (int32_t)strlen(src);
        dst = state->text + state->cursor;
        memmove(dst, src, (size_t)(tail_len + 1));
    } else {
        // event->key_code == _ui_edit_key_delete (the only other value reachable here)
        if ((size_t)state->cursor >= strlen(state->text)) {
            text_clamp_byte_length_to_character_boundary(state->text, &state->cursor);
            return;
        }
        scratch_offset = state->cursor;
        text_get_next_character(state->text, &scratch_offset); // advances scratch_offset past the deleted character; state->cursor itself is untouched
        src = state->text + scratch_offset;
        tail_len = (int32_t)strlen(src);
        dst = state->text + state->cursor;
        memmove(dst, src, (size_t)(tail_len + 1));
    }

    text_clamp_byte_length_to_character_boundary(state->text, &state->cursor);
}

#if 0
Original Ghidra decompilation (0x44c290):

void FUN_0044c290(byte *param_1)

{
  short sVar1;
  byte *pbVar2;
  char cVar3;
  int *in_EAX;
  char *pcVar4;
  int iVar5;
  void *_Dst;
  char *pcVar6;
  short local_8 [2];
  short local_4 [2];

  FUN_0044c780();
  pbVar2 = param_1;
  sVar1 = *(short *)(param_1 + 2);
  if ((sVar1 != 0x1d) && (sVar1 != 0x54)) {
    if ((sVar1 == 0x4f) || (sVar1 == 0x50)) {
      if (((*param_1 & 1) == 0) && (cVar3 = FUN_0044c5e0(&param_1,local_4), cVar3 != '\0')) {
        sVar1 = *(short *)(pbVar2 + 2);
        *(undefined2 *)(in_EAX + 2) = 0xffff;
        if (sVar1 != 0x4f) {
          *(short *)((int)in_EAX + 6) = local_4[0];
          FUN_00557720();
          return;
        }
        *(short *)((int)in_EAX + 6) = (short)param_1;
        FUN_00557720();
        return;
      }
      if (((*pbVar2 & 1) != 0) && ((short)in_EAX[2] == -1)) {
        *(undefined2 *)(in_EAX + 2) = *(undefined2 *)((int)in_EAX + 6);
      }
      if ((*(short *)(pbVar2 + 2) == 0x4f) && (0 < *(short *)((int)in_EAX + 6))) {
        FUN_005576d0(*in_EAX,(int)in_EAX + 6);
      }
      else if (*(short *)(pbVar2 + 2) == 0x50) {
        pcVar4 = (char *)*in_EAX;
        pcVar6 = pcVar4 + 1;
        do {
          cVar3 = *pcVar4;
          pcVar4 = pcVar4 + 1;
        } while (cVar3 != '\0');
        if ((uint)(int)*(short *)((int)in_EAX + 6) < (uint)((int)pcVar4 - (int)pcVar6)) {
          FUN_005576a0();
        }
      }
      if ((short)in_EAX[2] == *(short *)((int)in_EAX + 6)) {
        *(undefined2 *)(in_EAX + 2) = 0xffff;
        FUN_00557720();
        return;
      }
    }
    else if ((0x1f < param_1[1]) && (param_1[1] != 0xff)) {
      cVar3 = FUN_0044c5e0(local_4,local_8);
      if (cVar3 != '\0') {
        pcVar6 = (char *)((int)local_8[0] + *in_EAX);
        pcVar4 = pcVar6;
        do {
          cVar3 = *pcVar4;
          pcVar4 = pcVar4 + 1;
        } while (cVar3 != '\0');
        _memmove((void *)(local_4[0] + 1 + *in_EAX),pcVar6,
                 (size_t)(pcVar4 + (1 - (int)(pcVar6 + 1))));
        *(short *)((int)in_EAX + 6) = local_4[0];
        *(undefined2 *)(in_EAX + 2) = 0xffff;
        *(byte *)((int)local_4[0] + *in_EAX) = param_1[1];
        *(short *)((int)in_EAX + 6) = *(short *)((int)in_EAX + 6) + 1;
        FUN_00557720();
        return;
      }
      pcVar6 = (char *)*in_EAX;
      pcVar4 = pcVar6;
      do {
        cVar3 = *pcVar4;
        pcVar4 = pcVar4 + 1;
      } while (cVar3 != '\0');
      if ((uint)((int)pcVar4 - (int)(pcVar6 + 1)) < (uint)(int)(short)in_EAX[1]) {
        pcVar6 = pcVar6 + *(short *)((int)in_EAX + 6);
        pcVar4 = pcVar6;
        do {
          cVar3 = *pcVar4;
          pcVar4 = pcVar4 + 1;
        } while (cVar3 != '\0');
        _memmove(pcVar6 + 1,pcVar6,(size_t)(pcVar4 + (1 - (int)(pcVar6 + 1))));
        *(byte *)((int)*(short *)((int)in_EAX + 6) + *in_EAX) = pbVar2[1];
        *(short *)((int)in_EAX + 6) = *(short *)((int)in_EAX + 6) + 1;
        FUN_00557720();
        return;
      }
    }
    goto LAB_0044c58f;
  }
  cVar3 = FUN_0044c5e0(local_4,&param_1);
  if (cVar3 != '\0') {
    pcVar6 = (char *)((int)(short)param_1 + *in_EAX);
    pcVar4 = pcVar6;
    do {
      cVar3 = *pcVar4;
      pcVar4 = pcVar4 + 1;
    } while (cVar3 != '\0');
    _memmove((void *)((int)local_4[0] + *in_EAX),pcVar6,(size_t)(pcVar4 + (1 - (int)(pcVar6 + 1))));
    *(short *)((int)in_EAX + 6) = local_4[0];
    *(undefined2 *)(in_EAX + 2) = 0xffff;
    goto LAB_0044c58f;
  }
  if (*(short *)(pbVar2 + 2) == 0x1d) {
    sVar1 = *(short *)((int)in_EAX + 6);
    if (sVar1 < 1) goto LAB_0044c535;
    FUN_005576d0(*in_EAX,(short *)((int)in_EAX + 6));
    pcVar6 = (char *)((int)sVar1 + *in_EAX);
    pcVar4 = pcVar6;
    do {
      cVar3 = *pcVar4;
      pcVar4 = pcVar4 + 1;
    } while (cVar3 != '\0');
    iVar5 = (int)pcVar4 - (int)(pcVar6 + 1);
    _Dst = (void *)((int)*(short *)((int)in_EAX + 6) + *in_EAX);
  }
  else {
LAB_0044c535:
    if (*(short *)(pbVar2 + 2) != 0x54) goto LAB_0044c58f;
    pcVar4 = (char *)*in_EAX;
    pcVar6 = pcVar4 + 1;
    do {
      cVar3 = *pcVar4;
      pcVar4 = pcVar4 + 1;
    } while (cVar3 != '\0');
    if ((uint)((int)pcVar4 - (int)pcVar6) <= (uint)(int)(short)*(ushort *)((int)in_EAX + 6))
    goto LAB_0044c58f;
    param_1 = (byte *)(uint)*(ushort *)((int)in_EAX + 6);
    FUN_005576a0();
    pcVar6 = (char *)((int)(short)param_1 + *in_EAX);
    pcVar4 = pcVar6;
    do {
      cVar3 = *pcVar4;
      pcVar4 = pcVar4 + 1;
    } while (cVar3 != '\0');
    iVar5 = (int)pcVar4 - (int)(pcVar6 + 1);
    _Dst = (void *)((int)*(short *)((int)in_EAX + 6) + *in_EAX);
  }
  _memmove(_Dst,pcVar6,iVar5 + 1);
LAB_0044c58f:
  FUN_00557720();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
