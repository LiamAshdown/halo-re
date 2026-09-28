// message_delta_string_encode  (reached only through a .data code pointer; no C existed)
// address 0x4e8dc0, size 242 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e8dc0..0x4e8eb1: unchanged (strcmp): 0; else the length in the reserved bits
//   and 8 bits per character.
// blam-cc: cdecl

#include "message_delta_codec.h"

int32_t message_delta_string_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    const char *string = (const char *)current;
    int32_t length = (int32_t)strlen(string);
    int32_t bits;
    int32_t i;

    if (previous != 0 && strcmp((const char *)previous, string) == 0) {
        return 0;
    }
    bits = bit_stream_write_bits_chunked(stream, (const uint32_t *)&length, field_type->reserved_bits);
    for (i = 0; i < length; i++) {
        bits += bit_stream_write_bits_chunked(stream, (const uint32_t *)(string + i), 8);
    }
    return bits;
}
