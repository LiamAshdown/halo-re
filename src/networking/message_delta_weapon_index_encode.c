// message_delta_weapon_index_encode  (reached only through a .data code pointer; no C existed)
// address 0x4eb220, size 89 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4eb220..0x4eb278: the short as ((index == -1) << 2) | (index & 3); unchanged:
//   0; else 3 bits.
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t message_delta_weapon_index_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint16_t index = *(uint16_t *)current;
    int32_t code = ((index == 0xffff) << 2) | (index & 3);

    (void)field_type;
    if (previous != 0) {
        int16_t previous_index = *(int16_t *)previous;

        if (code == ((((uint16_t)previous_index == 0xffff) << 2) | (previous_index & 3))) {
            return 0;
        }
    }
    return bit_stream_write_bits_chunked(stream, (const uint32_t *)&code, 3);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
