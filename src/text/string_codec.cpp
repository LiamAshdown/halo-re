/**
 * @file src/text/string_codec.cpp
 * String conversion and lookup helpers.
 * The original author notes and decompiles are in docs/original/text/.
 */

#include "halo/text/text.hpp"

namespace halo::text {

uint16_t * string_codec::ascii_to_unicode(uint16_t *dst, uint32_t capacity_bytes, const char *source)
{
    int32_t count;
    int32_t i;

    count = (int32_t)strlen(source);
    if (capacity_bytes < (uint32_t)count * 2 + 2) {
        count = (int32_t)(capacity_bytes >> 1) - 1;
    }
    if ((uint32_t)count * 2 + 2 > capacity_bytes) {
        return 0;
    }
    dst[count] = 0;
    for (i = count - 1; i >= 0; i--) {
        dst[i] = (uint16_t)(uint8_t)source[i];
    }
    return dst;
}

uint8_t * string_codec::unicode_to_ascii(uint8_t *dest, uint16_t *source, int32_t capacity)
{
    uint32_t length;
    uint32_t i;

    length = (uint32_t)wcslen((const wchar_t *)source);
    if (length > (uint32_t)(capacity - 1)) {
        return (uint8_t *)((void *)0);
    }
    for (i = 0; i < length; i++) {
        if ((source[i] & 0xff00) != 0) {
            dest[i] = 0x20;
        } else {
            dest[i] = (uint8_t)source[i];
        }
    }
    dest[i] = 0;
    return dest;
}

int16_t string_codec::table_index_of(const char *search, int16_t count, const char **table)
{
    int16_t index;

    for (index = 0; index < count; index++) {
        const unsigned char *a = (const unsigned char *)table[index];
        const unsigned char *b = (const unsigned char *)search;

        for (;;) {
            if (*a != *b) {
                break;
            }
            if (*a == 0) {
                return index;
            }
            a++;
            b++;
        }
    }
    return -1;
}

}  // namespace halo::text
