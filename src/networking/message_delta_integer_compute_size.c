// message_delta_integer_compute_size  (reached only through a .data code pointer; no C existed)
// address 0x4e89c0, size 58 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e89c0..0x4e89f9: bits by the descriptor  subtype (no range check): 0 byte 8, 1
//   short 16, 2 long 32, 3..6 packed 1, 3, 5, 6.
// blam-cc: cdecl

#include "message_delta_codec.h"

int32_t message_delta_integer_compute_size(message_delta_field_type *field_type)
{
    switch (*(int32_t *)field_type->array_descriptor) {
    case 0:
        return 8;
    case 1:
        return 0x10;
    case 2:
        return 0x20;
    case 3:
        return 1;
    case 4:
        return 3;
    case 5:
        return 5;
    default:
        return 6;
    }
}
