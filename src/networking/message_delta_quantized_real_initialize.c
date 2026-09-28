// message_delta_quantized_real_initialize  (reached only through a .data code pointer; no C existed)
// address 0x4ea4e0, size 25 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ea4e0..0x4ea4f8: positive bits and levels.
// blam-cc: cdecl

#include "message_delta_codec.h"

uint8_t message_delta_quantized_real_initialize(message_delta_field_type *field_type)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;

    return descriptor[0] > 0 && descriptor[1] > 0;
}
