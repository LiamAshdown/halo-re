// message_delta_integer_initialize  (reached only through a .data code pointer; no C existed)
// address 0x4e8a20, size 24 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e8a20..0x4e8a37: a subtype in 0..27 is valid (also the initializer of kind
//   11).
// blam-cc: cdecl

#include "message_delta_codec.h"

uint8_t message_delta_integer_initialize(message_delta_field_type *field_type)
{
    int32_t subtype = *(int32_t *)field_type->array_descriptor;

    return subtype >= 0 && subtype < 0x1c;
}
