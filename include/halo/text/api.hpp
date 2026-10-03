/**
 * @file include/halo/text/api.hpp
 * Functions of the text module that other modules and the data tables call (namespace halo::text). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdarg.h>
#include <stdint.h>



struct ColorARGB;
struct Font;
struct FontCharacter;
struct Point2DInt;
struct Rectangle2D;
struct text_parse_state;
typedef uint32_t datum_index;
typedef void (*text_glyph_draw_proc)(text_parse_state *state, void *font, void *character,
    uint32_t color, int16_t x, int16_t y, int16_t source_x, int16_t source_y,
    int16_t width, int16_t height);

struct Globals;

namespace halo::text {

/**
 * The engine globals the text module owns (their storage is defined by standalone/data under the original link names);
 * other modules reach them through globals().
 */
struct Globals {
    ColorARGB &hud_text_draw_color_a;
    datum_index &hud_text_draw_font_tag_id;
    int16_t &hud_text_draw_background_mode;
    int16_t &hud_text_draw_color_or_flags;
    int16_t &hud_text_draw_column;
    uint32_t &hud_text_draw_unknown_4730;
    datum_index &localization_strings;
    float &color_scale;
    int16_t &ui_prompt_clip_x;
    int16_t &ui_prompt_clip_y;
    int16_t &text_encoding_state;
    char (&text_markup_codes)[11];
    ::Globals *&global_globals;
    char (&missing_string)[17];
    uint16_t (&missing_string_text)[];
    Rectangle2D &text_measure_bounds;
    uint32_t &text_measure_font;
    int16_t &text_highlight_start;
    int16_t &text_highlight_end;
    int16_t (&text_tab_stops)[16];
};

/**
 * The text service singleton. instance() builds the Globals reference table on first use (Meyers singleton); the state it
 * refers to lives in the data image. globals() is the short form every caller uses.
 */
class Service {
public:
    static Globals &instance();
};

inline Globals &globals() { return Service::instance(); }

uint8_t text_char_is_double_byte(uint8_t *string);
uint16_t text_get_next_character(uint8_t *string, int16_t *cursor);
uint8_t text_find_character(int16_t target_character, uint8_t *string);
uint16_t text_find_character_boundary(uint8_t *string, int16_t *length_inout);
void text_clamp_byte_length_to_character_boundary(uint8_t *string, int16_t *length_inout);
uint16_t * string_convert_ascii_to_unicode(uint16_t *dst, uint32_t capacity_bytes, const char *source);
uint8_t * string_convert_unicode_to_ascii(uint8_t *dest, uint16_t *source, int32_t capacity);
int16_t string_table_index_of(const char *search, int16_t count, const char **table);
int16_t text_parse_next_token_narrow(text_parse_state *state);
void text_draw_character_range_narrow(Rectangle2D *bounds, text_glyph_draw_proc callback, Point2DInt *pen, Rectangle2D *clip, uint32_t color, void *string, int16_t start_column, int16_t end_column);
void text_wrap_and_draw_narrow(text_glyph_draw_proc callback, Rectangle2D *bounds, Point2DInt *out_final_pen, Rectangle2D *clip, int16_t extra_line_spacing, void *string);
int16_t text_parse_next_token_wide(text_parse_state *state);
void text_draw_character_range_wide(Rectangle2D *bounds, text_glyph_draw_proc callback, Point2DInt *pen, Rectangle2D *clip, uint32_t color, void *string, int16_t start_column, int16_t end_column);
void text_wrap_and_draw_wide(text_glyph_draw_proc callback, Rectangle2D *bounds, Point2DInt *out_final_pen, Rectangle2D *clip, int16_t extra_line_spacing, void *string);
void text_set_render_context(datum_index font, ColorARGB *color, int16_t style, int16_t justification, uint32_t flags);
void text_parse_state_initialize(void *string, int16_t justification, int16_t style, text_parse_state *state, datum_index font, ColorARGB *color);
void text_language_initialize_from_string_list(void);
uint16_t * text_string_list_get_string(datum_index list_id, int16_t index);
FontCharacter * text_get_character_metrics(uint16_t character, Font *font);
void text_measure_string_extents(Rectangle2D *origin_bounds, Rectangle2D *out_cursor_rect, Rectangle2D *out_extents_rect, void *string);
int32_t text_measure_string_fit_width(void *string, int32_t *max_width_inout);
void text_measure_glyph_callback(text_parse_state *state, void *font, void *character, uint32_t color, int16_t x, int16_t y, int16_t source_x, int16_t source_y, int16_t width, int16_t height);
void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...);
void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...);

}
