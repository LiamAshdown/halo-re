// message_delta_compute_size_16  (reached only through a .data code pointer; no C existed)
// address 0x4e8d80, size 6 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e8d80..0x4e8d85: 16 bits (the short kind).
// blam-cc: cdecl

#include "message_delta_codec.h"

int32_t message_delta_compute_size_16(message_delta_field_type *field_type)
{
    (void)field_type;
    return 16;
}
