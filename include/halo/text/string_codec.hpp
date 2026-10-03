/**
 * @file include/halo/text/string_codec.hpp
 * String conversion and lookup helpers.
 */
#pragma once

#include "halo/text/text_types.hpp"

namespace halo::text {

/**
 * Stateless conversions between narrow and UTF-16 strings and string table lookup.
 */
struct string_codec {
    /**
     * Widens a narrow string into a UTF-16 buffer of the given capacity in bytes and returns the destination.
     *
     * @address 0x557990
     */
    static uint16_t * ascii_to_unicode(uint16_t *dst, uint32_t capacity_bytes, const char *source);

    /**
     * Copies a UTF-16 string into a narrow buffer, replacing every wide character with a nonzero high byte by a space.
     * Returns dest, or NULL (leaving dest untouched) when the source does not fit.
     *
     * @address 0x557950
     */
    static uint8_t * unicode_to_ascii(uint8_t *dest, uint16_t *source, int32_t capacity);

    /**
     * Returns the index of the first table entry equal to the search string, or -1 when none of the count entries matches.
     *
     * @address 0x4875c0
     */
    static int16_t table_index_of(const char *search, int16_t count, const char **table);

};

}  // namespace halo::text
