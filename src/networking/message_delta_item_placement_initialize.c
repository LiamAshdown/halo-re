// message_delta_item_placement_initialize  (reached only through a .data code pointer; no C existed)
// address 0x4eba60, size 72 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4eba60..0x4ebaa7: with the parameters protocol on registers the three widths
//   (ints, no scope); valid.
// blam-cc: cdecl

#include "message_delta_codec.h"

extern uint32_t item_placement_bits_x; // 0x0069a2e0
extern uint32_t item_placement_bits_y; // 0x0069a2e4
extern uint32_t item_placement_bits_z; // 0x0069a2e8
extern void message_delta_parameters_protocol_register(char *scope, char *name, int32_t type, void *value); // 0x4ebe00, EAX scope
extern uint8_t message_delta_parameters_enabled; // 0x0071cfa8

uint8_t message_delta_item_placement_initialize(message_delta_field_type *field_type)
{
    (void)field_type;
    if (message_delta_parameters_enabled == 1) {
        message_delta_parameters_protocol_register(0, "gITEM_PLACEMENT_BITS_X", 1, &item_placement_bits_x);
        message_delta_parameters_protocol_register(0, "gITEM_PLACEMENT_BITS_Y", 1, &item_placement_bits_y);
        message_delta_parameters_protocol_register(0, "gITEM_PLACEMENT_BITS_Z", 1, &item_placement_bits_z);
    }
    return 1;
}
