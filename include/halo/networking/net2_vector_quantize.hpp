/**
 * @file include/halo/networking/net2_vector_quantize.hpp
 * Vector quantization and digital throttle helpers.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

namespace halo::networking {

/**
 * Vector quantization and digital throttle helpers.
 *
 * Static behaviour facade; the original C entry points forward to these members.
 */
class VectorQuantizer {
public:
    /**
     * Original engine function `digital_throttle_decode_vector`.
     *
     * @address 0x4eb0c0
     */
    static void decode_vector(real *out, uint32_t code);

    /**
     * Original engine function `digital_throttle_encode_vector`.
     *
     * @address 0x4eb050
     */
    static int32_t encode_vector(real_vector3d vector);

    /**
     * Computes a direction vector from yaw/pitch: x = cos(pitch)*sin(yaw), y = sin(pitch)*sin(yaw), z = cos(yaw) -- an unusual pairing (z from yaw's cosine, not pitch's), transcribed exactly as Ghidra shows it rather than the more common spherical-to-cartesian convention.
     *
     * @address 0x4ea7d0
     */
    static void from_yaw_pitch(real_vector3d *out_direction, real yaw, real pitch);

    /**
     * Interpolates out_point[i] = (table->maximum - table->minimum) * (ratios[i] / denom) + table->minimum for i in 0..2, where denom is table->denominator_mode0 or table->denominator_mode1 depending on message_delta_vector3d_mode.
     *
     * @address 0x4eb370
     */
    static void lerp_by_mode_denominator(vector3d_lerp_table *table, real_vector3d *out_point,
    int32_t *ratios);

    /**
     * Original engine function `vector3d_quantize`.
     *
     * @address 0x4eb4a0
     */
    static void quantize(int32_t *out_indices, int32_t *descriptor, real *point);

    /**
     * Original engine function `vector3d_to_angles`.
     *
     * @address 0x4ea720
     */
    static void to_angles(real *out, real_vector3d vector);

    /**
     * Original engine function `waypoint_table_quantize_initialize`.
     *
     * @address 0x4eb560
     */
    static uint8_t quantize_initialize(message_delta_field_type *field_type);

};

}  // namespace halo::networking
