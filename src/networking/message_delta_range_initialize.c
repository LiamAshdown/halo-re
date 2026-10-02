// message_delta_range_initialize  (reached only through a .data code pointer; no C existed)
// address 0x4e9ae0, size 16 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e9ae0..0x4e9aef: maximum above minimum.
// blam-cc: cdecl

#include "message_delta_codec.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

uint8_t message_delta_range_initialize(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    return descriptor[1] > descriptor[0];
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
