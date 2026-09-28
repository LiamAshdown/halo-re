// message_delta_weapon_index_decode  (reached only through a .data code pointer; no C existed)
// address 0x4eb280, size 60 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4eb280..0x4eb2bb: 3 bits: the high bit means -1, else the low bits.
// blam-cc: cdecl

#include "message_delta_codec.h"

int32_t message_delta_weapon_index_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint32_t code = 0;
    int32_t bits = bit_stream_read_bits_chunked(3, &code, stream);

    (void)field_type;
    (void)previous;
    if (code & 4) {
        *(int16_t *)current = -1;
    } else {
        *(int16_t *)current = (int16_t)(code & 3);
    }
    return bits;
}
