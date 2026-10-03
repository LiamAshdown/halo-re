/**
 * @file include/halo/networking/net2_message_delta_index.hpp
 * Index, pointer, placement and range message-delta field codecs.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

namespace halo::networking {

/**
 * Index, pointer, placement and range message-delta field codecs.
 *
 * Static behaviour facade; the original C entry points forward to these members.
 */
class IndexFieldCodec {
public:
    /**
     * Original engine function `message_delta_count_initialize`.
     *
     * @address 0x4e8db0
     */
    static uint8_t count_initialize(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_enum_width_compute_size`.
     *
     * @address 0x4e9a90
     */
    static int32_t enum_width_compute_size(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_first_dword_compute_size`.
     *
     * @address 0x4ea4d0
     */
    static int32_t first_dword_compute_size(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_grenade_counts_decode`.
     *
     * @address 0x4ea660
     */
    static int32_t grenade_counts_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_grenade_counts_encode`.
     *
     * @address 0x4ea610
     */
    static int32_t grenade_counts_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_grenade_index_decode`.
     *
     * @address 0x4eb330
     */
    static int32_t grenade_index_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_grenade_index_encode`.
     *
     * @address 0x4eb2d0
     */
    static int32_t grenade_index_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * 0x0065d51f
     *
     * @address 0x4e9af0
     */
    static int32_t index_compute_size(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_index_decode`.
     *
     * @address 0x4e9bf0
     */
    static int32_t index_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_index_encode`.
     *
     * @address 0x4e9bc0
     */
    static int32_t index_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * 0x4f0530
     *
     * @address 0x4e9b10
     */
    static uint8_t index_initialize(message_delta_field_type *field_type);

    /**
     * 0x4f04c0, EDI table
     *
     * @address 0x4e9b90
     */
    static void index_teardown(message_delta_field_type *field_type);

    /**
     * 0x0069a2e8
     *
     * @address 0x4eba40
     */
    static int32_t item_placement_compute_size(message_delta_field_type *field_type);

    /**
     * 0x0069a2e8
     *
     * @address 0x4ebc20
     */
    static int32_t item_placement_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_item_placement_encode`.
     *
     * @address 0x4ebab0
     */
    static int32_t item_placement_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * 0x0071cfa8
     *
     * @address 0x4eba60
     */
    static uint8_t item_placement_initialize(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_pointer_compute_size`.
     *
     * @address 0x4e99d0
     */
    static int32_t pointer_compute_size(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_pointer_decode`.
     *
     * @address 0x4e9a60
     */
    static int32_t pointer_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_pointer_encode`.
     *
     * @address 0x4e9a30
     */
    static int32_t pointer_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_pointer_initialize`.
     *
     * @address 0x4e9a00
     */
    static uint8_t pointer_initialize(message_delta_field_type *field_type);

    /**
     * 0x0065d51f
     *
     * @address 0x4e9ac0
     */
    static int32_t range_compute_size(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_range_initialize`.
     *
     * @address 0x4e9ae0
     */
    static uint8_t range_initialize(message_delta_field_type *field_type);

    /**
     * Original engine function `message_delta_weapon_index_decode`.
     *
     * @address 0x4eb280
     */
    static int32_t weapon_index_decode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

    /**
     * Original engine function `message_delta_weapon_index_encode`.
     *
     * @address 0x4eb220
     */
    static int32_t weapon_index_encode(message_delta_field_type *field_type, void *previous, void *current, bit_stream *stream);

};

}  // namespace halo::networking
