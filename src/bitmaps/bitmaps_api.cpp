/**
 * @file src/bitmaps/bitmaps_api.cpp
 * The bitmaps module's free-function API (include/halo/bitmaps/api.hpp), forwarding to the class implementations.
 */

#include <stdarg.h>
#include "halo/bitmaps/bitmaps.hpp"
#include "halo/bitmaps/api.hpp"

namespace halo::bitmaps {

int32_t bitmap_data_calculate_mip_depth(BitmapData *bitmap, int32_t level)
{
    return halo::bitmaps::bitmap_data_view(bitmap).calculate_mip_depth(level);
}

uint32_t bitmap_data_calculate_mip_dimension(BitmapData *bitmap, int32_t level)
{
    return halo::bitmaps::bitmap_data_view(bitmap).calculate_mip_dimension(level);
}

uint32_t bitmap_data_calculate_mip_level_pixel_count(BitmapData *bitmap, int32_t level)
{
    return halo::bitmaps::bitmap_data_view(bitmap).calculate_mip_level_pixel_count(level);
}

uint32_t bitmap_data_calculate_mip_level_byte_size(BitmapData *bitmap, int32_t level)
{
    return halo::bitmaps::bitmap_data_view(bitmap).calculate_mip_level_byte_size(level);
}

uint32_t bitmap_data_calculate_mip_row_byte_size(BitmapData *bitmap, int32_t level)
{
    return halo::bitmaps::bitmap_data_view(bitmap).calculate_mip_row_byte_size(level);
}

uint32_t bitmap_data_calculate_pixel_data_size(BitmapData *bitmap)
{
    return halo::bitmaps::bitmap_data_view(bitmap).calculate_pixel_data_size();
}

void * bitmap_data_get_row_address(BitmapData *bitmap_data, int16_t mip_level, int16_t x, int16_t y)
{
    return halo::bitmaps::bitmap_data_view(bitmap_data).get_row_address(mip_level, x, y);
}

void * bitmap_data_get_cube_map_pixel_address(BitmapData *bitmap, int32_t mip_level, int16_t x, int16_t y, int16_t face)
{
    return halo::bitmaps::bitmap_data_view(bitmap).get_cube_map_pixel_address(mip_level, x, y, face);
}

void * bitmap_data_get_pixel_address(BitmapData *bitmap, int32_t mip_level)
{
    return halo::bitmaps::bitmap_data_view(bitmap).get_pixel_address(mip_level);
}

void bitmap_data_free(BitmapData *bitmap_data)
{
    halo::bitmaps::bitmap_data_view(bitmap_data).free();
}

char * targa_export(BitmapData *bitmap, file_reference_record *destination)
{
    return halo::bitmaps::bitmap_data_view(bitmap).targa_export(destination);
}

BitmapData * bitmap_group_get_bitmap_data(datum_index bitmap_tag_index, int16_t bitmap_data_index)
{
    return halo::bitmaps::bitmap_group::get_bitmap_data(bitmap_tag_index, bitmap_data_index);
}

BitmapData * bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag_index, int16_t frame_index, int16_t sequence_index)
{
    return halo::bitmaps::bitmap_group::sequence_get_bitmap_data(bitmap_tag_index, frame_index, sequence_index);
}

uint8_t bitmap_group_postprocess(datum_index tag_id, uint8_t skip_hardware_textures)
{
    return halo::bitmaps::bitmap_group::postprocess(tag_id, skip_hardware_textures);
}

void color_argb_int_to_real(ColorARGB *out, uint32_t packed)
{
    halo::bitmaps::color_codec::argb_int_to_real(out, packed);
}

void color_rgb_int_to_real(ColorRGB *out, uint32_t packed)
{
    halo::bitmaps::color_codec::rgb_int_to_real(out, packed);
}

ColorRGB * color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, color_interpolation_flags flags, float t)
{
    return halo::bitmaps::color_codec::interpolate(color1, color0, dest, flags, t);
}

ColorRGB * color_interpolate_argb_with_tint(color_interpolation_flags flags, ColorARGB *color1, ColorRGB *dest, ColorRGB *tint, ColorARGB *color0, float t)
{
    return halo::bitmaps::color_codec::interpolate_argb_with_tint(flags, color1, dest, tint, color0, t);
}

void dxt1_decode_block_texel(ColorARGBInt *out, dxt_color_block *block, int32_t x, int32_t y)
{
    halo::bitmaps::dxt_decoder::decode_dxt1_texel(out, block, x, y);
}

void dxt3_decode_alpha_texel(int32_t x, int32_t y, ColorARGBInt *texel_out, dxt3_block *block)
{
    halo::bitmaps::dxt_decoder::decode_dxt3_texel(x, y, texel_out, block);
}

void dxt5_decode_alpha_texel(dxt5_block *block, ColorARGBInt *texel_out, int32_t x, int32_t y)
{
    halo::bitmaps::dxt_decoder::decode_dxt5_texel(block, texel_out, x, y);
}

}
