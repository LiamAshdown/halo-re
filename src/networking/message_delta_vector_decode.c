// message_delta_vector_decode  (reached only through a .data code pointer; no C existed)
// address 0x4ea250, size 5 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ea250..0x4ea254: jumps to message_delta_dword_array_decode 0x4ea040.
// blam-cc: cdecl

#include "message_delta_codec.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t message_delta_dword_array_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream); // 0x4ea040

int32_t message_delta_vector_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    return message_delta_dword_array_decode(field_type, previous, current, stream);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
