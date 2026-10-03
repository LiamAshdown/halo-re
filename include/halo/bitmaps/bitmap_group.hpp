/**
 * @file include/halo/bitmaps/bitmap_group.hpp
 * Bitmap tag (group) level operations: element lookup and load-time postprocessing.
 */
#pragma once

#include "halo/bitmaps/bitmaps_types.hpp"

namespace halo::bitmaps {

/**
 * Operations on a whole Bitmap tag addressed by its tag index. All state is the engine's tag instance table.
 */
struct bitmap_group {
    /**
     * Returns the bitmap_data element with the given index of a bitmap tag, or NULL when the tag has no data or the index
     * is out of range.
     *
     * @address 0x43f250
     */
    static BitmapData * get_bitmap_data(datum_index bitmap_tag_index, int16_t bitmap_data_index);

    /**
     * Resolves a (sequence, frame) pair of a bitmap tag to its bitmap_data element, through the sequence's bitmap range or
     * sprite list. Returns NULL for an invalid tag or an out-of-range result.
     *
     * @address 0x43f290
     */
    static BitmapData * sequence_get_bitmap_data(datum_index bitmap_tag_index, int16_t frame_index, int16_t sequence_index);

    /**
     * Post-processes a freshly loaded bitmap tag: validates every bitmap_data, recomputes its pixel data size, resets the
     * runtime texture words and, unless skip_hardware_textures is set, creates the hardware textures. Returns nonzero on
     * success.
     *
     * @address 0x43f030
     */
    static uint8_t postprocess(datum_index tag_id, uint8_t skip_hardware_textures);

};

}  // namespace halo::bitmaps
