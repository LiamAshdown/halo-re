// message_delta_item_placement_encode  (reached only through a .data code pointer; no C existed)
// address 0x4ebab0, size 354 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ebab0..0x4ebc11: each axis of the position as floor((v + 5000) / 10000 *
//   levels + 0.5) clamped, at its width; always sent (the previous value is not looked at).
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint32_t item_placement_bits_x; // 0x0069a2e0
extern uint32_t item_placement_bits_y; // 0x0069a2e4
extern uint32_t item_placement_bits_z; // 0x0069a2e8
extern double floor(double x);

int32_t message_delta_item_placement_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    real *position = (real *)current;
    int32_t total = 0;

    (void)field_type;
    (void)previous;
    {
        uint32_t levels = (uint32_t)((1 << item_placement_bits_x) - 1);
        uint32_t level = (uint32_t)(int64_t)floor((double)((real)levels * ((position[0] - -5000.0f) * 0.0001f) + 0.5f));

        if (level > levels) {
            level = levels;
        }
        total += bit_stream_write_bits_chunked(stream, &level, (int32_t)item_placement_bits_x);
    }
    {
        uint32_t levels = (uint32_t)((1 << item_placement_bits_y) - 1);
        uint32_t level = (uint32_t)(int64_t)floor((double)((real)levels * ((position[1] - -5000.0f) * 0.0001f) + 0.5f));

        if (level > levels) {
            level = levels;
        }
        total += bit_stream_write_bits_chunked(stream, &level, (int32_t)item_placement_bits_y);
    }
    {
        uint32_t levels = (uint32_t)((1 << item_placement_bits_z) - 1);
        uint32_t level = (uint32_t)(int64_t)floor((double)((real)levels * ((position[2] - -5000.0f) * 0.0001f) + 0.5f));

        if (level > levels) {
            level = levels;
        }
        total += bit_stream_write_bits_chunked(stream, &level, (int32_t)item_placement_bits_z);
    }
    return total;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
