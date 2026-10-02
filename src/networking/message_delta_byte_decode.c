// message_delta_byte_decode  (reached only through a .data code pointer; no C existed)
// address 0x4e8d60, size 23 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e8d60..0x4e8d76: 8 bits into the byte.
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t message_delta_byte_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    (void)field_type;
    (void)previous;
    return bit_stream_read_bits_chunked(8, (uint32_t *)current, stream);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
