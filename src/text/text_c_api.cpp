/**
 * @file src/text/text_c_api.cpp
 * The text module's C ABI: one extern "C" function per original symbol, same name and signature,
 * forwarding to the halo::text implementation. The shims do nothing else.
 */

#include "halo/text/text_c_api.h"

extern "C" uint8_t text_char_is_double_byte(uint8_t *string)
{
    return halo::text::dbcs_text::char_is_double_byte(string);
}

extern "C" uint16_t text_get_next_character(uint8_t *string, int16_t *cursor)
{
    return halo::text::dbcs_text::get_next_character(string, cursor);
}

extern "C" uint8_t text_find_character(int16_t target_character, uint8_t *string)
{
    return halo::text::dbcs_text::find_character(target_character, string);
}

extern "C" uint16_t text_find_character_boundary(uint8_t *string, int16_t *length_inout)
{
    return halo::text::dbcs_text::find_character_boundary(string, length_inout);
}

extern "C" void text_clamp_byte_length_to_character_boundary(uint8_t *string, int16_t *length_inout)
{
    halo::text::dbcs_text::clamp_byte_length_to_character_boundary(string, length_inout);
}

extern "C" uint16_t * string_convert_ascii_to_unicode(uint16_t *dst, uint32_t capacity_bytes, const char *source)
{
    return halo::text::string_codec::ascii_to_unicode(dst, capacity_bytes, source);
}

extern "C" uint8_t * string_convert_unicode_to_ascii(uint8_t *dest, uint16_t *source, int32_t capacity)
{
    return halo::text::string_codec::unicode_to_ascii(dest, source, capacity);
}

extern "C" int16_t string_table_index_of(const char *search, int16_t count, const char **table)
{
    return halo::text::string_codec::table_index_of(search, count, table);
}

extern "C" int16_t text_parse_next_token_narrow(text_parse_state *state)
{
    return halo::text::narrow_text_strategy::instance().parse_next_token(state);
}

extern "C" void text_draw_character_range_narrow(Rectangle2D *bounds, text_glyph_draw_proc callback, Point2DInt *pen, Rectangle2D *clip, uint32_t color, void *string, int16_t start_column, int16_t end_column)
{
    halo::text::narrow_text_strategy::instance().draw_character_range(bounds, callback, pen, clip, color, string, start_column, end_column);
}

extern "C" void text_wrap_and_draw_narrow(text_glyph_draw_proc callback, Rectangle2D *bounds, Point2DInt *out_final_pen, Rectangle2D *clip, int16_t extra_line_spacing, void *string)
{
    halo::text::narrow_text_strategy::instance().wrap_and_draw(callback, bounds, out_final_pen, clip, extra_line_spacing, string);
}

extern "C" int16_t text_parse_next_token_wide(text_parse_state *state)
{
    return halo::text::wide_text_strategy::instance().parse_next_token(state);
}

extern "C" void text_draw_character_range_wide(Rectangle2D *bounds, text_glyph_draw_proc callback, Point2DInt *pen, Rectangle2D *clip, uint32_t color, void *string, int16_t start_column, int16_t end_column)
{
    halo::text::wide_text_strategy::instance().draw_character_range(bounds, callback, pen, clip, color, string, start_column, end_column);
}

extern "C" void text_wrap_and_draw_wide(text_glyph_draw_proc callback, Rectangle2D *bounds, Point2DInt *out_final_pen, Rectangle2D *clip, int16_t extra_line_spacing, void *string)
{
    halo::text::wide_text_strategy::instance().wrap_and_draw(callback, bounds, out_final_pen, clip, extra_line_spacing, string);
}

extern "C" void text_set_render_context(datum_index font, ColorARGB *color, int16_t style, int16_t justification, uint32_t flags)
{
    halo::text::text_context::set_render_context(font, color, style, justification, flags);
}

extern "C" void text_parse_state_initialize(void *string, int16_t justification, int16_t style, text_parse_state *state, datum_index font, ColorARGB *color)
{
    halo::text::text_context::parse_state_initialize(string, justification, style, state, font, color);
}

extern "C" void text_language_initialize_from_string_list(void)
{
    halo::text::text_context::language_initialize_from_string_list();
}

extern "C" uint16_t * text_string_list_get_string(datum_index list_id, int16_t index)
{
    return halo::text::text_context::string_list_get_string(list_id, index);
}

extern "C" FontCharacter * text_get_character_metrics(uint16_t character, Font *font)
{
    return halo::text::text_context::get_character_metrics(character, font);
}

extern "C" void text_measure_string_extents(Rectangle2D *origin_bounds, Rectangle2D *out_cursor_rect, Rectangle2D *out_extents_rect, void *string)
{
    halo::text::text_context::measure_string_extents(origin_bounds, out_cursor_rect, out_extents_rect, string);
}

extern "C" int32_t text_measure_string_fit_width(void *string, int32_t *max_width_inout)
{
    return halo::text::text_context::measure_string_fit_width(string, max_width_inout);
}

extern "C" void text_measure_glyph_callback(text_parse_state *state, void *font, void *character, uint32_t color, int16_t x, int16_t y, int16_t source_x, int16_t source_y, int16_t width, int16_t height)
{
    halo::text::text_measure_glyph_callback(state, font, character, color, x, y, source_x, source_y, width, height);
}

extern "C" void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...)
{
    va_list args;

    va_start(args, format);
    halo::text::wide_string_format::format_v(dest, format, args);
    va_end(args);
}

extern "C" void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...)
{
    va_list args;

    va_start(args, format);
    halo::text::wide_string_format::format_bounded_v(count, dest, format, args);
    va_end(args);
}
