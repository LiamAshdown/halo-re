// virtual_keyboard_draw_text  (Ghidra: virtual_keyboard_draw_text, already named)
// address 0x4a9300, size 521 bytes
// name confidence: 0.6 (existing Ghidra name)   rewrite confidence: 0.8
// evidence: rewritten from objdump 0x4a9300..0x4a9508 in the phase-4 review. The first
// rewrite had no parameter (the function takes the text bounds on the stack), dropped the
// register arguments of every callee and invented a return value (the function returns
// nothing meaningful).
//   Sets the shared text state (hud_text_draw_*: small_ui font, color 1.0 / 0.9 / 0.9 / 0.9,
// flags 0xffff, column 2). While the keyboard is opened, the edit text is measured
// (0x5562d0: EBX bounds, ESI cursor rect, EDI output rect; the cursor rect is not
// initialised by this function) and a highlight is drawn behind it with the white bitmap,
// the output rect widened by 2 on each side (ui_draw_screen_quad with the bounds as the
// source rect, color 0x7f7f7f7f). The text is drawn into the bounds (0x514ab0, EAX and ECX
// the bounds). When the keyboard is not opened and the white bitmap exists, a caret is
// drawn on odd seconds of the millisecond clock (0x449210 / 1000): the text is walked with
// text_get_character_metrics (DX character, EDI font data) summing the glyph advances (+2)
// of the whole text and of the characters before destination_end; the caret rect is one
// pixel wide at (left + right) / 2 - total / 2 + advance_before_caret, from y 0x78 to 0x78 +
// font ascent + descent (+4, +6 of the font tag data).
// register convention: plain cdecl, one stack argument.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern virtual_keyboard_globals virtual_keyboard; // 0x007193a8
extern tag_instance *tag_instances;               // 0x0087bc14

extern int32_t hud_text_draw_font_tag_id;     // 0x006e472c
extern uint32_t hud_text_draw_unknown_4730;   // 0x006e4730
extern uint16_t hud_text_draw_color_or_flags; // 0x006e4734
extern int16_t hud_text_draw_column;          // 0x006e4736
extern float hud_text_draw_color_a;           // 0x006e4738
extern float hud_text_draw_color_r;           // 0x006e473c
extern float hud_text_draw_color_g;           // 0x006e4740
extern float hud_text_draw_color_b;           // 0x006e4744

extern BitmapData *bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag, int16_t frame, int16_t sequence); // 0x43f290, blam-cc: EAX tag, DI frame
extern void text_measure_string_extents(Rectangle2D *origin, Rectangle2D *cursor, Rectangle2D *out, const uint16_t *text); // 0x5562d0, blam-cc: EBX origin, ESI cursor, EDI out; measures text
extern void ui_draw_screen_quad(int16_t *source_rect, int16_t *dest_rect, int32_t bitmap_data,
                                int16_t *clip_rect, uint32_t vertex_color); // 0x498b20, blam-cc: EAX source_rect, ECX dest_rect
extern void chimera__draw_16_bit_text(Rectangle2D *clip, Rectangle2D *bounds, int32_t unknown_0, int32_t unknown_1,
                                      const uint16_t *text); // 0x514ab0, blam-cc: EAX clip, ECX bounds
extern uint32_t time_query_performance_counter_ms(void); // 0x449210
extern const int16_t *text_get_character_metrics(uint16_t character, const void *font_data); // 0x557650, blam-cc: DX character, EDI font_data; +2 is the advance

void virtual_keyboard_draw_text(Rectangle2D *bounds)
{
    const uint8_t *font_data = (const uint8_t *)tag_instances[virtual_keyboard.small_ui_tag & 0xffff].data;

    hud_text_draw_font_tag_id = virtual_keyboard.small_ui_tag;
    hud_text_draw_color_a = 1.0f;
    hud_text_draw_color_b = 0.9f;
    hud_text_draw_color_r = 0.9f;
    hud_text_draw_color_g = 0.9f;
    hud_text_draw_color_or_flags = 0xffff;
    hud_text_draw_column = 2;
    hud_text_draw_unknown_4730 = 0;

    if (virtual_keyboard.opened == 1) {
        BitmapData *white = bitmap_group_sequence_get_bitmap_data(virtual_keyboard.white_bitmap, 0, 0);

        if (white != 0) {
            Rectangle2D cursor; // UNSURE: never initialised by this function
            Rectangle2D highlight;

            text_measure_string_extents(bounds, &cursor, &highlight, virtual_keyboard.destination);
            highlight.left -= 2;
            highlight.right += 2;
            ui_draw_screen_quad((int16_t *)bounds, (int16_t *)&highlight, (int32_t)white, 0, 0x7f7f7f7f);
        }
    }

    chimera__draw_16_bit_text(bounds, bounds, 0, 0, virtual_keyboard.destination);

    if (virtual_keyboard.opened == 0 && virtual_keyboard.white_bitmap != (datum_index)-1 &&
        ((time_query_performance_counter_ms() / 1000) & 1) != 0) {
        int16_t height = (int16_t)(*(const int16_t *)(font_data + 6) + *(const int16_t *)(font_data + 4));
        int16_t advance_before_caret = 0;
        int16_t total_advance = 0;
        const uint16_t *cursor = virtual_keyboard.destination;
        BitmapData *white = bitmap_group_sequence_get_bitmap_data(virtual_keyboard.white_bitmap, 0, 0);

        if (white != 0) {
            Rectangle2D caret;
            uint16_t character = *cursor;

            while (character != 0) {
                const int16_t *metrics = text_get_character_metrics(character, font_data);

                if (metrics == 0) {
                    break;
                }
                if (cursor < virtual_keyboard.destination_end) {
                    advance_before_caret += metrics[1];
                }
                character = cursor[1];
                total_advance += metrics[1];
                cursor++;
            }
            caret.top = 0x78;
            caret.left = (int16_t)((bounds->left + bounds->right) / 2 - (total_advance >> 1) + advance_before_caret);
            caret.bottom = (int16_t)(height + 0x78);
            caret.right = (int16_t)(caret.left + 1);
            ui_draw_screen_quad(0, (int16_t *)&caret, (int32_t)white, 0, 0xffffffff);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4a9300):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint __cdecl virtual_keyboard_draw_text(void)

{
  short sVar1;
  int iVar2;
  undefined4 uVar3;
  uint uVar4;
  int iVar5;
  uint uVar6;
  short *psVar7;

  DAT_006e472c = DAT_00719418;
  DAT_006e4738 = 0x3f800000;
  DAT_006e4744 = 0x3f666666;
  DAT_006e473c = 0x3f666666;
  DAT_006e4740 = 0x3f666666;
  DAT_006e4734._0_2_ = 0xffff;
  DAT_006e4734._2_2_ = 2;
  _DAT_006e4730 = 0;
  if ((DAT_007193bc._3_1_ == '\x01') &&
     (iVar2 = bitmap_group_sequence_get_bitmap_data(0), iVar2 != 0)) {
    FUN_005562d0(DAT_007193c0);
    FUN_00498b20(iVar2,0,0x7f7f7f7f);
  }
  uVar3 = chimera__draw_16_bit_text(0,0,DAT_007193c0);
  uVar6 = CONCAT31((int3)((uint)uVar3 >> 8),DAT_007193bc._3_1_);
  if ((DAT_007193bc._3_1_ == '\0') && (DAT_007193cc != -1)) {
    uVar4 = FUN_00449210();
    psVar7 = DAT_007193c0;
    uVar6 = uVar4 * 0x10624dd3;
    if ((uVar4 / 1000 & 1) != 0) {
      iVar2 = bitmap_group_sequence_get_bitmap_data(0);
      uVar6 = 0;
      if (iVar2 != 0) {
        sVar1 = *psVar7;
        while ((sVar1 != 0 && (iVar5 = text_get_character_metrics(), iVar5 != 0))) {
          sVar1 = psVar7[1];
          psVar7 = psVar7 + 1;
        }
        uVar6 = FUN_00498b20(iVar2,0,0xffffffff);
      }
    }
  }
  return uVar6;
}
#endif
