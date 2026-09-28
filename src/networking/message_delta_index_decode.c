// message_delta_index_decode  (reached only through a .data code pointer; no C existed)
// address 0x4e9bf0, size 46 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e9bf0..0x4e9c1d: reads the bits (descriptor +8) into a zeroed dword, stored in
//   full.
// blam-cc: cdecl

#include "message_delta_codec.h"

int32_t message_delta_index_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    uint32_t value = 0;
    int32_t bits = bit_stream_read_bits_chunked(descriptor[2], &value, stream);

    (void)previous;
    *(uint32_t *)current = value;
    return bits;
}
