// message_delta_normal_decode  (reached only through a .data code pointer; no C existed)
// address 0x4eaa70, size 233 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4eaa70..0x4eab58: both levels (bits by connection mode) back to angles -- a /
//   levels * pi and b / levels * 2pi - pi/2 -- and vector3d_from_yaw_pitch into the destination.
// blam-cc: cdecl

#include "message_delta_codec.h"

extern uint32_t message_delta_vector3d_mode; // 0x0069b350, nonzero picks the first bit widths

extern void vector3d_from_yaw_pitch(real_vector3d *out_direction, real yaw, real pitch); // 0x4ea7d0, ECX out

int32_t message_delta_normal_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    int32_t bits_a = message_delta_vector3d_mode != 0 ? descriptor[0] : descriptor[2];
    int32_t bits_b = message_delta_vector3d_mode != 0 ? descriptor[1] : descriptor[3];
    uint32_t value_a = 0;
    uint32_t value_b = 0;
    int32_t bits;
    real angle_a;
    real angle_b;

    (void)previous;
    bits = bit_stream_read_bits_chunked(bits_a, &value_a, stream);
    bits += bit_stream_read_bits_chunked(bits_b, &value_b, stream);
    angle_a = (real)((double)value_a / (double)(uint32_t)((1 << bits_a) - 1)) * 3.1415927f;
    angle_b = (real)((double)value_b / (double)(uint32_t)((1 << bits_b) - 1)) * 6.2831855f - 1.5707964f;
    vector3d_from_yaw_pitch((real_vector3d *)current, angle_a, angle_b);
    return bits;
}
