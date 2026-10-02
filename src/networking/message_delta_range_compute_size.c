// message_delta_range_compute_size  (reached only through a .data code pointer; no C existed)
// address 0x4e9ac0, size 20 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e9ac0..0x4e9ad3: the bits for maximum - minimum (item count table).
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t message_delta_item_count_bits[]; // 0x0065d51f

int32_t message_delta_range_compute_size(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    return message_delta_item_count_bits[descriptor[1] - descriptor[0] + 1];
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
