/**
 * @file src/text/text_api.cpp
 * The text module's free-function API (include/halo/text/api.hpp), forwarding to the class implementations.
 */

#include <stdarg.h>
#include "halo/text/text.hpp"
#include "halo/text/api.hpp"


namespace halo::text {


void text_clamp_byte_length_to_character_boundary(uint8_t *string, int16_t *length_inout)
{
    halo::text::dbcs_text::clamp_byte_length_to_character_boundary(string, length_inout);
}

uint16_t * string_convert_ascii_to_unicode(uint16_t *dst, uint32_t capacity_bytes, const char *source)
{
    return halo::text::string_codec::ascii_to_unicode(dst, capacity_bytes, source);
}

uint8_t * string_convert_unicode_to_ascii(uint8_t *dest, uint16_t *source, int32_t capacity)
{
    return halo::text::string_codec::unicode_to_ascii(dest, source, capacity);
}

uint16_t * text_string_list_get_string(datum_index list_id, int16_t index)
{
    return halo::text::text_context::string_list_get_string(list_id, index);
}

void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...)
{
    va_list args;

    va_start(args, format);
    halo::text::wide_string_format::format_v(dest, format, args);
    va_end(args);
}

static_assert(sizeof(wchar_t) == sizeof(uint16_t), "wide strings are UTF-16 code units");

void string_format_wide_va(wchar_t *dest, const wchar_t *format, ...)
{
    va_list args;

    va_start(args, format);
    halo::text::wide_string_format::format_v(reinterpret_cast<uint16_t *>(dest), reinterpret_cast<const uint16_t *>(format), args);
    va_end(args);
}

void string_format_wide_va_bounded(uint32_t count, wchar_t *dest, const wchar_t *format, ...)
{
    va_list args;

    va_start(args, format);
    halo::text::wide_string_format::format_bounded_v(count, reinterpret_cast<uint16_t *>(dest), reinterpret_cast<const uint16_t *>(format), args);
    va_end(args);
}

void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...)
{
    va_list args;

    va_start(args, format);
    halo::text::wide_string_format::format_bounded_v(count, dest, format, args);
    va_end(args);
}

}
