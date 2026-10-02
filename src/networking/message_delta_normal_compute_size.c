// message_delta_normal_compute_size  (reached only through a .data code pointer; no C existed)
// address 0x4ea6a0, size 29 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ea6a0..0x4ea6bc: the larger of the two bit-width pairs (a + b, c + d).
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t message_delta_normal_compute_size(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    int32_t second = descriptor[2] + descriptor[3];
    int32_t first = descriptor[0] + descriptor[1];

    return second > first ? second : first;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
