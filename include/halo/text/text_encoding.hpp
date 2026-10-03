/**
 * @file include/halo/text/text_encoding.hpp
 * Narrow and wide text layout: tokenising, column drawing and wrapping behind one strategy interface.
 */
#pragma once

#include "halo/text/text_types.hpp"

namespace halo::text {

/**
 * Strategy for the two text encodings the engine lays out: 8-bit DBCS-aware strings with |x markup and UTF-16 strings.
 * Each implementation tokenises, draws a column range and wraps text for its own encoding.
 */
class text_encoding_strategy {
public:
    /**
     * Reads the next token (character, break character, tab, newline, justification or markup change) from the parse
     * state's narrow string and updates the state.
     */
    virtual int16_t parse_next_token(text_parse_state *state) = 0;

    /**
     * Draws the columns [start_column, end_column) of the current parse string through the callback, one glyph at a time,
     * clipped to bounds and clip. Highlighted columns are drawn with the colour inverted; the pen advances for every
     * glyph.
     */
    virtual void draw_character_range(Rectangle2D *bounds, text_glyph_draw_proc callback, Point2DInt *pen, Rectangle2D *clip, uint32_t color, void *string, int16_t start_column, int16_t end_column) = 0;

    /**
     * Lays out the string inside bounds with word wrapping, tab stops and justification, drawing each glyph through the
     * callback, and writes the final pen position.
     */
    virtual void wrap_and_draw(text_glyph_draw_proc callback, Rectangle2D *bounds, Point2DInt *out_final_pen, Rectangle2D *clip, int16_t extra_line_spacing, void *string) = 0;

};

/**
 * Layout for 8-bit strings: DBCS-aware, with |x markup escapes.
 */
class narrow_text_strategy final : public text_encoding_strategy {
public:
    static narrow_text_strategy &instance();

    /**
     * Reads the next token (character, break character, tab, newline, justification or markup change) from the parse
     * state's narrow string and updates the state.
     *
     * @address 0x556bb0
     */
    int16_t parse_next_token(text_parse_state *state) override;

    /**
     * Draws the columns [start_column, end_column) of the current parse string through the callback, one glyph at a time,
     * clipped to bounds and clip. Highlighted columns are drawn with the colour inverted; the pen advances for every
     * glyph.
     *
     * @address 0x557030
     */
    void draw_character_range(Rectangle2D *bounds, text_glyph_draw_proc callback, Point2DInt *pen, Rectangle2D *clip, uint32_t color, void *string, int16_t start_column, int16_t end_column) override;

    /**
     * Lays out the string inside bounds with word wrapping, tab stops and justification, drawing each glyph through the
     * callback, and writes the final pen position.
     *
     * @address 0x556400
     */
    void wrap_and_draw(text_glyph_draw_proc callback, Rectangle2D *bounds, Point2DInt *out_final_pen, Rectangle2D *clip, int16_t extra_line_spacing, void *string) override;

};

/**
 * Layout for UTF-16 strings.
 */
class wide_text_strategy final : public text_encoding_strategy {
public:
    static wide_text_strategy &instance();

    /**
     * Reads the next token from the parse state's UTF-16 string and updates the state.
     *
     * @address 0x556f10
     */
    int16_t parse_next_token(text_parse_state *state) override;

    /**
     * Draws the columns [start_column, end_column) of the current parse string through the callback, one glyph at a time,
     * clipped to bounds and clip. Highlighted columns are drawn with the colour inverted; the pen advances for every
     * glyph.
     *
     * @address 0x5572b0
     */
    void draw_character_range(Rectangle2D *bounds, text_glyph_draw_proc callback, Point2DInt *pen, Rectangle2D *clip, uint32_t color, void *string, int16_t start_column, int16_t end_column) override;

    /**
     * Lays out the UTF-16 string inside bounds with word wrapping, tab stops and justification, drawing each glyph through
     * the callback, and writes the final pen position.
     *
     * @address 0x556780
     */
    void wrap_and_draw(text_glyph_draw_proc callback, Rectangle2D *bounds, Point2DInt *out_final_pen, Rectangle2D *clip, int16_t extra_line_spacing, void *string) override;

};

}  // namespace halo::text
