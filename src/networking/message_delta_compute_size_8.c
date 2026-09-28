// message_delta_compute_size_8  (reached only through a .data code pointer; no C existed)
// address 0x4e8d20, size 6 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e8d20..0x4e8d25: 8 bits (the byte kind).
// blam-cc: cdecl

#include "message_delta_codec.h"

int32_t message_delta_compute_size_8(message_delta_field_type *field_type)
{
    (void)field_type;
    return 8;
}
