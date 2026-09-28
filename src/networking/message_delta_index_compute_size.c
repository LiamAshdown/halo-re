// message_delta_index_compute_size  (reached only through a .data code pointer; no C existed)
// address 0x4e9af0, size 20 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e9af0..0x4e9b03: the bits for the count, kept in the descriptor (+8) and
//   returned.
// blam-cc: cdecl

#include "message_delta_codec.h"

extern uint8_t message_delta_item_count_bits[]; // 0x0065d51f

int32_t message_delta_index_compute_size(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    descriptor[2] = message_delta_item_count_bits[descriptor[0] + 1];
    return descriptor[2];
}
