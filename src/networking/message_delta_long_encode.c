// message_delta_long_encode  (reached only through a .data code pointer; no C existed)
// address 0x4ea430, size 35 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ea430..0x4ea452: an unchanged dword: 0; else 32 bits.
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t message_delta_long_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    (void)field_type;
    if (previous != 0 && *(uint32_t *)previous == *(uint32_t *)current) {
        return 0;
    }
    return bit_stream_write_bits_chunked(stream, (const uint32_t *)current, 0x20);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
