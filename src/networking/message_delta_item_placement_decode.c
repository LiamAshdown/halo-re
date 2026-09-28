// message_delta_item_placement_decode  (reached only through a .data code pointer; no C existed)
// address 0x4ebc20, size 297 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ebc20..0x4ebd48: each axis back as v / levels * 10000 - 5000.
// blam-cc: cdecl

#include "message_delta_codec.h"

extern uint32_t item_placement_bits_x; // 0x0069a2e0
extern uint32_t item_placement_bits_y; // 0x0069a2e4
extern uint32_t item_placement_bits_z; // 0x0069a2e8

int32_t message_delta_item_placement_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    real *position = (real *)current;
    uint32_t value;
    int32_t total = 0;

    (void)field_type;
    (void)previous;
    value = 0;
    total += bit_stream_read_bits_chunked((int32_t)item_placement_bits_x, &value, stream);
    position[0] = (real)((double)value / (double)(uint32_t)((1 << item_placement_bits_x) - 1)) * 10000.0f - 5000.0f;
    value = 0;
    total += bit_stream_read_bits_chunked((int32_t)item_placement_bits_y, &value, stream);
    position[1] = (real)((double)value / (double)(uint32_t)((1 << item_placement_bits_y) - 1)) * 10000.0f - 5000.0f;
    value = 0;
    total += bit_stream_read_bits_chunked((int32_t)item_placement_bits_z, &value, stream);
    position[2] = (real)((double)value / (double)(uint32_t)((1 << item_placement_bits_z) - 1)) * 10000.0f - 5000.0f;
    return total;
}
