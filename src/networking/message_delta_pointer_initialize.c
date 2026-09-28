// message_delta_pointer_initialize  (reached only through a .data code pointer; no C existed)
// address 0x4e9a00, size 41 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e9a00..0x4e9a28: the pointed type exists and initializes.
// blam-cc: cdecl

#include "message_delta_codec.h"

uint8_t message_delta_pointer_initialize(message_delta_field_type *field_type)
{
    message_delta_field_type *pointed = *(message_delta_field_type **)field_type->array_descriptor;

    if (pointed == 0) {
        return 0;
    }
    return MESSAGE_DELTA_INITIALIZE(pointed) == 1;
}
