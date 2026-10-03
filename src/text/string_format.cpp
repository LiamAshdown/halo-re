/**
 * @file src/text/string_format.cpp
 * Wide-character formatting forwarded to the C runtime.
 * The original author notes are in docs/original/text/.
 */

#include "halo/text/text.hpp"

extern "C" {
extern int _vswprintf(uint16_t *buffer, const uint16_t *format, va_list args);
extern int _vsnwprintf(uint16_t *buffer, uint32_t count, const uint16_t *format, va_list args);
}

namespace halo::text {

void wide_string_format::format_v(uint16_t *dest, const uint16_t *format, va_list args)
{
    _vswprintf(dest, format, args);
}

void wide_string_format::format_bounded_v(uint32_t count, uint16_t *dest, const uint16_t *format, va_list args)
{
    _vsnwprintf(dest, count, format, args);
}

}  // namespace halo::text
