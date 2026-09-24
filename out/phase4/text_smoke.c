#include "tags.h"
#include "memory.h"
#include "text.h"

// Every pointer-width field in text.h is held as uint32_t, so the sizes below are exact on any host.
#define CHECK(name, cond) typedef char check_##name[(cond) ? 1 : -1]
#define OFF(t, f) __builtin_offsetof(t, f)

// text_parse_state (0x556b00 writes; 0x556bb0 / 0x556f10 advance)
CHECK(ps_size, sizeof(text_parse_state) == 0x1c);
CHECK(ps_font_def, OFF(text_parse_state, font_definition) == 0x04);
CHECK(ps_string, OFF(text_parse_state, string) == 0x08);
CHECK(ps_position, OFF(text_parse_state, position) == 0x0c);
CHECK(ps_style, OFF(text_parse_state, style) == 0x0e);
CHECK(ps_just, OFF(text_parse_state, justification) == 0x10);
CHECK(ps_char, OFF(text_parse_state, character) == 0x12);
CHECK(ps_token, OFF(text_parse_state, token) == 0x14);
CHECK(ps_color, OFF(text_parse_state, color) == 0x18);

// tags.h layouts the module reads by raw offset
CHECK(font_ascending, OFF(Font, ascending_height) == 0x04);
CHECK(font_descending, OFF(Font, descending_height) == 0x06);
CHECK(font_leading_h, OFF(Font, leading_height) == 0x08);
CHECK(font_leading_w, OFF(Font, leading_width) == 0x0a);
CHECK(font_tables_ptr, OFF(Font, character_tables) + OFF(TagReflexive, pointer) == 0x34);
CHECK(font_bold_id, OFF(Font, bold) + OFF(TagDependency, tag_id) == 0x48);
CHECK(font_style_stride, OFF(Font, italic) - OFF(Font, bold) == k_text_font_style_dependency_stride);
CHECK(font_underline, OFF(Font, underline) - OFF(Font, bold) == 3 * k_text_font_style_dependency_stride);
CHECK(font_chars_ptr, OFF(Font, characters) + OFF(TagReflexive, pointer) == 0x80);
CHECK(font_table_stride, sizeof(FontCharacterTables) == 0x0c);
CHECK(fc_size, sizeof(FontCharacter) == 0x14);
CHECK(fc_width, OFF(FontCharacter, character_width) == 0x02);
CHECK(fc_bw, OFF(FontCharacter, bitmap_width) == 0x04);
CHECK(fc_bh, OFF(FontCharacter, bitmap_height) == 0x06);
CHECK(fc_ox, OFF(FontCharacter, bitmap_origin_x) == 0x08);
CHECK(fc_oy, OFF(FontCharacter, bitmap_origin_y) == 0x0a);
CHECK(sl_entry, sizeof(StringListString) == 0x14);
CHECK(sl_ptr, OFF(StringListString, string) + OFF(TagDataOffset, pointer) == 0x0c);
CHECK(usl_entry, sizeof(UnicodeStringListString) == 0x14);
CHECK(sl_str4_size, _text_localization_single_byte_break_characters * sizeof(StringListString) == 0x50);
CHECK(sl_str6_ptr, _text_localization_no_break_characters * sizeof(StringListString) + 0x0c == 0x84);
CHECK(glob_ib, OFF(Globals, interface_bitmaps) == 0x140);
CHECK(ib_localization, OFF(GlobalsInterfaceBitmaps, localization) + OFF(TagDependency, tag_id) == 0xac);

// global layout: 0x006e4738 ColorARGB, tab stops up to the highlight words
CHECK(color_end, 0x006e4738 + sizeof(ColorARGB) == 0x006e4748);
CHECK(tabs_end, 0x006e474a + k_text_maximum_tab_stops * 2 == 0x006e476a);
CHECK(bounds_end, 0x006e4714 + sizeof(Rectangle2D) == 0x006e471c);

// enum values the code tests
CHECK(tok, _text_token_character == 6 && _text_token_style == 7);
CHECK(just, _text_justification_center == 2);
CHECK(style, _text_style_plain == -1 && _text_style_underline == 3);
CHECK(enc, _text_encoding_korean_johab == k_text_encoding_count - 1);

static void glyph(text_parse_state *s, void *f, void *c, uint32_t col, int16_t x, int16_t y,
    int16_t sx, int16_t sy, int16_t w, int16_t h) { (void)s; (void)f; (void)c; (void)col; (void)x; (void)y; (void)sx; (void)sy; (void)w; (void)h; }
int main(void) { text_glyph_draw_proc p = glyph; (void)p; return 0; }
