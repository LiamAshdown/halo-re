// virtual_keyboard_render  (Ghidra: virtual_keyboard_render, already named)
// address 0x4a9510, size 472 bytes
// name confidence: 0.6 (existing Ghidra name)   rewrite confidence: 0.8
// evidence: rewritten from objdump 0x4a9510..0x4a96e7 in the phase-4 review. The first
// rewrite dropped the register arguments of the bitmap, quad, string and text calls, the
// four text rectangles, the string index (field_kind) of the title and the bounds argument
// of virtual_keyboard_draw_text.
//   With the strings tag data (virtual_keyboard.strings_tag_data) holding a background
// bitmap at +0x1c, draws it over the whole 640x480 screen. Then, with the large_ui font and
// color 1.0 / 0.9 / 0.9 / 0.9 (flags 0xffff), draws the title (string field_kind of the
// string list at +0x2c, column 0) in {0x4e, 0x72, 0x6e, 0x280} and the prompt (string 14 of
// the same list, terminated in place; L"<missing string>" 0x00671fac without it, column 1)
// in {0x19e, 0, 0x1c2, 0x276}, and finally the edit text in {0x76, 0x78, 0x8f, 0x208}.
// Rectangles are {top, left, bottom, right}.
// register convention: plain cdecl, no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern virtual_keyboard_globals virtual_keyboard; // 0x007193a8
extern tag_instance *tag_instances;               // 0x0087bc14
extern uint16_t missing_string_text[]; // 0x00671fac, L"<missing string>"

extern int32_t hud_text_draw_font_tag_id;     // 0x006e472c
extern uint32_t hud_text_draw_unknown_4730;   // 0x006e4730
extern uint16_t hud_text_draw_color_or_flags; // 0x006e4734
extern int16_t hud_text_draw_column;          // 0x006e4736
extern float hud_text_draw_color_a;           // 0x006e4738
extern float hud_text_draw_color_r;           // 0x006e473c
extern float hud_text_draw_color_g;           // 0x006e4740
extern float hud_text_draw_color_b;           // 0x006e4744

extern BitmapData *bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag, int16_t frame, int16_t sequence); // 0x43f290, blam-cc: EAX tag, DI frame
extern void ui_draw_screen_quad(int16_t *source_rect, int16_t *dest_rect, int32_t bitmap_data,
                                int16_t *clip_rect, uint32_t vertex_color); // 0x498b20, blam-cc: EAX source_rect, ECX dest_rect
extern uint16_t *text_string_list_get_string(datum_index tag_id, int16_t index); // 0x5578c0; blam-cc: ECX, DX
extern void chimera__draw_16_bit_text(Rectangle2D *clip, Rectangle2D *bounds, int32_t unknown_0, int32_t unknown_1,
                                      const uint16_t *text); // 0x514ab0, blam-cc: EAX clip, ECX bounds
extern void virtual_keyboard_draw_text(Rectangle2D *bounds); // 0x4a9300

static void virtual_keyboard_set_text_state(int16_t column)
{
    hud_text_draw_font_tag_id = virtual_keyboard.large_ui_tag;
    hud_text_draw_color_a = 1.0f;
    hud_text_draw_color_r = 0.9f;
    hud_text_draw_color_g = 0.9f;
    hud_text_draw_color_b = 0.9f;
    hud_text_draw_color_or_flags = 0xffff;
    hud_text_draw_column = column;
    hud_text_draw_unknown_4730 = 0;
}

void virtual_keyboard_render(void)
{
    const uint8_t *strings = (const uint8_t *)virtual_keyboard.strings_tag_data;
    datum_index background = *(const datum_index *)(strings + 0x1c);
    datum_index string_list;
    const uint16_t *prompt = missing_string_text;
    Rectangle2D rect;

    if (background != (datum_index)-1) {
        BitmapData *bitmap = bitmap_group_sequence_get_bitmap_data(background, 0, 0);

        rect.top = 0;
        rect.left = 0;
        rect.bottom = 0x1e0;
        rect.right = 0x280;
        ui_draw_screen_quad((int16_t *)&rect, (int16_t *)&rect, (int32_t)bitmap, 0, 0xffffffff);
    }

    virtual_keyboard_set_text_state(0);
    string_list = *(const datum_index *)((const uint8_t *)virtual_keyboard.strings_tag_data + 0x2c);
    if (string_list != (datum_index)-1) {
        const uint16_t *title = text_string_list_get_string(string_list, virtual_keyboard.field_kind);

        rect.top = 0x4e;
        rect.left = 0x72;
        rect.bottom = 0x6e;
        rect.right = 0x280;
        chimera__draw_16_bit_text(&rect, &rect, 0, 0, title);
    }

    string_list = *(const datum_index *)((const uint8_t *)virtual_keyboard.strings_tag_data + 0x2c);
    if (string_list != (datum_index)-1) {
        UnicodeStringList *list = (UnicodeStringList *)tag_instances[string_list & 0xffff].data;

        if (list->strings.count > 0xe) {
            UnicodeStringListString *entry = (UnicodeStringListString *)list->strings.pointer + 0xe;

            if ((int32_t)entry->string.size > 0) {
                uint16_t *text = (uint16_t *)entry->string.pointer;

                text[(entry->string.size >> 1) - 1] = 0;
                prompt = text;
            }
        }
    }
    virtual_keyboard_set_text_state(1);
    rect.top = 0x19e;
    rect.left = 0;
    rect.bottom = 0x1c2;
    rect.right = 0x276;
    chimera__draw_16_bit_text(&rect, &rect, 0, 0, prompt);

    rect.top = 0x76;
    rect.left = 0x78;
    rect.bottom = 0x8f;
    rect.right = 0x208;
    virtual_keyboard_draw_text(&rect);
}

#if 0
Original Ghidra decompilation (0x4a9510):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl virtual_keyboard_render(void)

{
  int *piVar1;
  int iVar2;
  uint uVar3;
  undefined4 uVar4;
  undefined **ppuVar5;

  if (*(int *)(DAT_007193ac + 0x1c) != -1) {
    uVar4 = bitmap_group_sequence_get_bitmap_data(0,0,0xffffffff);
    FUN_00498b20(uVar4);
  }
  DAT_006e472c = DAT_00719414;
  DAT_006e4738 = 0x3f800000;
  DAT_006e473c = 0x3f666666;
  DAT_006e4740 = 0x3f666666;
  DAT_006e4744 = 0x3f666666;
  DAT_006e4734._0_2_ = 0xffff;
  DAT_006e4734._2_2_ = 0;
  _DAT_006e4730 = 0;
  if (*(int *)(DAT_007193ac + 0x2c) != -1) {
    uVar4 = text_string_list_get_string();
    chimera__draw_16_bit_text(0,0,uVar4);
  }
  ppuVar5 = &PTR_DAT_00671fac;
  if ((*(uint *)(DAT_007193ac + 0x2c) != 0xffffffff) &&
     (piVar1 = *(int **)((*(uint *)(DAT_007193ac + 0x2c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
     0xe < *piVar1)) {
    iVar2 = piVar1[1];
    uVar3 = *(uint *)(iVar2 + 0x118);
    if (0 < (int)uVar3) {
      ppuVar5 = *(undefined ***)(iVar2 + 0x124);
      *(undefined2 *)((int)ppuVar5 + ((uVar3 & 0xfffffffe) - 2)) = 0;
    }
  }
  DAT_006e472c = DAT_00719414;
  DAT_006e4744 = 0x3f666666;
  DAT_006e4738 = 0x3f800000;
  DAT_006e473c = 0x3f666666;
  DAT_006e4740 = 0x3f666666;
  DAT_006e4734._0_2_ = 0xffff;
  DAT_006e4734._2_2_ = 1;
  _DAT_006e4730 = 0;
  chimera__draw_16_bit_text(0,0,ppuVar5);
  virtual_keyboard_draw_text();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
