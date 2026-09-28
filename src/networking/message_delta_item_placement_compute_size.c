// message_delta_item_placement_compute_size  (reached only through a .data code pointer; no C existed)
// address 0x4eba40, size 20 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4eba40..0x4eba53: the three component widths summed.
// blam-cc: cdecl

#include "message_delta_codec.h"

extern uint32_t item_placement_bits_x; // 0x0069a2e0
extern uint32_t item_placement_bits_y; // 0x0069a2e4
extern uint32_t item_placement_bits_z; // 0x0069a2e8

int32_t message_delta_item_placement_compute_size(message_delta_field_type *field_type)
{
    (void)field_type;
    return (int32_t)(item_placement_bits_y + item_placement_bits_z + item_placement_bits_x);
}
