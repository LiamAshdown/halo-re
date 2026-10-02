// message_delta_grenade_index_decode  (reached only through a .data code pointer; no C existed)
// address 0x4eb330, size 60 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4eb330..0x4eb36b: 2 bits: the high bit means -1, else the low bits.
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t message_delta_grenade_index_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint32_t code = 0;
    int32_t bits = bit_stream_read_bits_chunked(2, &code, stream);

    (void)field_type;
    (void)previous;
    if (code & 2) {
        *(int16_t *)current = -1;
    } else {
        *(int16_t *)current = (int16_t)(code & 1);
    }
    return bits;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
