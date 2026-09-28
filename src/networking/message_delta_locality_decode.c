// message_delta_locality_decode  (reached only through a .data code pointer; no C existed)
// address 0x4eaed0, size 374 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4eaed0..0x4eb045: with a previous position a flag bit picks the form: 0 -- per
//   axis a sign bit and a delta-bits magnitude, v / levels * range (negated for the sign) added to the previous
//   position; 1 (or no previous) -- three absolute components at the mode  width, v / levels * 10000 - 5000. The bits
//   read.
// blam-cc: cdecl

#include "message_delta_codec.h"

extern uint32_t message_delta_vector3d_absolute_bits_mode1; // 0x0069a2cc
extern uint32_t message_delta_vector3d_delta_bits;          // 0x0069a2d0
extern real message_delta_vector3d_delta_range;             // 0x0069a2d4
extern real message_delta_vector3d_delta_epsilon;           // 0x0069a2d8
extern uint32_t message_delta_vector3d_absolute_bits_mode0; // 0x0069a2dc
extern uint32_t message_delta_vector3d_mode; // 0x0069b350, nonzero picks the first bit widths

int32_t message_delta_locality_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    real *destination = (real *)current;
    int32_t total = 0;
    uint32_t bits;
    int32_t i;

    (void)field_type;
    if (previous != 0) {
        uint8_t absolute;

        total = (int32_t)bit_stream_read_bit(&absolute, stream);
        if (!absolute) {
            uint8_t sign[3];
            real delta[3];

            for (i = 0; i < 3; i++) {
                uint32_t value = 0;
                int32_t sign_bits = (int32_t)bit_stream_read_bit(&sign[i], stream);
                int32_t value_bits = bit_stream_read_bits_chunked((int32_t)message_delta_vector3d_delta_bits, &value, stream);

                total += value_bits + sign_bits;
                delta[i] = (real)((double)value / (double)(uint32_t)((1 << message_delta_vector3d_delta_bits) - 1)) *
                           message_delta_vector3d_delta_range;
                if (sign[i]) {
                    delta[i] = -delta[i];
                }
            }
            destination[0] = delta[0] + ((real *)previous)[0];
            destination[1] = delta[1] + ((real *)previous)[1];
            destination[2] = delta[2] + ((real *)previous)[2];
            return total;
        }
    }
    bits = message_delta_vector3d_mode != 0 ? message_delta_vector3d_absolute_bits_mode1 : message_delta_vector3d_absolute_bits_mode0;
    for (i = 0; i < 3; i++) {
        uint32_t value = 0;

        total += bit_stream_read_bits_chunked((int32_t)bits, &value, stream);
        destination[i] = (real)((double)value / (double)(uint32_t)((1 << bits) - 1)) * 10000.0f - 5000.0f;
    }
    return total;
}
