// Blam text module (halo.exe 1.0.10 retail, 0x5561b0..0x5579d2, 22 Ghidra functions).
// The bitmap-font text layout and draw engine: a global draw state (font, style,
// justification, flags, colour, tab stops, indents), a word-wrap loop that measures
// and places each line, and a per-glyph draw loop that hands every visible glyph to a
// caller-supplied callback. There are two parallel pipelines:
//   - narrow (8-bit, DBCS aware, with |x markup codes):
//       wrap 0x556400 -> tokenizer 0x556bb0 -> draw range 0x557030
//   - wide (UTF-16 code units, only |n recognised):
//       wrap 0x556780 -> tokenizer 0x556f10 -> draw range 0x5572b0,
//       also used by the fit-width measure 0x557530 and the extents probe 0x5562d0
//       (text_measure_string_extents, glyph callback text_measure_glyph_callback 0x556260)
// Both share one parse state record (text_parse_state below) that 0x556b00 fills.
//
// Offsets in comments are byte offsets from the struct base. Where the binary carries
// the layout it is preferred, and said so:
//   - text_parse_state (0x1c): 0x556b00 writes every field it owns; each of the four
//     frames that embed it (0x556400, 0x556780, 0x557030, 0x557530) places it at the top
//     of the local area with exactly 0x1c bytes before the saved registers.
//   - Font and FontCharacter are the tag definitions in types/tags.h. Every offset the
//     module reads lines up with them: Font +0x04/+0x06/+0x08/+0x0a heights and width,
//     +0x34 character_tables.pointer (stride 0x0c, count 0x100), +0x48 + style * 0x10 is
//     the tag_id of bold/italic/condense/underline, +0x80 characters.pointer (stride
//     0x14); FontCharacter +0x02 character_width, +0x04/+0x06 bitmap size, +0x08/+0x0a
//     bitmap origin.
//   - StringList / StringListString and UnicodeStringList / UnicodeStringListString are
//     the tags.h definitions (entry stride 0x14, TagDataOffset size at +0x00, pointer at
//     +0x0c). GlobalsInterfaceBitmaps.localization (+0xa0, tag_id at +0xac) is the
//     string list 0x5561b0 caches.
//
// Types this module uses that are defined elsewhere and NOT repeated here:
//   types/tags.h    Font, FontCharacter, FontCharacterTables, ColorARGB, Rectangle2D,
//                   Point2DInt, StringList, UnicodeStringList, Globals,
//                   GlobalsInterfaceBitmaps
//   types/memory.h  datum_index
//   types/cache.h   tag_instance (0x0087bc14, tag data at +0x14)
//
// Pointer-width fields are held as uint32_t with the pointee in the comment, so the
// sizes are exact on any host. Wide text is UTF-16 held in uint16_t (no wchar_t for
// the CParser).
//
// Not part of the types of this module (see out/phase4/text_types_notes.md):
//   0x557910 / 0x557930 wrap the CRT wide vsnwprintf / vswprintf; 0x557950 and 0x557990
//   are narrow/wide string copies. They live in the text directory but touch no struct.
#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// ---------------------------------------------------------------------------
// constants
// ---------------------------------------------------------------------------
typedef enum text_constants {
    // tab stop slots between 0x006e474a and the highlight words at 0x006e476a.
    // Callers fill at most 7 (decompile line 65006 writes count 7, slots 0..6 at
    // 0x006e474a..0x006e4757, x = 0x19 .. 0x230); 16 is the room the .bss gap leaves,
    // not a bound the code checks.
    k_text_maximum_tab_stops = 16,
    // 0x5561b0 clamps the localization value into 0 .. 5
    k_text_encoding_count = 6,
    // FontCharacterTables.character_table.count a page must have to be used
    // (0x556400, 0x556780, 0x557030, 0x5572b0, 0x557530, 0x557650)
    k_text_font_character_table_page_size = 0x100,
    // Font +0x48 + style * 0x10: the tag_id of Font.bold / italic / condense / underline
    k_text_font_style_dependency_stride = 0x10,
    // 0x556b00 scales each ColorARGB channel by 255.0 (0x00672b60) before __ftol
    k_text_color_channel_scale = 255
} text_constants;

// ---------------------------------------------------------------------------
// text_justification  (0x006e4736, text_parse_state.justification)
// Written by the |l |r |c markup codes in 0x556bb0; the wrap loops right-align on 1
// (x = right - leading_width - span width) and centre on 2.
// ---------------------------------------------------------------------------
typedef enum text_justification {
    _text_justification_left = 0,
    _text_justification_right = 1,
    _text_justification_center = 2
} text_justification;

// ---------------------------------------------------------------------------
// text_style  (0x006e4734, text_parse_state.style)
// Selects which Font dependency replaces the base font: the style value indexes the
// four TagDependency slots starting at Font.bold (tag_id at +0x48 + style * 0x10).
// A style whose dependency is NONE (tag_id -1) falls back to the base font.
// Markup codes (0x556bb0): |b bold, |i italic, |k condense, |u underline, |p plain.
// ---------------------------------------------------------------------------
typedef enum text_style {
    _text_style_plain = -1,
    _text_style_bold = 0,
    _text_style_italic = 1,
    _text_style_condense = 2,
    _text_style_underline = 3
} text_style;

// ---------------------------------------------------------------------------
// text_token  (return value of 0x556bb0 / 0x556f10, text_parse_state.token)
// 0x556bb0 classifies an ordinary glyph as a break point (2) or not (6) using the
// localization string list (see text_localization_string). 0x556f10 never returns
// 2, 4 or 7. 5 is never produced by either tokenizer; 0x557530 handles it like 1..4
// and 0x556bb0 loops past it the way it loops past 7.
// ---------------------------------------------------------------------------
typedef enum text_token {
    _text_token_end = 0,               // NUL
    _text_token_newline = 1,           // CR (0x0d), or |n
    _text_token_break_character = 2,   // glyph after which a line may wrap
    _text_token_tab = 3,               // TAB (0x09), or |t
    _text_token_justification = 4,     // |l |r |c
    _text_token_unused_5 = 5,
    _text_token_character = 6,         // ordinary glyph
    _text_token_style = 7              // |b |i |k |u |p, consumed inside 0x556bb0
} text_token;

// ---------------------------------------------------------------------------
// text_flags  (0x006e4730, third argument of text_set_render_context 0x5563b0)
// ---------------------------------------------------------------------------
typedef enum text_flags {
    // 0x556400 / 0x556780: when a glyph runs past the right edge, back up to the last
    // break character and start a new line; without it the line runs on
    _text_flag_word_wrap_bit = 0x01,
    // 0x556400 / 0x556780: draw the line even when its baseline is at or below the
    // bounds bottom (otherwise such lines are measured but not drawn)
    _text_flag_draw_past_bottom_bit = 0x02,
    // written as 8 by one caller (decompile line 61238); nothing in the module tests it
    _text_flag_unknown_bit3 = 0x08
} text_flags;

// ---------------------------------------------------------------------------
// text_encoding  (0x006e4800)
// The double-byte code page text_char_is_double_byte 0x557750 applies to 8-bit text,
// read by 0x5561b0 as atol() of localization string 0. The names come from the lead
// and trail byte ranges the switch tests:
//   1  lead 81-9f or e0-fe, trail 40-fc except 7f                Shift-JIS
//   2  lead a1-fe,           trail a1-fe                          EUC (GB2312 or EUC-KR)
//   3  lead 81-fe,           trail 40-7e or a1-fe                 Big5
//   4  lead 81-fe,           trail 41-5a, 61-7a or 81-fe          Korean UHC (cp949)
//   5  lead 84-d3, d8-de or e0-f9, trail 41-7e or 81-fe           Korean Johab
// The language names are inferred from the ranges; the binary only has the numbers.
// ---------------------------------------------------------------------------
typedef enum text_encoding {
    _text_encoding_single_byte = 0,
    _text_encoding_shift_jis = 1,
    _text_encoding_euc = 2,
    _text_encoding_big5 = 3,
    _text_encoding_korean_uhc = 4,
    _text_encoding_korean_johab = 5
} text_encoding;

// ---------------------------------------------------------------------------
// text_localization_string  (indices into the string list at 0x006e4728)
// 0x5561b0 reads string 0; 0x556bb0 reads strings 4, 5 and 6 as character sets and
// searches them with 0x557870. A glyph is a break character when
//   single byte: it is in string 4 and not in string 6
//   double byte: it is not in string 5 and not in string 6
// Strings 1 .. 3 are not read by this module.
// ---------------------------------------------------------------------------
typedef enum text_localization_string {
    _text_localization_encoding = 0,
    _text_localization_single_byte_break_characters = 4,
    _text_localization_double_byte_no_break_characters = 5,
    _text_localization_no_break_characters = 6
} text_localization_string;

// ---------------------------------------------------------------------------
// text_parse_state  (text_parse_state_initialize 0x556b00 fills it; the tokenizers
// 0x556bb0 (EDI) and 0x556f10 (EAX) advance it; 0x556400, 0x556780, 0x557030,
// 0x5572b0 and 0x557530 keep one on their stack)
// 0x556b00 register use: ESI state, stack font datum and ColorARGB*, ECX string,
// DX justification, BX style. Every caller passes 0x006e472c, 0x006e4738, 0x006e4736
// and 0x006e4734. The draw loops pass the whole record to the glyph callback as its
// first argument.
// ---------------------------------------------------------------------------
typedef struct text_parse_state {
    datum_index font;              // 0x00 base font tag (0x006e472c); never rewritten
    uint32_t font_definition;      // 0x04 Font* tag data of the font the current style
                                   //      resolves to; rewritten by style tokens in 0x556bb0
    uint32_t string;               // 0x08 char* (0x556bb0 path) or uint16_t* UTF-16
                                   //      (0x556f10 path); not owned
    int16_t position;              // 0x0c next index into string, in bytes or code units;
                                   //      0 at init, the draw loops seed it with the span start
    int16_t style;                 // 0x0e text_style
    int16_t justification;         // 0x10 text_justification
    uint16_t character;            // 0x12 last code read; a double byte pair is lead << 8 | trail
    int16_t token;                 // 0x14 text_token of the last token
    int16_t unknown_16;            // 0x16 never written or read (alignment)
    uint32_t color;                // 0x18 a << 24 | r << 16 | g << 8 | b from the ColorARGB
                                   //      argument, each channel * 255.0 through __ftol;
                                   //      the draw loops XOR it with 0xffffff inside the highlight
} text_parse_state;                // size 0x1c

// ---------------------------------------------------------------------------
// text_glyph_draw_proc  (called by 0x557030 at 0x557283 and by 0x5572b0 at 0x557503)
// cdecl, the caller pops 0x28 bytes: ten dword slots. The 16-bit values are pushed as
// dwords and read back as words by the one callee in this module (0x556260).
// x/y are the clipped screen position of the glyph bitmap, source_x/source_y the
// number of texels clipped off its left/top, width/height what is left to draw.
// ---------------------------------------------------------------------------
typedef void (*text_glyph_draw_proc)(text_parse_state *state, void *font, void *character,
    uint32_t color, int16_t x, int16_t y, int16_t source_x, int16_t source_y,
    int16_t width, int16_t height);

// ---------------------------------------------------------------------------
// module globals
//
// The globals that hold a text_flags / text_style / text_justification / text_encoding
// value carry a _state suffix: C does not allow a variable and a typedef to share a name.
//
// draw state, set by text_set_render_context 0x5563b0 (ECX font, EAX ColorARGB*, stack
// style, justification, flags) and, inlined by LTCG, directly by about 17 interface,
// HUD, console and debug-overlay routines
// global 0x006e472c: datum_index text_font                 font tag; -1 after 0x5561b0
// global 0x006e4730: uint32_t text_flags_state             text_flags; 0 after 0x5561b0
// global 0x006e4734: int16_t text_style_state              text_style
// global 0x006e4736: int16_t text_justification_state      text_justification; 0 after 0x5561b0
// global 0x006e4738: ColorARGB text_color                  alpha first; callers store 1.0 there
//                                                         (decompile line 58189)
// global 0x006e4748: int16_t text_tab_stop_count           0 after 0x5561b0
// global 0x006e474a: int16_t text_tab_stops[k_text_maximum_tab_stops]
//                                                         absolute x, not offsets from the bounds:
//                                                         the span after tab n (1-based) starts at
//                                                         text_tab_stops[n - 1] and, while n < count,
//                                                         its right edge is text_tab_stops[n]
// global 0x006e476a: int16_t text_highlight_start          first index drawn with inverted colour
// global 0x006e476c: int16_t text_highlight_end            one past the last; both zeroed at the end
//                                                         of every wrap, never set anywhere else
// global 0x006e476e: int16_t text_first_line_indent        x added on line 0; the UI prompt code sets
//                                                         it to the pen x left by the previous span
// global 0x006e4770: int16_t text_wrapped_line_indent      x added on every later line; always 0
//
// localization, set by text_language_initialize_from_string_list 0x5561b0
// global 0x006e4728: datum_index text_localization_strings StringList tag from
//                                                         global_globals->interface_bitmaps[0]
//                                                         .localization; -1 on map unload (0x45b370)
// global 0x006e4800: int16_t text_encoding_state           text_encoding, 0 .. 5
//
// extents probe, owned by text_measure_string_extents 0x5562d0 and its glyph callback
// text_measure_glyph_callback 0x556260 (not a Ghidra function)
// global 0x006e4714: Rectangle2D text_measure_bounds       union of every glyph rectangle the
//                                                         callback sees; reset to 0x7fff, 0x7fff,
//                                                         0x8000, 0x8000 (top, left, bottom, right)
// global 0x006e471c: uint32_t text_measure_font            Font* of the last glyph drawn, used for
//                                                         the cursor ascent/descent
//
// .rdata
// global 0x00671fa0: char text_markup_codes[11]            "ibukprlctn", the letters after '|' that
//                                                         0x557750 treats as a two-byte escape
// global 0x00671fd0: char missing_string[17]               "<missing string>", the narrow fallback
//                                                         of 0x5561b0 and 0x556bb0
// global 0x00672b60: float text_color_scale                255.0 (shared .rdata constant)
//
// Globals this module reads but does not own:
//   0x00671fac  uint16_t missing_string_text[]  (types/input.h), fallback of 0x5578c0
//   0x0087bc14  tag_instance *tag_instances      (types/cache.h)
//   0x00746fa0  Globals *global_globals          (cache)
// ---------------------------------------------------------------------------

#pragma pack(pop)
