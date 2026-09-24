// text_wrap_and_draw_wide  (Ghidra: FUN_00556780, renamed)
// address 0x556780, size 863 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// review (phase 4 gate): objdump 0x556780..0x556b00 diffed against the narrow twin; it
//   differs only in register allocation, the tokenizer/draw callees and a re-read of
//   text_tab_stop_count. The same two fixes as text_wrap_and_draw_narrow.c were applied:
//   the glyph lookup (0x55691f / 0x556921) and the sar centring.
// evidence: out/phase4/text_types_notes.md names this pipeline "wide (UTF-16 code
//   units, only |n recognised): wrap 0x556780 -> tokenizer 0x556f10 -> draw range
//   0x5572b0", matching this rewrite's own text_parse_next_token_wide.c /
//   text_draw_character_range_wide.c (0x556f10 / 0x5572b0). Byte-for-byte the same
//   word-wrap algorithm as text_wrap_and_draw_narrow (0x556400, the narrow twin of
//   this function): confirmed via objdump -d -M intel over 0x556780..0x556b00, same
//   frame size (sub esp,0x58 + 4 pushes = 0x68, text_parse_state at [esp+0x4c]), same
//   tab-stop / line-indent / word-wrap-backtrack / justification / multi-row-line
//   bookkeeping, only the tokenizer and draw callees (and the register they hand the
//   state pointer through) differ. See text_wrap_and_draw_narrow.c for the derivation
//   of the local bookkeeping variables' names and the UNSURE notes that apply equally
//   here.
// register convention: no register-passed arguments; all six on the stack: callback,
//   bounds Rectangle2D*, out Point2DInt* final pen, clip Rectangle2D*, int16 extra
//   line spacing, string. text_parse_state_initialize (0x556b00) is called with ESI=&state,
//   ECX=string, DX=text_justification_state, BX=text_style_state, stack=(text_font,
//   &text_color) -- identical to text_wrap_and_draw_narrow. text_parse_next_token_wide
//   (0x556f10) is called with EAX=&state (not EDI, unlike the narrow tokenizer).
//   text_draw_character_range_wide (0x5572b0) is called with EAX=&line_bounds (the
//   tab-adjusted local Rectangle2D copy), stack = (callback, &pen, clip, state.color,
//   string, start_column, end_column).

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "text.h"

extern tag_instance *tag_instances;                  // 0x0087bc14
extern datum_index text_font;                         // 0x006e472c
extern ColorARGB text_color;                           // 0x006e4738
extern int16_t text_style_state;                       // 0x006e4734
extern int16_t text_justification_state;                 // 0x006e4736
extern int16_t text_tab_stop_count;                       // 0x006e4748
extern int16_t text_tab_stops[k_text_maximum_tab_stops];   // 0x006e474a
extern uint32_t text_flags_state;                            // 0x006e4730
extern int16_t text_highlight_start;                          // 0x006e476a
extern int16_t text_highlight_end;                             // 0x006e476c
extern int16_t text_first_line_indent;                          // 0x006e476e
extern int16_t text_wrapped_line_indent;                         // 0x006e4770

extern void text_parse_state_initialize(void *string, int16_t justification, int16_t style,
    text_parse_state *state, datum_index font, ColorARGB *color); // 0x556b00
extern int16_t text_parse_next_token_wide(text_parse_state *state); // 0x556f10
extern void text_draw_character_range_wide(Rectangle2D *bounds, text_glyph_draw_proc callback,
    Point2DInt *pen, Rectangle2D *clip, uint32_t color, void *string,
    int16_t start_column, int16_t end_column); // 0x5572b0

// blam-cc: stack -> callback, bounds, out_final_pen, clip, extra_line_spacing, string
void text_wrap_and_draw_wide(text_glyph_draw_proc callback, Rectangle2D *bounds,
    Point2DInt *out_final_pen, Rectangle2D *clip, int16_t extra_line_spacing, void *string)
{
    text_parse_state state;
    Font *font;
    int16_t tab_index, line_index, max_wrapped_sub_line_count, wrapped_sub_line_count;
    Point2DInt pen;
    Rectangle2D line_bounds;

    tab_index = 0;
    line_index = 0;
    wrapped_sub_line_count = 0;
    max_wrapped_sub_line_count = 0;

    text_parse_state_initialize(string, text_justification_state, text_style_state, &state,
        text_font, &text_color);
    font = (Font *)state.font_definition;

    for (;;) {
        int16_t span_start_position = state.position;
        int16_t span_justification = state.justification;
        int16_t span_width = 0;
        // candidate_position is a "not set yet" sentinel at 0: word-wrap backs up to
        // it only once a break character has actually been seen (state.position is
        // always > 0 once any character has been read).
        int16_t candidate_position = 0;
        int16_t candidate_width = 0;
        int16_t flush_end_position = 0;
        int16_t token, previous_token;
        int16_t pen_y;

        line_bounds = *bounds;

        if (text_tab_stop_count < 1) {
            line_bounds.left = (int16_t)(line_bounds.left +
                (line_index == 0 ? text_first_line_indent : text_wrapped_line_indent));
        } else if (tab_index == 0) {
            line_bounds.left = (int16_t)(line_bounds.left +
                (line_index == 0 ? text_first_line_indent : text_wrapped_line_indent));
            if (tab_index < text_tab_stop_count) {
                line_bounds.right = text_tab_stops[tab_index];
            }
        } else {
            // tab_index >= 1: text_tab_stop_count immediately precedes text_tab_stops[]
            // in memory (0x006e4748 then 0x006e474a with no gap), so indexing from
            // &text_tab_stop_count by tab_index reads text_tab_stops[tab_index - 1].
            line_bounds.left = (&text_tab_stop_count)[tab_index];
            if (tab_index < text_tab_stop_count) {
                line_bounds.right = text_tab_stops[tab_index];
            }
        }

        pen_y = (int16_t)((font->ascending_height + font->descending_height +
            font->leading_height + extra_line_spacing) * (wrapped_sub_line_count + line_index) +
            font->ascending_height + bounds->top);
        pen.x = (int16_t)(font->leading_width + line_bounds.left);
        pen.y = pen_y;

        previous_token = -1;
        for (;;) {
            int wrapped = 0;

            token = text_parse_next_token_wide(&state);
            font = (Font *)state.font_definition;

            if (token == _text_token_break_character || token == _text_token_character) {
                FontCharacterTables *page = (FontCharacterTables *)font->character_tables.pointer +
                    (state.character >> 8);
                if (page->character_table.count >= 1) {
                    // UNSURE: when count is nonzero but not 0x100, character_index is NULL
                    // and gets dereferenced below anyway, matching the original
                    // (0x55691f xor eax,eax / 0x556921 mov ax,[eax]).
                    int16_t *character_index = (page->character_table.count ==
                        k_text_font_character_table_page_size)
                            ? (int16_t *)page->character_table.pointer + (state.character & 0xff)
                            : (int16_t *)0;
                    if (*character_index != -1) {
                        FontCharacter *character = (FontCharacter *)font->characters.pointer +
                            *character_index;
                        if (character != (FontCharacter *)0) {
                            if (token != _text_token_break_character &&
                                previous_token == _text_token_break_character) {
                                candidate_position = flush_end_position;
                                candidate_width = span_width;
                            }
                            if (character->bitmap_width + pen.x + span_width < line_bounds.right) {
                                // The glyph fits: keep accumulating this span and read
                                // the next character.
                                span_width = (int16_t)(span_width + character->character_width);
                                flush_end_position = state.position;
                                previous_token = token;
                                continue;
                            }
                            if ((text_flags_state & _text_flag_word_wrap_bit) != 0) {
                                // Word wrap is enabled: back up to the last break
                                // character if one was seen, otherwise cut right here.
                                if (candidate_position > 0) {
                                    flush_end_position = candidate_position;
                                    span_width = candidate_width;
                                } else {
                                    flush_end_position = state.position;
                                }
                                previous_token = token;
                                wrapped = 1;
                            }
                            // else: word wrap disabled -- the line runs on past the
                            // edge; fall through and keep scanning.
                        }
                    }
                }
            } else {
                flush_end_position = state.position;
                previous_token = token;
                wrapped = 1;
            }

            if (wrapped) {
                break;
            }
            // Every other path (invalid page/character, or an overflowing glyph with
            // word wrap disabled) just tracks how far we've read and keeps scanning.
            flush_end_position = state.position;
            previous_token = token;
        }

        if (span_justification == _text_justification_right) {
            pen.x = (int16_t)(line_bounds.right - font->leading_width - span_width);
        } else if (span_justification == _text_justification_center) {
            // sar, not idiv: an odd negative remainder rounds toward -infinity
            pen.x = (int16_t)((((int16_t)(line_bounds.right - line_bounds.left) - span_width) >> 1) +
                line_bounds.left);
        }

        if ((text_flags_state & _text_flag_draw_past_bottom_bit) != 0 || pen_y < bounds->bottom) {
            text_draw_character_range_wide(&line_bounds, callback, &pen, clip, state.color,
                string, span_start_position, flush_end_position);
        }

        state.position = flush_end_position;

        switch (token) {
        case _text_token_newline:
            tab_index = 0;
            wrapped_sub_line_count = 0;
            line_index = (int16_t)(line_index + 1 + max_wrapped_sub_line_count);
            break;
        case _text_token_break_character:
        case _text_token_character:
            wrapped_sub_line_count = (int16_t)(wrapped_sub_line_count + 1);
            if (max_wrapped_sub_line_count < wrapped_sub_line_count) {
                max_wrapped_sub_line_count = wrapped_sub_line_count;
            }
            break;
        case _text_token_tab:
            if (tab_index < text_tab_stop_count) {
                tab_index = (int16_t)(tab_index + 1);
                wrapped_sub_line_count = 0;
            }
            break;
        case _text_token_justification:
            wrapped_sub_line_count = 0;
            break;
        default:
            break;
        }

        if (token == _text_token_end) {
            text_highlight_end = 0;
            text_highlight_start = 0;
            if (out_final_pen != (Point2DInt *)0) {
                *out_final_pen = pen;
            }
            return;
        }
    }
}

#if 0
Original Ghidra decompilation (0x556780):

void FUN_00556780(undefined4 param_1,undefined4 *param_2,undefined4 *param_3,undefined4 param_4,
                 short param_5,undefined4 param_6)

{
  int *piVar1;
  int iVar2;
  bool bVar3;
  bool bVar4;
  short *psVar5;
  short sVar6;
  int iVar7;
  short sVar8;
  undefined4 uVar9;
  undefined4 local_54;
  int local_50;
  undefined4 local_4c;
  int local_48;
  int local_44;
  undefined4 local_40;
  int local_3c;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_2c;
  short local_28;
  short sStack_26;
  short local_24;
  short sStack_22;
  int local_18;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  local_48 = 0;
  local_44 = 0;
  local_50 = 0;
  local_3c = 0;
  FUN_00556b00(DAT_006e472c,&DAT_006e4738);
  iVar7 = local_18;
  do {
    local_2c = local_10;
    local_30 = local_c;
    local_28 = (short)*param_2;
    sStack_26 = (short)((uint)*param_2 >> 0x10);
    local_54 = 0;
    local_38 = 0;
    bVar4 = false;
    local_24 = (short)param_2[1];
    sStack_22 = (short)((uint)param_2[1] >> 0x10);
    if (DAT_006e4748 < 1) {
      if ((short)local_44 == 0) {
        sStack_26 = sStack_26 + DAT_006e476e;
      }
      else {
        sStack_26 = sStack_26 + DAT_006e4770;
      }
    }
    else {
      sVar8 = (short)local_48;
      if (sVar8 == 0) {
        sVar6 = DAT_006e4770;
        if ((short)local_44 == 0) {
          sVar6 = DAT_006e476e;
        }
        sStack_26 = sStack_26 + sVar6;
      }
      else {
        sStack_26 = (&DAT_006e4748)[sVar8];
      }
      if (sVar8 < DAT_006e4748) {
        sStack_22 = *(short *)(&DAT_006e474a + sVar8 * 2);
      }
    }
    sVar8 = (*(short *)(iVar7 + 8) + *(short *)(iVar7 + 6) + *(short *)(iVar7 + 4) + param_5) *
            ((short)local_50 + (short)local_44) + *(short *)(iVar7 + 4) + local_28;
    local_4c = CONCAT22(sVar8,*(short *)(iVar7 + 10) + sStack_26);
    uVar9 = 0xffffffff;
    do {
      FUN_00556f10();
      iVar7 = local_18;
      if (((short)local_8 == 2) || ((short)local_8 == 6)) {
        piVar1 = (int *)(*(int *)(local_18 + 0x34) + (uint)(local_c._2_2_ >> 8) * 0xc);
        iVar2 = *piVar1;
        if (iVar2 < 1) goto LAB_005569b1;
        if (iVar2 == 0x100) {
          psVar5 = (short *)(piVar1[1] + (local_c._2_2_ & 0xff) * 2);
        }
        else {
          psVar5 = (short *)0x0;
        }
        if ((*psVar5 == -1) || (iVar2 = *(int *)(local_18 + 0x80) + *psVar5 * 0x14, iVar2 == 0))
        goto LAB_005569b1;
        if (((short)local_8 != 2) && ((short)uVar9 == 2)) {
          local_38 = local_40;
          local_34 = local_54;
        }
        if ((int)*(short *)(iVar2 + 4) + (int)(short)local_4c + (int)(short)local_54 <
            (int)sStack_22) {
          local_54 = CONCAT22(local_54._2_2_,(short)local_54 + *(short *)(iVar2 + 2));
          goto LAB_005569b1;
        }
        if ((DAT_006e4730 & 1) == 0) goto LAB_005569b1;
        bVar3 = 0 < (short)local_38;
        if (bVar3) {
          local_40 = local_38;
          local_54 = local_34;
        }
        bVar4 = true;
        if (!bVar3) goto LAB_005569b1;
      }
      else {
        bVar4 = true;
LAB_005569b1:
        local_40 = local_10;
      }
      uVar9 = local_8;
    } while (!bVar4);
    if ((short)local_30 == 1) {
      local_4c = CONCAT22(local_4c._2_2_,(sStack_22 - *(short *)(local_18 + 10)) - (short)local_54);
    }
    else if ((short)local_30 == 2) {
      local_4c = CONCAT22(local_4c._2_2_,
                          (short)((int)(short)(sStack_22 - sStack_26) - (int)(short)local_54 >> 1) +
                          sStack_26);
    }
    if (((DAT_006e4730 & 2) != 0) || (sVar8 < local_24)) {
      FUN_005572b0(param_1,&local_4c,param_4,local_4,param_6,local_2c,local_40);
    }
    local_10 = CONCAT22(local_10._2_2_,(undefined2)local_40);
    switch((short)local_8) {
    case 1:
      local_48 = 0;
      local_50 = 0;
      local_44 = local_44 + 1 + local_3c;
      break;
    case 2:
    case 6:
      local_50 = local_50 + 1;
      if ((short)local_3c < (short)local_50) {
        local_3c = local_50;
      }
      break;
    case 3:
      if ((short)local_48 < DAT_006e4748) {
        local_48 = local_48 + 1;
        goto switchD_00556a66_caseD_4;
      }
      break;
    case 4:
switchD_00556a66_caseD_4:
      local_50 = 0;
    }
    if ((short)local_8 == 0) {
      DAT_006e476c = 0;
      DAT_006e476a = 0;
      if (param_3 != (undefined4 *)0x0) {
        *param_3 = local_4c;
      }
      return;
    }
  } while( true );
}

-- objdump -d -M intel excerpt confirming the register/stack layout and that the only
-- structural differences from text_wrap_and_draw_narrow (0x556400) are the tokenizer
-- (0x556f10, EAX=&state) and draw (0x5572b0) callees: see 0x556780..0x556ae7 in
-- bin/halo.exe and out/phase4/text_types_notes.md.
#endif
