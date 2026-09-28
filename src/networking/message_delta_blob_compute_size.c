// message_delta_blob_compute_size  (reached only through a .data code pointer; no C existed)
// address 0x4e90a0, size 13 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4e90a0..0x4e90ac: 8 bits per byte of the count.
// blam-cc: cdecl

#include "message_delta_codec.h"

int32_t message_delta_blob_compute_size(message_delta_field_type *field_type)
{
    return *(int32_t *)field_type->array_descriptor << 3;
}
