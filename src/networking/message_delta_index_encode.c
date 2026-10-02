// message_delta_index_encode  (reached only through a .data code pointer; no C existed)
// address 0x4e9bc0, size 47 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e9bc0..0x4e9bee: an unchanged dword: 0; else its bits (descriptor +8).
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t message_delta_index_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    if (previous != 0 && *(uint32_t *)current == *(uint32_t *)previous) {
        return 0;
    }
    return bit_stream_write_bits_chunked(stream, (const uint32_t *)current, descriptor[2]);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
