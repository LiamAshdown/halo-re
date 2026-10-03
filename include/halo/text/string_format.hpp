/**
 * @file include/halo/text/string_format.hpp
 * Wide-character formatting forwarded to the C runtime.
 */
#pragma once

#include "halo/text/text_types.hpp"

namespace halo::text {

/**
 * Formatting entry points that take a va_list. The variadic C functions string_format_wide_va and
 * string_format_wide_va_bounded start the list and forward here.
 */
struct wide_string_format {
    /**
     * Forwards to the C runtime's unbounded wide vswprintf (the legacy form without a count).
     *
     * @address 0x00557930
     */
    static void format_v(uint16_t *dest, const uint16_t *format, va_list args);

    /**
     * Forwards to the C runtime's bounded wide vsnwprintf, writing at most count code units into dest.
     *
     * @address 0x00557910
     */
    static void format_bounded_v(uint32_t count, uint16_t *dest, const uint16_t *format, va_list args);
};

}  // namespace halo::text
