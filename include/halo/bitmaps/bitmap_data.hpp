/**
 * @file include/halo/bitmaps/bitmap_data.hpp
 * BitmapData records: mip arithmetic, pixel addressing, validation, teardown and Targa export.
 */
#pragma once

#include "halo/bitmaps/bitmaps_types.hpp"

namespace halo::bitmaps {

/**
 * Non-owning view of one BitmapData record (stride 0x30, array element of a Bitmap tag). Every operation reads or
 * writes the record in place; the view owns nothing.
 */
class bitmap_data_view {
public:
    explicit bitmap_data_view(BitmapData *p) : self(p) {}

    /**
     * Returns the depth of the given mip level: the base depth shifted right by the level, never below 1.
     *
     * @address 0x43fbe0
     */
    int32_t calculate_mip_depth(int32_t level);

    /**
     * Returns the height of the given mip level (the base height shifted right by the level, minimum 1, rounded up to a
     * multiple of 4 for compressed formats).
     *
     * @address 0x43fbb0
     */
    uint32_t calculate_mip_dimension(int32_t level);

    /**
     * Returns the pixel count of one mip level: width * height * depth with compressed dimensions rounded up to a multiple
     * of 4, times six for cube maps.
     *
     * @address 0x43fc10
     */
    uint32_t calculate_mip_level_pixel_count(int32_t level);

    /**
     * Computes the byte size of the bitmap's pixel data at one mip level from its pixel count and bits per pixel.
     *
     * @address 0x43fcb0
     */
    uint32_t calculate_mip_level_byte_size(int32_t level);

    /**
     * Computes the byte pitch of one row of pixels at the given mip level.
     *
     * @address 0x43fce0
     */
    uint32_t calculate_mip_row_byte_size(int32_t level);

    /**
     * Computes the total byte size of the bitmap's pixel data across every mip level, from level 0 through mipmap_count
     * inclusive.
     *
     * @address 0x43fb70
     */
    uint32_t calculate_pixel_data_size();

    /**
     * Computes the byte address of pixel (x, y) at the given mip level of a 2D bitmap by walking the mip chain from the
     * pixel base address.
     *
     * @address 0x43f8e0
     */
    void * get_row_address(int16_t mip_level, int16_t x, int16_t y);

    /**
     * Computes the byte address of pixel (x, y, z) at the given mip level of a 3D volume bitmap.
     *
     * @address 0x43f990
     */
    void * get_volume_pixel_address(int16_t x, int16_t y, int16_t z, int16_t mip_level);

    /**
     * Computes the byte address of pixel (x, y) on one cube map face at the given mip level, accounting for the six faces
     * stored per level.
     *
     * @address 0x43fa90
     */
    void * get_cube_map_pixel_address(int32_t mip_level, int16_t x, int16_t y, int16_t face);

    /**
     * Returns the base address of the pixels at a mip level, dispatching on the bitmap type to the 2D, volume or cube map
     * addressing at (0, 0, 0). Any other type returns the pixel base pointer.
     *
     * @address 0x43fb20
     */
    void * get_pixel_address(int32_t mip_level);

    /**
     * Validates the tag-side fields: signature, flags, type, format, dimensions, depth and mipmap count. With
     * require_runtime set it also requires a resident 32-bit-per-pixel bitmap with no mip chain and no compressed,
     * palettized or swizzled flag.
     *
     * @address 0x43fd30
     */
    uint8_t verify(uint8_t require_runtime);

    /**
     * Releases what the record owns: evicts its texture cache entry, releases the hardware texture, and for
     * runtime-allocated bitmaps frees the pixel buffer and the record itself. A NULL record is ignored.
     *
     * @address 0x43f880
     */
    void free();

    /**
     * Writes mip level 0 as an uncompressed 32-bit-per-pixel Targa file at the destination. Returns NULL on success or a
     * static error string on failure.
     *
     * @address 0x43fe60
     */
    char * targa_export(file_reference_record *destination);

private:
    BitmapData *self;
};

/**
 * Returns nonzero when the depth is in 1..256 and is 1 unless the bitmap type is a 3D texture.
 *
 * @address 0x43fe30
 */
uint32_t bitmap_data_depth_valid_for_type(int32_t depth, BitmapDataType_t type);

/**
 * Frees the bitmap_data element at the given index of a tag reflexive of BitmapData (stride 0x30).
 *
 * @address 0x43f010
 */
void bitmap_data_block_delete_element(TagReflexive *block, int32_t index);

}  // namespace halo::bitmaps
