// message_delta_wide_string_decode  (reached only through a .data code pointer; no C existed)
// address 0x4e9020, size 114 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e9020..0x4e9091: the length; within 0..count, that many 16-bit characters and
//   a NUL.
// blam-cc: cdecl

#include "message_delta_codec.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

int32_t message_delta_wide_string_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    uint16_t *string = (uint16_t *)current;
    int32_t length = 0;
    int32_t bits = bit_stream_read_bits_chunked(field_type->reserved_bits, (uint32_t *)&length, stream);
    int32_t i;

    (void)previous;
    if (length < 0 || length > descriptor[0]) {
        return bits;
    }
    for (i = 0; i < length; i++) {
        bits += bit_stream_read_bits_chunked(0x10, (uint32_t *)(string + i), stream);
    }
    string[length] = 0;
    return bits;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
