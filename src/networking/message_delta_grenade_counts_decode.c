// message_delta_grenade_counts_decode  (reached only through a .data code pointer; no C existed)
// address 0x4ea660, size 56 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ea660..0x4ea697: 6 bits: the high 3 into byte 0, the low 3 into byte 1.
// blam-cc: cdecl

#include "message_delta_codec.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

int32_t message_delta_grenade_counts_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint32_t packed = 0;
    int32_t bits = bit_stream_read_bits_chunked(6, &packed, stream);

    (void)field_type;
    (void)previous;
    ((uint8_t *)current)[0] = (uint8_t)(packed >> 3);
    ((uint8_t *)current)[1] = (uint8_t)(packed & 7);
    return bits;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
