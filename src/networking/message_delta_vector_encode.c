// message_delta_vector_encode  (reached only through a .data code pointer; no C existed)
// address 0x4ea240, size 5 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ea240..0x4ea244: jumps to message_delta_float_array_encode 0x4e9db0.
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t message_delta_float_array_encode(message_delta_field_type *field_type, float *previous, float *values,
    bit_stream *stream); // 0x4e9db0

int32_t message_delta_vector_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return message_delta_float_array_encode(field_type, (float *)previous, (float *)current, stream);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
