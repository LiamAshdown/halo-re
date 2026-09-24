// text_measure_string_fit_width  (Ghidra: text_measure_string_fit_width, already named)
// address 0x557530, size 245 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/text_types_notes.md: "0x557530: ECX = UTF-16 string (passed
//   straight through to 0x556b00), stack = int *max_width". Drives the wide tokenizer
//   text_parse_next_token_wide (0x556f10) in a loop, accumulating each character's
//   advance width (FontCharacter+0x02) while its rendered footprint
//   (FontCharacter+0x04, bitmap_width) still fits; returns the last non-glyph token's
//   position (newline/break/tab/justification) as the break column once it would not,
//   and reduces *max_width_inout by the width actually consumed either way.
// register convention: objdump 0x557530..0x557650 shows no instruction loads ECX before
//   "call 0x556b00" at 0x55755d -- ECX is a pure pass-through from this function's own
//   caller, matching the notes. EAX/EDX/EBX are loaded from the text_font/
//   text_justification_state/text_style_state globals (same as every other text_parse_state_initialize
//   call site), not from registers this function receives. The stack argument
//   max_width_inout sits at [esp+0x2c] (mov ebp,[esp+0x2c] at 0x557547).

#include "tags.h"
#include "memory.h"
#include "text.h"

extern datum_index text_font;              // 0x006e472c
extern int16_t text_style_state;                 // 0x006e4734
extern int16_t text_justification_state;         // 0x006e4736
extern ColorARGB text_color;               // 0x006e4738

// blam-cc: ECX=string, EDX=justification, EBX=style, ESI=state, stack=(font, color)
extern void text_parse_state_initialize(void *string, int16_t justification, int16_t style,
    text_parse_state *state, datum_index font, ColorARGB *color); // 0x556b00

// blam-cc: EAX=state, no other arguments
extern int16_t text_parse_next_token_wide(text_parse_state *state); // 0x556f10

// blam-cc: string in ECX (a pure pass-through, never read here), max_width_inout on the stack
// Walks string (UTF-16, via the wide tokenizer) accumulating glyph advance widths until the
// next glyph's rendered footprint would not fit within *max_width_inout, or the string ends.
// Reduces *max_width_inout by the width actually consumed and returns the column (string
// position) at which the text may break: the position just after the last committed
// newline/break-character/tab/justification token or ordinary glyph.
int32_t text_measure_string_fit_width(void *string, int32_t *max_width_inout)
{
    text_parse_state state;
    int32_t consumed_width;
    int32_t break_column;

    consumed_width = 0;
    break_column = 0;
    text_parse_state_initialize(string, text_justification_state, text_style_state, &state, text_font,
        &text_color);

    for (;;) {
        text_parse_next_token_wide(&state);
        switch (state.token) {
        case _text_token_end:
            *max_width_inout -= consumed_width;
            return 0;
        case _text_token_newline:
        case _text_token_break_character:
        case _text_token_tab:
        case _text_token_justification:
        case _text_token_unused_5:
            break_column = state.position;
            break;
        case _text_token_character: {
            Font *font;
            FontCharacterTables *page;

            font = (Font *)state.font_definition;
            page = (FontCharacterTables *)font->character_tables.pointer +
                (state.character >> 8);
            if (0 < (int32_t)page->character_table.count) {
                // a page whose count is not 0x100 reads through NULL, as in the binary
                int16_t *character_index = (page->character_table.count ==
                    k_text_font_character_table_page_size)
                        ? (int16_t *)page->character_table.pointer + (state.character & 0xff)
                        : (int16_t *)0;
                int16_t hardware_index = *character_index;
                if (hardware_index != -1) {
                    FontCharacter *character = (FontCharacter *)font->characters.pointer +
                        hardware_index;
                    if (character != (void *)0) {
                        if (*max_width_inout <= (int32_t)character->bitmap_width + consumed_width) {
                            *max_width_inout -= consumed_width;
                            return break_column;
                        }
                        break_column = state.position;
                        consumed_width += character->character_width;
                    }
                }
            }
            break;
        }
        default:
            break; // token > 6: never produced by text_parse_next_token_wide; loop again
        }
    }
}

#if 0
Original Ghidra decompilation (0x557530):

int __cdecl text_measure_string_fit_width(int *max_width_inout)

{
  int *piVar1;
  int iVar2;
  short *psVar3;
  int iVar4;
  int local_20;
  int local_18;
  short local_10;
  ushort local_a;
  undefined2 local_8;

  iVar4 = 0;
  local_20 = 0;
  FUN_00556b00(DAT_006e472c,&DAT_006e4738);
  do {
    FUN_00556f10();
    switch(local_8) {
    case 0:
      *max_width_inout = *max_width_inout - iVar4;
      return 0;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
      local_20 = (int)local_10;
      break;
    case 6:
      piVar1 = (int *)(*(int *)(local_18 + 0x34) + (uint)(local_a >> 8) * 0xc);
      iVar2 = *piVar1;
      if (0 < iVar2) {
        if (iVar2 == 0x100) {
          psVar3 = (short *)(piVar1[1] + (local_a & 0xff) * 2);
        }
        else {
          psVar3 = (short *)0x0;
        }
        if ((*psVar3 != -1) && (iVar2 = *(int *)(local_18 + 0x80) + *psVar3 * 0x14, iVar2 != 0)) {
          if (*max_width_inout <= *(short *)(iVar2 + 4) + iVar4) {
            *max_width_inout = *max_width_inout - iVar4;
            return local_20;
          }
          local_20 = (int)local_10;
          iVar4 = iVar4 + *(short *)(iVar2 + 2);
        }
      }
    }
  } while( true );
}
#endif
