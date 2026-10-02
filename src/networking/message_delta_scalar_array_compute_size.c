// message_delta_scalar_array_compute_size  (reached only through a .data code pointer; no C existed)
// address 0x4ea220, size 20 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ea220..0x4ea233: one flag per component (reserved bits = count) and 32 bits
//   each.
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t message_delta_scalar_array_compute_size(message_delta_field_type *field_type)
{
    int32_t count = *(int32_t *)field_type->array_descriptor;

    field_type->reserved_bits = count;
    return (count << 5) + count;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
