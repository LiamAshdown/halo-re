// message_delta_velocity_encode  (reached only through a .data code pointer; no C existed)
// address 0x4eb680, size 519 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4eb680..0x4eb886: quantizes the vector (vector3d_quantize, bits by connection
//   mode). Without a previous value: a vector in the waypoint table (+0x19c) is sent as i + 1 one-bits from 0x65d438
//   and, with more than one waypoint, a terminating 0 written in place; any other vector is a 0 bit and three
//   components. With a previous value: unchanged quantized: 0; else just the three components.
// blam-cc: cdecl

#include "message_delta_codec.h"

extern uint32_t message_delta_vector3d_mode; // 0x0069b350, nonzero picks the first bit widths

extern void vector3d_quantize(int32_t *out_indices, int32_t *descriptor, real *point); // 0x4eb4a0
extern uint32_t message_delta_unary_ones[]; // 0x0065d438

static int32_t write_zero_bit(bit_stream *stream)
{
    uint32_t position = message_delta_stream_position(stream);

    if (position < stream->first_bit || position > stream->last_bit) {
        return 0;
    }
    stream->data[stream->byte_cursor] &= (uint8_t)~(1 << stream->bit_cursor);
    position = message_delta_stream_position(stream) + 1;
    if ((position >= stream->first_bit && position <= stream->last_bit) || position == stream->last_bit + 1) {
        stream->bit_cursor = position & 7;
        stream->byte_cursor = position >> 3;
    }
    return 1;
}

int32_t message_delta_velocity_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    int32_t bits = message_delta_vector3d_mode != 0 ? descriptor[2] : descriptor[4];
    int32_t quantized[3];
    int32_t total;

    vector3d_quantize(quantized, descriptor, (real *)current);
    if (previous == 0) {
        int32_t *table = descriptor + 0x67;
        int32_t i;

        for (i = 0; i < descriptor[6]; i++) {
            if (quantized[0] == table[i * 3] && quantized[1] == table[i * 3 + 1] && quantized[2] == table[i * 3 + 2]) {
                break;
            }
        }
        if (i < descriptor[6]) {
            total = bit_stream_write_bits_chunked(stream, message_delta_unary_ones, i + 1);
            if (descriptor[6] > 1) {
                total += write_zero_bit(stream);
            }
            return total;
        }
        total = write_zero_bit(stream);
    } else {
        int32_t old[3];

        vector3d_quantize(old, descriptor, (real *)previous);
        if (old[0] == quantized[0] && old[1] == quantized[1] && old[2] == quantized[2]) {
            return 0;
        }
        total = 0;
    }
    total += bit_stream_write_bits_chunked(stream, (const uint32_t *)&quantized[0], bits);
    total += bit_stream_write_bits_chunked(stream, (const uint32_t *)&quantized[1], bits);
    total += bit_stream_write_bits_chunked(stream, (const uint32_t *)&quantized[2], bits);
    return total;
}
