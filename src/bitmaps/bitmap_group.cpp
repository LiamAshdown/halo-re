/**
 * @file src/bitmaps/bitmap_group.cpp
 * Bitmap tag (group) level operations: element lookup and load-time postprocessing.
 * The original author notes and decompiles are in docs/original/bitmaps/.
 */

#include "halo/bitmaps/bitmaps.hpp"

extern "C" {
extern tag_instance *tag_instances;
extern int8_t bitmap_format_bits_per_pixel[k_bitmap_data_format_count];
extern uint8_t bitmap_group_debug_dump;
extern uint8_t rasterizer_bitmap_create_hardware_texture(BitmapData *bitmap);
extern void rasterizer_bitmap_upload_2d_mipmaps(BitmapData *bitmap);
extern void rasterizer_bitmap_upload_cubemap_mipmaps(BitmapData *bitmap);
extern void rasterizer_bitmap_upload_cubemap_mipmaps_by_face(BitmapData *bitmap);
}

namespace halo::bitmaps {

BitmapData * bitmap_group::get_bitmap_data(datum_index bitmap_tag_index, int16_t bitmap_data_index)
{
    Bitmap *bitmap = (Bitmap *)tag_instances[(uint16_t)bitmap_tag_index].data;

    if (bitmap != 0 && bitmap_data_index >= 0) {
        if (bitmap_data_index < (int32_t)bitmap->bitmap_data.count) {
            return (BitmapData *)bitmap->bitmap_data.pointer + bitmap_data_index;
        }
    }
    return 0;
}

BitmapData * bitmap_group::sequence_get_bitmap_data(datum_index bitmap_tag_index, int16_t frame_index, int16_t sequence_index)
{
    Bitmap *bitmap;
    int16_t bitmap_index;

    if (bitmap_tag_index == (datum_index)k_datum_index_none) {
        return 0;
    }
    bitmap = (Bitmap *)tag_instances[(uint16_t)bitmap_tag_index].data;
    if (bitmap == 0) {
        return 0;
    }

    bitmap_index = frame_index;
    if (bitmap->bitmap_group_sequence.count > 0) {
        BitmapGroupSequence *sequence = (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer +
            (int32_t)sequence_index % (int32_t)bitmap->bitmap_group_sequence.count;
        int16_t resolved;

        if ((int16_t)sequence->bitmap_count > 0) {
            resolved = (int16_t)((int32_t)frame_index % (int32_t)(int16_t)sequence->bitmap_count +
                (int16_t)sequence->first_bitmap_index);
        } else if (sequence->sprites.count != 0) {
            resolved = ((BitmapGroupSprite *)sequence->sprites.pointer + frame_index)->bitmap_index;
        } else {
            resolved = frame_index;
        }

        if (resolved != (int16_t)k_datum_index_none) {
            bitmap_index = resolved;
        }
    }

    if (bitmap_index >= 0 && bitmap_index < (int32_t)bitmap->bitmap_data.count) {
        return (BitmapData *)bitmap->bitmap_data.pointer + bitmap_index;
    }
    return 0;
}

uint8_t bitmap_group::postprocess(datum_index tag_id, uint8_t skip_hardware_textures)
{
    Bitmap *bitmap = (Bitmap *)tag_instances[(uint16_t)tag_id].data;
    BitmapData *bitmap_data_array = (BitmapData *)bitmap->bitmap_data.pointer;
    int32_t bitmap_data_count = (int32_t)bitmap->bitmap_data.count;
    BitmapGroupSequence *sequences = (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer;
    int32_t sequence_count = (int32_t)bitmap->bitmap_group_sequence.count;
    uint8_t success = 1;
    int32_t i;

    for (i = 0; i < bitmap_data_count; i++) {
        BitmapData *entry = bitmap_data_array + i;
        uint32_t total_pixel_count = 0;
        int32_t bit_total;
        uint8_t verified;
        int32_t pixel_data_offset;

        entry->bitmap_tag_id.index = (uint16_t)tag_id;
        entry->bitmap_tag_id.id = (uint16_t)(tag_id >> 16);

        if ((int16_t)entry->mipmap_count >= 0) {
            int16_t level;
            for (level = 0; level <= (int16_t)entry->mipmap_count; level++) {
                total_pixel_count += bitmap_data_view(entry).calculate_mip_level_pixel_count(level);
            }
        }

        bit_total = (int32_t)bitmap_format_bits_per_pixel[entry->format] * (int32_t)total_pixel_count;
        entry->pixel_data_size = (uint32_t)((bit_total + ((bit_total >> 31) & 7)) >> 3);

        entry->pointer = (uint32_t)k_datum_index_none;
        entry->pixel_base = 0;
        *(void **)&entry->hardware_texture = 0;

        if (bitmap->type == bitmaptype_interface_bitmaps) {
            entry->flags |= _bitmap_data_linear_bit;
        }

        verified = bitmap_data_view(entry).verify(0);
        pixel_data_offset = (int32_t)entry->pixel_data_offset;
        if (!verified || pixel_data_offset < 0 ||
            (int32_t)bitmap->processed_pixel_data.size <
                (int32_t)bitmap_data_view(entry).calculate_pixel_data_size() + pixel_data_offset) {
            success = 0;
        } else {
            entry->pixel_base = (uint8_t *)bitmap->processed_pixel_data.pointer + pixel_data_offset;
        }
    }

    if (!skip_hardware_textures && success && bitmap_data_count > 0) {
        for (i = 0; i < bitmap_data_count; i++) {
            BitmapData *entry = bitmap_data_array + i;
            if (bitmap->type != bitmaptype_interface_bitmaps) {
                if (*(void **)&entry->hardware_texture == 0) {
                    rasterizer_bitmap_create_hardware_texture(entry);
                }
                switch (entry->type) {
                case bitmapdatatype_2d_texture:
                    rasterizer_bitmap_upload_2d_mipmaps(entry);
                    break;
                case bitmapdatatype_3d_texture:
                    rasterizer_bitmap_upload_cubemap_mipmaps(entry);
                    break;
                case bitmapdatatype_cube_map:
                    rasterizer_bitmap_upload_cubemap_mipmaps_by_face(entry);
                    break;
                default:
                    break;
                }
            }
        }
    }

    for (i = 0; i < sequence_count; i++) {
        if (bitmap->type == bitmaptype_sprites &&
            (sequences[i].first_bitmap_index != 0 || sequences[i].bitmap_count != 0)) {
            sequences[i].first_bitmap_index = 0;
            sequences[i].bitmap_count = 0;
        }
    }

    if (sequence_count > 0) {
        BitmapGroupSequence *last = &sequences[sequence_count - 1];
        if (last->bitmap_count == 0 && last->sprites.count == 0) {
            success = 0;
        }
    }

    if (bitmap_group_debug_dump) {
        int32_t count_a;
        for (count_a = 0; count_a < bitmap_data_count; count_a++) {
        }
        for (i = 0; i < sequence_count; i++) {
            if (bitmap->type == bitmaptype_sprites) {
                int32_t sprite_count = (int32_t)sequences[i].sprites.count;
                int32_t j;
                for (j = 0; j < sprite_count; j++) {
                }
            }
        }
    }

    return success;
}

}  // namespace halo::bitmaps
