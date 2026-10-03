/**
 * @file src/bitmaps/bitmap_data.cpp
 * BitmapData records: mip arithmetic, pixel addressing, validation, teardown and Targa export.
 * The original author notes and decompiles are in docs/original/bitmaps/.
 */

#include "halo/bitmaps/bitmaps.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"

extern "C" {
extern int8_t bitmap_format_bits_per_pixel[k_bitmap_data_format_count];
extern uint8_t file_reference_create(file_reference_record *ref);
extern uint8_t file_reference_open(file_reference_record *ref, uint8_t mode);
extern uint8_t file_reference_write(file_reference_record *ref, const void *buffer, uint32_t size);
extern uint8_t file_reference_close(file_reference_record *ref);
}

namespace halo::bitmaps {

int32_t bitmap_data_view::calculate_mip_depth(int32_t level)
{
    int32_t depth;

    depth = (int16_t)self->depth >> (level & 0x1f);
    if (depth < 2) {
        depth = 1;
    }
    return depth;
}

uint32_t bitmap_data_view::calculate_mip_dimension(int32_t level)
{
    int32_t height;

    height = (int16_t)self->height >> (level & 0x1f);
    if (height < 2) {
        height = 1;
    }
    if ((self->flags & _bitmap_data_compressed_bit) != 0) {
        height = (height + 3) & ~3;
    }
    return (uint32_t)height;
}

uint32_t bitmap_data_view::calculate_mip_level_pixel_count(int32_t level)
{
    int32_t width;
    int32_t height;
    int32_t depth;
    int32_t pixel_count;
    uint32_t compressed;

    compressed = self->flags & _bitmap_data_compressed_bit;

    width = (int16_t)self->width >> (level & 0x1f);
    if (width < 2) {
        width = 1;
    }
    if (compressed != 0) {
        width = (width + 3) & ~3;
    }

    height = (int16_t)self->height >> (level & 0x1f);
    if (height < 2) {
        height = 1;
    }
    if (compressed != 0) {
        height = (height + 3) & ~3;
    }

    depth = (int16_t)self->depth >> (level & 0x1f);
    if (depth < 2) {
        depth = 1;
    }

    pixel_count = height * width * depth;
    if (self->type == bitmapdatatype_cube_map) {
        pixel_count = pixel_count * k_cube_map_face_count;
    }
    return (uint32_t)pixel_count;
}

uint32_t bitmap_data_view::calculate_mip_level_byte_size(int32_t level)
{
    int32_t pixel_count;
    int32_t bits;

    pixel_count = (int32_t)bitmap_data_view(self).calculate_mip_level_pixel_count(level);
    bits = pixel_count * (int32_t)bitmap_format_bits_per_pixel[self->format];
    return (uint32_t)(bits / 8);
}

uint32_t bitmap_data_view::calculate_mip_row_byte_size(int32_t level)
{
    int32_t width;
    int32_t bits;

    width = (int16_t)self->width >> (level & 0x1f);
    if (width < 2) {
        width = 1;
    }
    if ((self->flags & _bitmap_data_compressed_bit) != 0) {
        width = (width + 3) & ~3;
    }

    bits = (int32_t)bitmap_format_bits_per_pixel[self->format] * width;
    return (uint32_t)(bits / 8);
}

uint32_t bitmap_data_view::calculate_pixel_data_size()
{
    int32_t total_pixels;
    int16_t level;
    int32_t bits;

    total_pixels = 0;
    if (0 <= (int16_t)self->mipmap_count) {
        level = 0;
        do {
            total_pixels += (int32_t)bitmap_data_view(self).calculate_mip_level_pixel_count(level);
            level++;
        } while (level <= (int16_t)self->mipmap_count);
    }

    bits = total_pixels * (int32_t)bitmap_format_bits_per_pixel[self->format];
    return (uint32_t)(bits / 8);
}

void * bitmap_data_view::get_row_address(int16_t mip_level, int16_t x, int16_t y)
{
    int16_t width = (int16_t)self->width;
    int16_t height = (int16_t)self->height;
    int16_t min_dimension = (self->flags & _bitmap_data_compressed_bit) ?
        k_bitmap_compressed_block_dimension : 1;
    int32_t pixel_offset = 0;
    int32_t bit_offset;
    int16_t level;
    uint32_t base;

    for (level = mip_level; level > 0; level--) {
        pixel_offset += (int32_t)width * (int32_t)height;
        width  = (width  >> 1 >= min_dimension) ? (int16_t)(width  >> 1) : min_dimension;
        height = (height >> 1 >= min_dimension) ? (int16_t)(height >> 1) : min_dimension;
    }

    pixel_offset += (int32_t)x + (int32_t)width * (int32_t)y;
    bit_offset = pixel_offset * (int32_t)bitmap_format_bits_per_pixel[self->format];

    base = (uint32_t)self->pixel_base;
    return (void *)(base + (uint32_t)((bit_offset + ((bit_offset >> 31) & 7)) >> 3));
}

void * bitmap_data_view::get_volume_pixel_address(int16_t x, int16_t y, int16_t z, int16_t mip_level)
{
    int16_t width = (int16_t)self->width;
    int16_t height = (int16_t)self->height;
    int16_t depth = (int16_t)self->depth;
    int16_t min_dimension = (self->flags & _bitmap_data_compressed_bit) ?
        k_bitmap_compressed_block_dimension : 1;
    int32_t voxel_offset = 0;
    int32_t bit_offset;
    int16_t level;
    uint32_t base;

    for (level = mip_level; level > 0; level--) {
        voxel_offset += (int32_t)width * (int32_t)height * (int32_t)depth;
        width  = (width  >> 1 >= min_dimension) ? (int16_t)(width  >> 1) : min_dimension;
        height = (height >> 1 >= min_dimension) ? (int16_t)(height >> 1) : min_dimension;
        depth  = (depth < 2) ? 1 : (int16_t)(depth >> 1);
    }

    voxel_offset += (int32_t)x + ((int32_t)height * (int32_t)z + (int32_t)y) * (int32_t)width;
    bit_offset = voxel_offset * (int32_t)bitmap_format_bits_per_pixel[self->format];

    base = (uint32_t)self->pixel_base;
    return (void *)(base + (uint32_t)((bit_offset + ((bit_offset >> 31) & 7)) >> 3));
}

void * bitmap_data_view::get_cube_map_pixel_address(int32_t mip_level, int16_t x, int16_t y, int16_t face)
{
    int16_t width;
    int16_t min_dimension;
    int16_t levels_remaining;
    int32_t earlier_levels_pixel_count;
    int32_t pixel_index;
    int32_t bit_offset;

    width = (int16_t)self->width;
    min_dimension = ((self->flags & _bitmap_data_compressed_bit) != 0) ?
        k_bitmap_compressed_block_dimension : 1;

    earlier_levels_pixel_count = 0;
    levels_remaining = (int16_t)mip_level;
    while (levels_remaining > 0) {
        earlier_levels_pixel_count += (int32_t)width * width * k_cube_map_face_count;
        width = ((width >> 1) < min_dimension) ? min_dimension : (int16_t)(width >> 1);
        levels_remaining--;
    }

    pixel_index = (int32_t)x + ((int32_t)face * (int32_t)width + (int32_t)y) * (int32_t)width +
        earlier_levels_pixel_count;
    bit_offset = pixel_index * (int32_t)bitmap_format_bits_per_pixel[self->format];

    return *(uint8_t **)&((struct BitmapData *)self)->pixel_base + bit_offset / 8;
}

void * bitmap_data_view::get_pixel_address(int32_t mip_level)
{
    switch (self->type) {
    case bitmapdatatype_2d_texture:
        return bitmap_data_view(self).get_row_address((int16_t)mip_level, 0, 0);
    case bitmapdatatype_3d_texture:
        return bitmap_data_view(self).get_volume_pixel_address(0, 0, 0, (int16_t)mip_level);
    case bitmapdatatype_cube_map:
        return bitmap_data_view(self).get_cube_map_pixel_address(mip_level, 0, 0, 0);
    default:
        return self;
    }
}

uint8_t bitmap_data_view::verify(uint8_t require_runtime)
{
    int32_t max_dimension;
    int32_t max_levels;

    if (self->bitmap_class != k_bitmap_data_signature) {
        return 0;
    }
    if ((self->flags & ~(uint32_t)k_bitmap_data_valid_flags_mask) != 0) {
        return 0;
    }
    if (self->type < 0 || self->type >= k_bitmap_data_type_count) {
        return 0;
    }
    if (self->format < 0 || self->format >= k_bitmap_data_format_count) {
        return 0;
    }
    if (self->width <= 0 || self->width > k_bitmap_maximum_dimension) {
        return 0;
    }
    if (self->height <= 0 || self->height > k_bitmap_maximum_dimension) {
        return 0;
    }
    if (!bitmap_data_depth_valid_for_type(self->depth, self->type)) {
        return 0;
    }
    if ((int16_t)self->mipmap_count < 0) {
        return 0;
    }

    max_dimension = (self->height > self->depth) ? self->height : self->depth;
    if (self->width > max_dimension) {
        max_dimension = self->width;
    }
    max_levels = halo::math::uint32_log2_floor((uint32_t)max_dimension);
    if ((int16_t)self->mipmap_count > (int16_t)max_levels) {
        return 0;
    }

    if (require_runtime == 0) {
        return 1;
    }

    if (self->format == k_bitmap_runtime_format &&
        *(void **)&((struct BitmapData *)self)->pixel_base != 0 &&
        self->mipmap_count == 0 &&
        (self->flags & (_bitmap_data_compressed_bit | _bitmap_data_palettized_bit | _bitmap_data_swizzled_bit)) == 0) {
        return 1;
    }
    return 0;
}

void bitmap_data_view::free()
{
    if (self == 0) {
        return;
    }

    if (self->flags & _bitmap_data_texture_cache_bit) {
        if (self->pointer != (uint32_t)k_datum_index_none) {
            halo::memory::cache_evict_entry((datum_index)self->pointer, halo::cache::globals().texture_cache);
        }
        self->pointer = (uint32_t)k_datum_index_none;
        self->pixel_base = 0;
    }

    if (*(void **)&self->hardware_texture != 0) {
        void *hardware_texture = *(void **)&self->hardware_texture;
        void **vtable = *(void ***)hardware_texture;
        ((bitmap_hardware_texture_release_proc)vtable[2])(hardware_texture);
        *(void **)&self->hardware_texture = 0;
    }

    if (self->flags & _bitmap_data_runtime_allocated_bit) {
        if (self->pixel_base != 0) {
            GlobalFree(self->pixel_base);
        }
        GlobalFree(self);
    }
}

char * bitmap_data_view::targa_export(file_reference_record *destination)
{
    targa_header header;
    char *error;
    int32_t row;
    int32_t row_byte_size;
    void *row_pixels;

    if (file_reference_create(destination) != 0 &&
        file_reference_open(destination, _file_open_write) != 0) {
        header.id_length = 0;
        header.color_map_type = 0;
        header.image_type = k_targa_image_type_true_color;
        header.color_map_first_entry = 0;
        header.color_map_length = 0;
        header.color_map_entry_size = 0;
        header.x_origin = 0;
        header.y_origin = 0;
        header.width = (int16_t)self->width;
        header.height = (int16_t)self->height;
        header.bits_per_pixel = k_targa_bits_per_pixel;
        header.image_descriptor = k_targa_image_descriptor_top_left_8_alpha;

        error = 0;
        if (file_reference_write(destination, &header, sizeof(header)) == 0) {
            error = (char *)"couldn't write header";
        } else if (0 < (int16_t)self->height) {
            row_byte_size = (int32_t)(int16_t)self->width * 4;
            row = 0;
            do {
                row_pixels = bitmap_data_view(self).get_row_address(0, 0, (int16_t)row);
                if (file_reference_write(destination, row_pixels, row_byte_size) == 0) {
                    file_reference_close(destination);
                    return (char *)"couldn't write row";
                }
                row = row + 1;
            } while (row < (int16_t)self->height);
            file_reference_close(destination);
            return 0;
        }
        file_reference_close(destination);
        return error;
    }
    return (char *)"couldn't open file";
}

uint32_t bitmap_data_depth_valid_for_type(int32_t depth, BitmapDataType_t type)
{
    if (depth > 0 && depth <= k_bitmap_maximum_depth &&
        (depth == 1 || type == bitmapdatatype_3d_texture)) {
        return 1;
    }
    return 0;
}

void bitmap_data_block_delete_element(TagReflexive *block, int32_t index)
{
    bitmap_data_view((BitmapData *)((uint8_t *)block->pointer + index * sizeof(BitmapData))).free();
}

}  // namespace halo::bitmaps
