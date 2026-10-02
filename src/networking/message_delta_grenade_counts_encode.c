// message_delta_grenade_counts_encode  (reached only through a .data code pointer; no C existed)
// address 0x4ea610, size 67 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ea610..0x4ea652: the two bytes as (a << 3) | b (sign-extended); unchanged: 0;
//   else 6 bits.
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t message_delta_grenade_counts_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    int8_t *counts = (int8_t *)current;
    int32_t packed = ((int32_t)counts[0] << 3) | (int32_t)counts[1];

    (void)field_type;
    if (previous != 0 && packed == ((((int32_t)((int8_t *)previous)[0]) << 3) | (int32_t)((int8_t *)previous)[1])) {
        return 0;
    }
    return bit_stream_write_bits_chunked(stream, (const uint32_t *)&packed, 6);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
