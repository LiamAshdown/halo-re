// message_delta_normal_encode  (reached only through a .data code pointer; no C existed)
// address 0x4ea8b0, size 441 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4ea8b0..0x4eaa68: the direction as angles: acos(z) / pi and (yaw + pi/2) / 2pi,
//   each times its level count (2^bits - 1, bits by connection mode) + 0.5, floored and clamped; unchanged from the
//   previous direction quantized over [0, pi] and [-pi/2, 3pi/2]: 0; else both levels.
// blam-cc: cdecl

#include "message_delta_codec.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint32_t message_delta_vector3d_mode; // 0x0069b350, nonzero picks the first bit widths
extern double floor(double x);
extern uint32_t message_delta_quantize_float_to_int(uint32_t max_level, real value, real minimum, real maximum); // 0x4ea480, ESI max_level

extern void vector3d_to_angles(real *out, real_vector3d vector); // 0x4ea720

int32_t message_delta_normal_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream)
{
    int32_t *descriptor = (int32_t *)field_type->array_descriptor;
    int32_t bits_a = message_delta_vector3d_mode != 0 ? descriptor[0] : descriptor[2];
    int32_t bits_b = message_delta_vector3d_mode != 0 ? descriptor[1] : descriptor[3];
    uint32_t levels_a = (uint32_t)((1 << bits_a) - 1);
    uint32_t levels_b = (uint32_t)((1 << bits_b) - 1);
    real angles[2];
    uint32_t level_a;
    uint32_t level_b;

    vector3d_to_angles(angles, *(real_vector3d *)current);
    level_a = (uint32_t)(int64_t)floor((double)(angles[0] * 0.31830987f * (real)levels_a + 0.5f));
    if (level_a > levels_a) {
        level_a = levels_a;
    }
    level_b = (uint32_t)(int64_t)floor((double)((angles[1] - -1.5707964f) * 0.15915494f * (real)levels_b + 0.5f));
    if (level_b > levels_b) {
        level_b = levels_b;
    }
    if (previous != 0) {
        uint32_t previous_a;

        vector3d_to_angles(angles, *(real_vector3d *)previous);
        previous_a = message_delta_quantize_float_to_int(levels_a, angles[0], 0.0f, 3.1415927f);
        if (level_a == previous_a &&
            level_b == message_delta_quantize_float_to_int(levels_b, angles[1], -1.5707964f, 4.712389f)) {
            return 0;
        }
    }
    return bit_stream_write_bits_chunked(stream, &level_a, bits_a) + bit_stream_write_bits_chunked(stream, &level_b, bits_b);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
