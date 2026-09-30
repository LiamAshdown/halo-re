// message_delta_throttle_decode  (reached only through a .data code pointer; no C existed)
// address 0x4eb1d0, size 52 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4eb1d0..0x4eb203: 4 bits back into a unit throttle vector.
// blam-cc: cdecl

#include "message_delta_codec.h"
#include "fn_networking.h"


int32_t message_delta_throttle_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint32_t code = 0;
    int32_t bits = bit_stream_read_bits_chunked(4, &code, stream);

    (void)field_type;
    (void)previous;
    digital_throttle_decode_vector((real *)current, code);
    return bits;
}
