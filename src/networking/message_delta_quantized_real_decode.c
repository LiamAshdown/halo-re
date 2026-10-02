// message_delta_quantized_real_decode  (reached only through a .data code pointer; no C existed)
// address 0x4ea5b0, size 79 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ea5b0..0x4ea5fe: level / levels (both unsigned) from the descriptor  bits.
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t message_delta_quantized_real_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint32_t *descriptor = (uint32_t *)field_type->array_descriptor;
    uint32_t level = 0;
    int32_t bits = bit_stream_read_bits_chunked((int32_t)descriptor[0], &level, stream);

    (void)previous;
    *(real *)current = (real)((double)level / (double)descriptor[1]);
    return bits;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
