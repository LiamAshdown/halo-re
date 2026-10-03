/**
 * @file include/halo/text/dbcs_text.hpp
 * Character-boundary logic for the narrow text encodings (single byte, DBCS code pages and |x markup escapes).
 */
#pragma once

#include "halo/text/text_types.hpp"

namespace halo::text {

/**
 * Stateless helpers that walk narrow strings one character at a time, honouring the active DBCS code page and the
 * two-byte markup escapes.
 */
struct dbcs_text {
    /**
     * Classifies the byte pair at the string: true when the two bytes must be consumed together, either a '|x' markup
     * escape or a lead/trail pair of the active DBCS code page. False for ordinary single-byte characters, NUL, or an
     * encoding with no code page.
     *
     * @address 0x557750
     */
    static uint8_t char_is_double_byte(uint8_t *string);

    /**
     * Reads the character at string + *cursor, two bytes (lead << 8 | trail) for a double-byte pair and one byte
     * otherwise, and advances *cursor by the bytes consumed.
     *
     * @address 0x5576a0
     */
    static uint16_t get_next_character(uint8_t *string, int16_t *cursor);

    /**
     * Searches the string, one or two bytes per character, for the target character. Returns true if it occurs before the
     * terminating NUL.
     *
     * @address 0x557870
     */
    static uint8_t find_character(int16_t target_character, uint8_t *string);

    /**
     * Walks the string until the running offset would reach *length_inout, rewrites it with the offset of the last
     * character boundary at or before that length and returns the character at that boundary.
     *
     * @address 0x5576d0
     */
    static uint16_t find_character_boundary(uint8_t *string, int16_t *length_inout);

    /**
     * Rewrites *length_inout with the first character boundary at or after its original value, so a length that splits a
     * double-byte pair is rounded up by one. A length of 0 or less becomes 0.
     *
     * @address 0x557720
     */
    static void clamp_byte_length_to_character_boundary(uint8_t *string, int16_t *length_inout);

};

}  // namespace halo::text
