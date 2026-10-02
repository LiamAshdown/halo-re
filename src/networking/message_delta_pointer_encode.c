// message_delta_pointer_encode  (reached only through a .data code pointer; no C existed)
// address 0x4e9a30, size 46 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e9a30..0x4e9a5d: the pointed type codes through the stored pointers (previous:
//   its pointer, or NULL).
// blam-cc: cdecl

#include "message_delta_codec.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

int32_t message_delta_pointer_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    message_delta_field_type *pointed = *(message_delta_field_type **)field_type->array_descriptor;
    void *previous_value = previous != 0 ? *(void **)previous : 0;

    return MESSAGE_DELTA_ENCODE(pointed, previous_value, *(void **)current, stream);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
