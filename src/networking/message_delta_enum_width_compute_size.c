// message_delta_enum_width_compute_size  (reached only through a .data code pointer; no C existed)
// address 0x4e9a90, size 35 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e9a90..0x4e9ab2: subtype 0: 1 bit, 1: 2 bits, else 4.
// blam-cc: cdecl

#include "message_delta_codec.h"

int32_t message_delta_enum_width_compute_size(message_delta_field_type *field_type)
{
    switch (*(int32_t *)field_type->array_descriptor) {
    case 0:
        return 1;
    case 1:
        return 2;
    default:
        return 4;
    }
}
