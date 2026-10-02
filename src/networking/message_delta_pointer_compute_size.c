// message_delta_pointer_compute_size  (reached only through a .data code pointer; no C existed)
// address 0x4e99d0, size 33 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e99d0..0x4e99f0: the pointed type  size, cached in it.
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t message_delta_pointer_compute_size(message_delta_field_type *field_type)
{
    message_delta_field_type **descriptor = (message_delta_field_type **)field_type->array_descriptor;
    int32_t bits = MESSAGE_DELTA_COMPUTE_SIZE(descriptor[0]);

    descriptor[0]->size_bits = bits;
    return bits;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
