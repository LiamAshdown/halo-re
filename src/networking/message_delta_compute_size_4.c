// message_delta_compute_size_4  (reached only through a .data code pointer; no C existed)
// address 0x4eb150, size 6 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4eb150..0x4eb155: 4 bits (digital throttle).
// blam-cc: cdecl

#include "message_delta_codec.h"

int32_t message_delta_compute_size_4(message_delta_field_type *field_type)
{
    (void)field_type;
    return 4;
}
