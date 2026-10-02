// message_delta_structure_array_compute_size  (reached only through a .data code pointer; no C existed)
// address 0x4e90b0, size 53 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e90b0..0x4e90e4: the element type  size (cached in it); one changed flag per
//   element (reserved bits = count); count * element + count.
// blam-cc: cdecl

#include "message_delta_codec.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

int32_t message_delta_structure_array_compute_size(message_delta_field_type *field_type)
{
    message_delta_array_descriptor *descriptor = (message_delta_array_descriptor *)field_type->array_descriptor;
    int32_t count = descriptor->count;
    int32_t element_bits = MESSAGE_DELTA_COMPUTE_SIZE(descriptor->field_type);

    descriptor->field_type->size_bits = element_bits;
    field_type->reserved_bits = count;
    return descriptor->count * element_bits + count;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
