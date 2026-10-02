// message_delta_count_initialize  (reached only through a .data code pointer; no C existed)
// address 0x4e8db0, size 14 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e8db0..0x4e8dbd: a positive element count is valid (strings, blobs, points,
//   vectors).
// blam-cc: cdecl

#include "message_delta_codec.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

uint8_t message_delta_count_initialize(message_delta_field_type *field_type)
{
    return *(int32_t *)field_type->array_descriptor > 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
