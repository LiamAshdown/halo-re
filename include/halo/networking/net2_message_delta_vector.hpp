/**
 * @file include/halo/networking/net2_message_delta_vector.hpp
 * Vector, normal, throttle and quantized real message-delta field codecs.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

namespace halo::networking {

/**
 * Vector, normal, throttle and quantized real message-delta field codecs.
 *
 * Static behaviour facade; the original C entry points forward to these members.
 */
class VectorFieldCodec {
public:
    /**
     * If mode is nonzero, decodes three raw chunked values into ratios and interpolates directly via vector3d_lerp_by_mode_denominator, returning the bits consumed. Otherwise decodes a unary-coded index (one bit at a time, directly from the stream's byte buffer) into table's point array (stride 0xc, at +0x1c) and copies that entry into destination; if the index never resolves...
     *
     * @address 0x4eb890
     */
    static int32_t decode_vector3d_indexed(int32_t param_1, int32_t mode, real *destination,
    bit_stream *stream);

    /**
     * Encodes a 3D position (values) either as a small quantized delta from previous (if the distance between them is within range and each axis's delta fits the configured delta bit width), or as a full quantized absolute position otherwise. previous may be NULL to force the absolute path.
     *
     * @address 0x4eabe0
     */
    static int32_t encode_vector3d(int32_t unused, real *previous, real *values,
    bit_stream *stream);

    /**
     * Original engine function `message_delta_locality_compute_size`.
     *
     * @address 0x4eab60
     */
    static int32_t locality_compute_size(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_locality_decode`.
     *
     * @address 0x4eaed0
     */
    static int32_t locality_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_locality_initialize`.
     *
     * @address 0x4eab80
     */
    static uint8_t locality_initialize(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_normal_compute_size`.
     *
     * @address 0x4ea6a0
     */
    static int32_t normal_compute_size(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_normal_decode`.
     *
     * @address 0x4eaa70
     */
    static int32_t normal_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_normal_encode`.
     *
     * @address 0x4ea8b0
     */
    static int32_t normal_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_normal_initialize`.
     *
     * @address 0x4ea6c0
     */
    static uint8_t normal_initialize(message_delta_field_type *field_type);

    /**
     * Maps value from [minimum, maximum] onto an integer index in [0, max_level], rounding to the nearest level and clamping the result to max_level.
     *
     * @address 0x4ea480
     */
    static uint32_t quantize_float_to_int(uint32_t max_level, real value, real minimum,
    real maximum);

    /**
     * Original engine function `message_delta_quantized_real_decode`.
     *
     * @address 0x4ea5b0
     */
    static int32_t quantized_real_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_quantized_real_encode`.
     *
     * @address 0x4ea500
     */
    static int32_t quantized_real_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_quantized_real_initialize`.
     *
     * @address 0x4ea4e0
     */
    static uint8_t quantized_real_initialize(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_real_encode`.
     *
     * @address 0x4e8c70
     */
    static int32_t real_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_throttle_decode`.
     *
     * @address 0x4eb1d0
     */
    static int32_t throttle_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_throttle_encode`.
     *
     * @address 0x4eb160
     */
    static int32_t throttle_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_vector_decode`.
     *
     * @address 0x4ea250
     */
    static int32_t vector_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_vector_encode`.
     *
     * @address 0x4ea240
     */
    static int32_t vector_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_velocity_compute_size`.
     *
     * @address 0x4eb520
     */
    static int32_t velocity_compute_size(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_velocity_encode`.
     *
     * @address 0x4eb680
     */
    static int32_t velocity_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

};

}  // namespace halo::networking
