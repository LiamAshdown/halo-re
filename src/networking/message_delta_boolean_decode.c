// message_delta_boolean_decode  (reached only through a .data code pointer; no C existed)
// address 0x4e8d00, size 21 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e8d00..0x4e8d14: clears the byte and reads one bit into it.
// blam-cc: cdecl

#include "message_delta_codec.h"

int32_t message_delta_boolean_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    (void)field_type;
    (void)previous;
    *(uint8_t *)current = 0;
    return (int32_t)bit_stream_read_bit((uint8_t *)current, stream);
}
