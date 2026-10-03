/**
 * @file include/halo/networking/net2_message_delta_scalar.hpp
 * Scalar and flag message-delta field codecs.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

namespace halo::networking {

/**
 * Scalar and flag message-delta field codecs.
 *
 * Static behaviour facade; the original C entry points forward to these members.
 */
class ScalarFieldCodec {
public:
    /**
     * Original engine function `message_delta_boolean_decode`.
     *
     * @address 0x4e8d00
     */
    static int32_t boolean_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_boolean_encode`.
     *
     * @address 0x4e8cc0
     */
    static int32_t boolean_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_byte_decode`.
     *
     * @address 0x4e8d60
     */
    static int32_t byte_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_byte_encode`.
     *
     * @address 0x4e8d30
     */
    static int32_t byte_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_compute_size_1`.
     *
     * @address 0x4e8cb0
     */
    static int32_t compute_size_1(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_compute_size_16`.
     *
     * @address 0x4e8d80
     */
    static int32_t compute_size_16(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_compute_size_2`.
     *
     * @address 0x4eb2c0
     */
    static int32_t compute_size_2(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_compute_size_3`.
     *
     * @address 0x4eb210
     */
    static int32_t compute_size_3(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_compute_size_32`.
     *
     * @address 0x4e8c60
     */
    static int32_t compute_size_32(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_compute_size_4`.
     *
     * @address 0x4eb150
     */
    static int32_t compute_size_4(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_compute_size_6`.
     *
     * @address 0x4ea600
     */
    static int32_t compute_size_6(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_compute_size_8`.
     *
     * @address 0x4e8d20
     */
    static int32_t compute_size_8(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_flags_decode`.
     *
     * @address 0x4ea3b0
     */
    static int32_t flags_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_flags_encode`.
     *
     * @address 0x4ea2b0
     */
    static int32_t flags_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_flags_initialize`.
     *
     * @address 0x4ea260
     */
    static uint8_t flags_initialize(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_integer_compute_size`.
     *
     * @address 0x4e89c0
     */
    static int32_t integer_compute_size(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_integer_decode`.
     *
     * @address 0x4e8b70
     */
    static int32_t integer_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_integer_encode`.
     *
     * @address 0x4e8a40
     */
    static int32_t integer_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_integer_initialize`.
     *
     * @address 0x4e8a20
     */
    static uint8_t integer_initialize(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_long_decode`.
     *
     * @address 0x4ea460
     */
    static int32_t long_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_long_encode`.
     *
     * @address 0x4ea430
     */
    static int32_t long_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

};

}  // namespace halo::networking
