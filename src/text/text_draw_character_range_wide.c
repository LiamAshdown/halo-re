// text_draw_character_range_wide  (Ghidra: FUN_005572b0; renamed here)
// address 0x5572b0, size 626 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// review (phase 4 gate): objdump 0x5572b0..0x557521 diffed against the narrow twin;
//   the only differences are register allocation and the tokenizer. It got the same
//   16-bit clip math fix as text_draw_character_range_narrow.c.
// evidence: byte-for-byte the same clip/intersection and highlight-XOR structure as
//   text_draw_character_range_narrow (0x557030; objdump 0x5572b0..0x557530 matches
//   0x557030..0x5572b0 instruction for instruction up to the tokenizer call), but it
//   resolves each column through text_parse_next_token_wide (0x556f10, the UTF-16
//   tokenizer) instead of the narrow, markup-aware one -- out/phase4/text_types_notes.md
//   calls this pairing "narrow" vs "wide", correcting the phase 2 "plain" name.
// register convention: EAX = bounds Rectangle2D*. Stack args, in order: callback, pen
//   Point2DInt*, clip Rectangle2D*, color, string (dropped by Ghidra as param_5, reaches
//   text_parse_state_initialize only through ECX). text_parse_state_initialize (0x556b00) is called
//   with ESI=&state, ECX=string, DX=hud_text_draw_column, BX=hud_text_draw_color_or_flags, stack=(hud_text_draw_font_tag_id,
//   &hud_text_draw_color_a) -- confirmed at 0x557399 "lea esi,[esp+0x30]" through 0x55739d
//   "call 0x556b00". text_parse_next_token_wide (0x556f10) is called with EAX=&state
//   (confirmed at 0x5573e9 "lea eax,[esp+0x28]"), matching
//   out/phase4/text_types_notes.md's "0x556f10: EAX = state".
//   The glyph callback is invoked as (*callback)(&state, font, character, color, draw_x,
//   draw_y, source_x, source_y, width, height), matching text_glyph_draw_proc in
//   types/text.h exactly (same 10-push cdecl call as the narrow twin).

#include "tags.h"
#include "memory.h"
#include "text.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern datum_index hud_text_draw_font_tag_id;              // 0x006e472c
extern int16_t hud_text_draw_color_or_flags;                 // 0x006e4734
extern int16_t hud_text_draw_column;         // 0x006e4736
extern ColorARGB hud_text_draw_color_a;               // 0x006e4738
extern int16_t text_highlight_start;       // 0x006e476a, first column drawn with inverted colour
extern int16_t text_highlight_end;         // 0x006e476c, one past the last such column

// blam-cc: ECX=string, EDX=justification, EBX=style, ESI=state, stack=(font, color)
extern void text_parse_state_initialize(void *string, int16_t justification, int16_t style,
    text_parse_state *state, datum_index font, ColorARGB *color); // 0x556b00

// blam-cc: EAX=state, no other arguments
extern int16_t text_parse_next_token_wide(text_parse_state *state); // 0x556f10

// Draws the wide (UTF-16 code unit) text columns [start_column, end_column) of the
// current parse string through callback, one glyph at a time, clipped to the
// intersection of bounds and clip. Columns inside [text_highlight_start, text_highlight_end)
// are drawn with color inverted (XOR 0xffffff). pen->x is advanced by each glyph's
// character_width even when the glyph itself is not visible or not drawn. Structurally
// identical to text_draw_character_range_narrow; see that file for the clip-math proof.
void text_draw_character_range_wide(Rectangle2D *bounds, text_glyph_draw_proc callback,
    Point2DInt *pen, Rectangle2D *clip, uint32_t color, void *string,
    int16_t start_column, int16_t end_column)
{
    text_parse_state state;
    // every clip bound is compared and subtracted as a 16-bit value in the binary
    int16_t left, right, top, bottom;

    left = -0x8000;
    right = 0x7fff;
    top = -0x8000;
    bottom = 0x7fff;
    if (bounds != (void *)0) {
        left = bounds->left;
        right = bounds->right;
        top = bounds->top;
        bottom = bounds->bottom;
    }
    if (clip != (void *)0) {
        if (clip->left > left) left = clip->left;
        if (clip->right < right) right = clip->right;
        if (clip->top > top) top = clip->top;
        if (clip->bottom < bottom) bottom = clip->bottom;
    }

    if (left < right && top < bottom) {
        text_parse_state_initialize(string, hud_text_draw_column, hud_text_draw_color_or_flags, &state, hud_text_draw_font_tag_id,
            &hud_text_draw_color_a);
        state.position = start_column;
        while (state.position < end_column) {
            uint32_t glyph_color;
            Font *font;
            FontCharacterTables *page;
            int16_t hardware_index;
            FontCharacter *character;

            glyph_color = (state.position < text_highlight_start ||
                           text_highlight_end <= state.position)
                              ? color : (color ^ 0xffffff);

            text_parse_next_token_wide(&state);

            font = (Font *)state.font_definition;
            page = (FontCharacterTables *)font->character_tables.pointer + (state.character >> 8);
            if (0 < (int32_t)page->character_table.count) {
                // a page whose count is not 0x100 reads through NULL, as in the binary
                int16_t *character_index = (page->character_table.count ==
                    k_text_font_character_table_page_size)
                        ? (int16_t *)page->character_table.pointer + (state.character & 0xff)
                        : (int16_t *)0;
                hardware_index = *character_index;
                if (hardware_index != -1) {
                    character = (FontCharacter *)font->characters.pointer + hardware_index;
                    if (character != (void *)0) {
                        int16_t draw_x, draw_y;
                        // 16-bit in the binary: the > 0 tests are test si,si / test bx,bx
                        int16_t source_x, source_y, width, height;

                        draw_x = (int16_t)(pen->x - character->bitmap_origin_x);
                        draw_y = (int16_t)(pen->y - character->bitmap_origin_y);
                        pen->x = (int16_t)(pen->x + character->character_width);

                        source_x = 0;
                        source_y = 0;
                        width = character->bitmap_width;
                        height = character->bitmap_height;

                        if ((int32_t)draw_x + (int32_t)character->bitmap_width > right) {
                            width = (int16_t)(right - draw_x);
                        }
                        if (draw_x < left) {
                            source_x = (int16_t)(left - draw_x);
                            width = (int16_t)(width - source_x);
                            draw_x = left;
                        }

                        if ((int32_t)draw_y + (int32_t)character->bitmap_height > bottom) {
                            height = (int16_t)(bottom - draw_y);
                        }
                        if (draw_y < top) {
                            source_y = (int16_t)(top - draw_y);
                            height = (int16_t)(height - source_y);
                            draw_y = top;
                        }

                        if (0 < width && 0 < height) {
                            callback(&state, font, character, glyph_color, draw_x, draw_y,
                                source_x, source_y, width, height);
                        }
                    }
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x5572b0):

void FUN_005572b0(code *param_1,short *param_2,ushort *param_3,uint param_4,undefined4 param_5,
                 short param_6,short param_7)

{
  int *piVar1;
  ushort uVar2;
  int iVar3;
  ushort *puVar4;
  ushort *in_EAX;
  short *psVar5;
  short sVar6;
  ushort uVar8;
  short sVar9;
  ushort uVar11;
  int iVar12;
  ushort uVar13;
  uint uVar14;
  ushort uVar15;
  uint uVar16;
  uint local_34;
  uint local_30;
  uint local_2c;
  int local_28;
  uint local_24;
  undefined1 local_1c [4];
  int local_18;
  short local_10;
  ushort local_a;
  uint uVar7;
  uint uVar10;

  puVar4 = param_3;
  uVar13 = 0x8000;
  uVar8 = 0x7fff;
  uVar11 = 0x8000;
  local_30 = 0xffff8000;
  local_2c = 0xffff8000;
  param_3 = (ushort *)0x7fff;
  local_34 = 0x7fff;
  uVar15 = 0x7fff;
  if (in_EAX != (ushort *)0x0) {
    uVar8 = in_EAX[1];
    uVar13 = 0x8000;
    if (-0x8000 < (short)uVar8) {
      local_30 = (uint)uVar8;
      uVar13 = uVar8;
    }
    uVar11 = in_EAX[3];
    uVar8 = 0x7fff;
    if ((short)uVar11 < 0x7fff) {
      param_3 = (ushort *)(uint)uVar11;
      uVar8 = uVar11;
    }
    uVar2 = *in_EAX;
    uVar11 = 0x8000;
    if (-0x8000 < (short)uVar2) {
      local_2c = (uint)uVar2;
      uVar11 = uVar2;
    }
    uVar2 = in_EAX[2];
    if ((short)uVar2 < 0x7fff) {
      local_34 = (int)(short)uVar2;
      uVar15 = uVar2;
    }
  }
  if (puVar4 != (ushort *)0x0) {
    uVar2 = puVar4[1];
    if ((short)uVar13 < (short)uVar2) {
      uVar13 = uVar2;
      local_30 = (uint)uVar2;
    }
    uVar2 = puVar4[3];
    if ((short)uVar2 < (short)uVar8) {
      uVar8 = uVar2;
      param_3 = (ushort *)(uint)uVar2;
    }
    uVar2 = *puVar4;
    if ((short)uVar11 < (short)uVar2) {
      uVar11 = uVar2;
      local_2c = (uint)uVar2;
    }
    uVar2 = puVar4[2];
    if ((short)uVar2 < (short)uVar15) {
      local_34 = (uint)uVar2;
      uVar15 = uVar2;
    }
  }
  if (((short)uVar13 < (short)uVar8) && ((short)uVar11 < (short)uVar15)) {
    FUN_00556b00(DAT_006e472c,&DAT_006e4738);
    local_10 = param_6;
    while (local_10 < param_7) {
      if ((local_10 < DAT_006e476a) || (DAT_006e476c <= local_10)) {
        local_24 = param_4;
      }
      else {
        local_24 = param_4 ^ 0xffffff;
      }
      FUN_00556f10();
      piVar1 = (int *)(*(int *)(local_18 + 0x34) + (uint)(local_a >> 8) * 0xc);
      iVar3 = *piVar1;
      if (0 < iVar3) {
        if (iVar3 == 0x100) {
          psVar5 = (short *)(piVar1[1] + (local_a & 0xff) * 2);
        }
        else {
          psVar5 = (short *)0x0;
        }
        if (*psVar5 != -1) {
          iVar3 = *(int *)(local_18 + 0x80) + *psVar5 * 0x14;
          if (iVar3 != 0) {
            sVar9 = param_2[1] - *(short *)(iVar3 + 10);
            uVar10 = CONCAT22((short)((uint)param_2 >> 0x10),sVar9);
            uVar13 = *(ushort *)(iVar3 + 4);
            uVar14 = (uint)uVar13;
            sVar6 = *param_2 - *(short *)(iVar3 + 8);
            uVar7 = CONCAT22((short)((uint)*(int *)(local_18 + 0x80) >> 0x10),sVar6);
            local_28 = 0;
            uVar8 = *(ushort *)(iVar3 + 6);
            uVar16 = (uint)uVar8;
            *param_2 = *(short *)(iVar3 + 2) + *param_2;
            if ((int)(short)param_3 < (int)(short)uVar13 + (int)sVar6) {
              uVar14 = (int)param_3 - uVar7;
            }
            if (sVar6 < (short)local_30) {
              local_28 = local_30 - uVar7;
              uVar14 = uVar14 - local_28;
              uVar7 = local_30;
            }
            if ((int)(short)local_34 < (int)(short)uVar8 + (int)sVar9) {
              uVar16 = local_34 - uVar10;
            }
            if (sVar9 < (short)local_2c) {
              iVar12 = local_2c - uVar10;
              uVar16 = uVar16 - iVar12;
              uVar10 = local_2c;
            }
            else {
              iVar12 = 0;
            }
            if ((0 < (short)uVar14) && (0 < (short)uVar16)) {
              (*param_1)(local_1c,local_18,iVar3,local_24,uVar7,uVar10,local_28,iVar12,uVar14,uVar16
                        );
            }
          }
        }
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
