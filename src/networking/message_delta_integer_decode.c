// message_delta_integer_decode  (reached only through a .data code pointer; no C existed)
// address 0x4e8b70, size 200 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e8b70..0x4e8c37: reads the subtype width into the destination (the packed
//   widths clear the byte first); subtypes above 6: 0.
// blam-cc: cdecl

#include "message_delta_codec.h"

int32_t message_delta_integer_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    (void)previous;
    switch (*(int32_t *)field_type->array_descriptor) {
    case 0:
        return bit_stream_read_bits_chunked(8, (uint32_t *)current, stream);
    case 1:
        return bit_stream_read_bits_chunked(16, (uint32_t *)current, stream);
    case 2:
        return bit_stream_read_bits_chunked(32, (uint32_t *)current, stream);
    case 3:
        *(uint8_t *)current = 0;
        return bit_stream_read_bits_chunked(1, (uint32_t *)current, stream);
    case 4:
        *(uint8_t *)current = 0;
        return bit_stream_read_bits_chunked(3, (uint32_t *)current, stream);
    case 5:
        *(uint8_t *)current = 0;
        return bit_stream_read_bits_chunked(5, (uint32_t *)current, stream);
    case 6:
        *(uint8_t *)current = 0;
        return bit_stream_read_bits_chunked(6, (uint32_t *)current, stream);
    }
    return 0;
}
