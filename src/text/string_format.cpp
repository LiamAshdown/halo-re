/**
 * @file src/text/string_format.cpp
 * Wide-character formatting forwarded to the C runtime.
 */

#include "halo/core/crt.hpp"
#include "halo/text/text.hpp"

namespace halo::text {

void wide_string_format::format_v(uint16_t *dest, const uint16_t *format, va_list args)
{
    _vsnwprintf(reinterpret_cast<wchar_t *>(dest), static_cast<size_t>(-1), reinterpret_cast<const wchar_t *>(format), args);
}

void wide_string_format::format_bounded_v(uint32_t count, uint16_t *dest, const uint16_t *format, va_list args)
{
    _vsnwprintf(reinterpret_cast<wchar_t *>(dest), count, reinterpret_cast<const wchar_t *>(format), args);
}

}  // namespace halo::text
