// message_delta_byte_encode  (reached only through a .data code pointer; no C existed)
// address 0x4e8d30, size 35 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e8d30..0x4e8d52: an unchanged byte: 0; else 8 bits.
// blam-cc: cdecl

#include "message_delta_codec.h"

int32_t message_delta_byte_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    (void)field_type;
    if (previous != 0 && *(uint8_t *)previous == *(uint8_t *)current) {
        return 0;
    }
    return bit_stream_write_bits_chunked(stream, (const uint32_t *)current, 8);
}
