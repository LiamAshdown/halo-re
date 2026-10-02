// message_delta_string_compute_size  (reached only through a .data code pointer; no C existed)
// address 0x4e8d90, size 25 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e8d90..0x4e8da8: the length header (bits for 0..count) is the reserved-bit
//   count; plus 8 bits per character.
// blam-cc: cdecl

#include "message_delta_codec.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t message_delta_item_count_bits[]; // 0x0065d51f

int32_t message_delta_string_compute_size(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    field_type->reserved_bits = message_delta_item_count_bits[descriptor[0] + 1];
    return field_type->reserved_bits + descriptor[0] * 8;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
