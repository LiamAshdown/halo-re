// text_parse_next_token_wide  (Ghidra: FUN_00556f10; renamed here)
// address 0x556f10, size 125 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: phase 2 named this "text_parse_next_token_plain", but out/phase4/
//   text_types_notes.md corrects that: this tokenizer reads the string as 16-bit code
//   units (`*(undefined2*)(*(int*)(in_EAX+8)+sVar1*2)`), i.e. UTF-16, and is the wide
//   twin of the narrow, markup-aware text_parse_next_token_narrow (0x556bb0). It is the
//   tokenizer of the wide pipeline: wrap 0x556780 -> this -> draw range
//   text_draw_character_range_wide (0x5572b0); also used by text_measure_string_fit_width
//   (0x557530) and the extents probe 0x5562d0. Only NUL, tab, CR and the two-code-unit
//   "|n" escape are special; everything else -- including a lone '|' -- is an ordinary
//   character (token 6).
// register convention: EAX = state (in_EAX), no other arguments. Confirmed by
//   text_draw_character_range_wide's call site (0x5573e9 "lea eax,[esp+0x28]" then
//   "call 0x556f10") and out/phase4/text_types_notes.md's "0x556f10: EAX = state".

#include "tags.h"
#include "memory.h"
#include "text.h"

// blam-cc: EAX=state, no other arguments
// Reads the next 16-bit code unit from state->string at state->position, stores it in
// state->character, advances state->position by one code unit (two for the "|n" escape),
// classifies it into state->token and returns that token.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int16_t text_parse_next_token_wide(text_parse_state *state)
{
    uint16_t *string;
    int16_t position;
    uint16_t code;

    string = (uint16_t *)state->string;
    position = state->position;
    code = string[position];
    state->character = code;
    state->position = position + 1;

    switch (code) {
    case 0:
        state->token = _text_token_end;
        return state->token;
    case 9:
        state->token = _text_token_tab;
        return state->token;
    case 0xd:
        state->token = _text_token_newline;
        return state->token;
    case 0x7c: /* '|' */
        state->position = position + 2;
        if (string[position + 1] == 'n') {
            state->character = 0xd;
            state->token = _text_token_newline;
            return state->token;
        }
        state->position = position + 1;
        break;
    }
    state->token = _text_token_character;
    return state->token;
}

#if 0
Original Ghidra decompilation (0x556f10):

undefined2 FUN_00556f10(void)

{
  short sVar1;
  undefined2 uVar2;
  short sVar3;
  int in_EAX;

  sVar1 = *(short *)(in_EAX + 0xc);
  uVar2 = *(undefined2 *)(*(int *)(in_EAX + 8) + sVar1 * 2);
  *(undefined2 *)(in_EAX + 0x12) = uVar2;
  *(short *)(in_EAX + 0xc) = sVar1 + 1;
  switch(uVar2) {
  case 0:
    *(undefined2 *)(in_EAX + 0x14) = 0;
    return *(undefined2 *)(in_EAX + 0x14);
  case 9:
    *(undefined2 *)(in_EAX + 0x14) = 3;
    return *(undefined2 *)(in_EAX + 0x14);
  case 0xd:
switchD_00556f39_caseD_d:
    *(undefined2 *)(in_EAX + 0x14) = 1;
    return *(undefined2 *)(in_EAX + 0x14);
  case 0x7c:
    sVar3 = *(short *)(*(int *)(in_EAX + 8) + (short)(sVar1 + 1) * 2);
    *(short *)(in_EAX + 0xc) = sVar1 + 2;
    if (sVar3 == 0x6e) {
      *(undefined2 *)(in_EAX + 0x12) = 0xd;
      goto switchD_00556f39_caseD_d;
    }
    *(short *)(in_EAX + 0xc) = sVar1 + 1;
  }
  *(undefined2 *)(in_EAX + 0x14) = 6;
  return *(undefined2 *)(in_EAX + 0x14);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
