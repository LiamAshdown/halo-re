// message_delta_quantized_real_encode  (reached only through a .data code pointer; no C existed)
// address 0x4ea500, size 174 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ea500..0x4ea5ad: a real in [0,1] as floor(levels * value + 0.5) clamped to
//   levels (unsigned); unchanged from the previous value quantized the same way (message_delta_quantize_float_to_int
//   0..1): 0; else the level in the descriptor  bits.
// blam-cc: cdecl

#include "message_delta_codec.h"

extern double floor(double x);
extern uint32_t message_delta_quantize_float_to_int(uint32_t max_level, real value, real minimum, real maximum); // 0x4ea480

int32_t message_delta_quantized_real_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    uint32_t *descriptor = (uint32_t *)field_type->array_descriptor;
    uint32_t levels = descriptor[1];
    uint32_t level = (uint32_t)(int64_t)floor((double)((real)levels * *(real *)current + 0.5f));

    if (level > levels) {
        level = levels;
    }
    if (previous != 0 && level == message_delta_quantize_float_to_int(descriptor[1], *(real *)previous, 0.0f, 1.0f)) {
        return 0;
    }
    return bit_stream_write_bits_chunked(stream, &level, (int32_t)descriptor[0]);
}
