// message_delta_long_decode  (reached only through a .data code pointer; no C existed)
// address 0x4ea460, size 23 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ea460..0x4ea476: 32 bits into the dword.
// blam-cc: cdecl

#include "message_delta_codec.h"

int32_t message_delta_long_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    (void)field_type;
    (void)previous;
    return bit_stream_read_bits_chunked(0x20, (uint32_t *)current, stream);
}
