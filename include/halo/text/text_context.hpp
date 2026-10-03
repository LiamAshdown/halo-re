/**
 * @file include/halo/text/text_context.hpp
 * The shared text render context, fonts, localised string lists and measuring.
 */
#pragma once

#include "halo/text/text_types.hpp"

namespace halo::text {

/**
 * Operations on the engine's shared text render state (font, colour, style, justification) plus font and string list
 * lookup and string measuring.
 */
struct text_context {
    /**
     * Stores the font, colour, style, justification and flags the following text draws use.
     *
     * @address 0x5563b0
     */
    static void set_render_context(datum_index font, ColorARGB *color, int16_t style, int16_t justification, uint32_t flags);

    /**
     * Initialises a text parse state for a string: packs the colour to ARGB bytes, resolves the styled font variant and
     * records the font definition.
     *
     * @address 0x556b00
     */
    static void parse_state_initialize(void *string, int16_t justification, int16_t style, text_parse_state *state, datum_index font, ColorARGB *color);

    /**
     * Initialises the text language state from the localisation string list.
     *
     * @address 0x5561b0
     */
    static void language_initialize_from_string_list(void);

    /**
     * Fetches a string of a unicode string list tag, or the missing-string text when the list is -1, the index is out of
     * range or the entry is empty.
     *
     * @address 0x5578c0
     */
    static uint16_t * string_list_get_string(datum_index list_id, int16_t index);

    /**
     * Looks a character up in the font's two-level character table and returns its metrics, or NULL when the font has no
     * glyph for it.
     *
     * @address 0x557650
     */
    static FontCharacter * get_character_metrics(uint16_t character, Font *font);

    /**
     * Measures a wide string by laying it out with a bounds-collecting glyph callback, writing the cursor rectangle at the
     * end of the text and the extents rectangle.
     *
     * @address 0x5562d0
     */
    static void measure_string_extents(Rectangle2D *origin_bounds, Rectangle2D *out_cursor_rect, Rectangle2D *out_extents_rect, void *string);

    /**
     * Walks a wide string accumulating glyph widths until the next glyph would not fit in *max_width_inout, reduces it by
     * the width consumed and returns the column at which the text may break.
     *
     * @address 0x557530
     */
    static int32_t measure_string_fit_width(void *string, int32_t *max_width_inout);

};

/**
 * Glyph callback used while measuring: grows the measurement bounds to include the glyph rectangle and records the
 * font.
 *
 * @address 0x556260
 */
void text_measure_glyph_callback(text_parse_state *state, void *font, void *character, uint32_t color, int16_t x, int16_t y, int16_t source_x, int16_t source_y, int16_t width, int16_t height);

}  // namespace halo::text
