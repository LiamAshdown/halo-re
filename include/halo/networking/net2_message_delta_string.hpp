/**
 * @file include/halo/networking/net2_message_delta_string.hpp
 * String and blob message-delta field codecs.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

namespace halo::networking {

/**
 * String and blob message-delta field codecs.
 *
 * Static behaviour facade; the original C entry points forward to these members.
 */
class StringFieldCodec {
public:
    /**
     * Original engine function `message_delta_blob_compute_size`.
     *
     * @address 0x4e90a0
     */
    static int32_t blob_compute_size(message_delta_field_type *field_type);

    /**
     * 0x0065d51f
     *
     * @address 0x4e8d90
     */
    static int32_t string_compute_size(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_string_decode`.
     *
     * @address 0x4e8ec0
     */
    static int32_t string_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_string_encode`.
     *
     * @address 0x4e8dc0
     */
    static int32_t string_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * 0x0065d51f
     *
     * @address 0x4e8f30
     */
    static int32_t wide_string_compute_size(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_wide_string_decode`.
     *
     * @address 0x4e9020
     */
    static int32_t wide_string_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_wide_string_encode`.
     *
     * @address 0x4e8f50
     */
    static int32_t wide_string_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

};

}  // namespace halo::networking
