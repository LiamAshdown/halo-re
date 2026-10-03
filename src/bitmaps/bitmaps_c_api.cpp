/**
 * @file src/bitmaps/bitmaps_c_api.cpp
 * The bitmaps module's C ABI: one extern "C" function per original symbol, same name and signature,
 * forwarding to the halo::bitmaps implementation. The shims do nothing else.
 */

#include "halo/bitmaps/bitmaps_c_api.h"

extern "C" int32_t bitmap_data_calculate_mip_depth(BitmapData *bitmap, int32_t level)
{
    return halo::bitmaps::bitmap_data_view(bitmap).calculate_mip_depth(level);
}

extern "C" uint32_t bitmap_data_calculate_mip_dimension(BitmapData *bitmap, int32_t level)
{
    return halo::bitmaps::bitmap_data_view(bitmap).calculate_mip_dimension(level);
}

extern "C" uint32_t bitmap_data_calculate_mip_level_pixel_count(BitmapData *bitmap, int32_t level)
{
    return halo::bitmaps::bitmap_data_view(bitmap).calculate_mip_level_pixel_count(level);
}

extern "C" uint32_t bitmap_data_calculate_mip_level_byte_size(BitmapData *bitmap, int32_t level)
{
    return halo::bitmaps::bitmap_data_view(bitmap).calculate_mip_level_byte_size(level);
}

extern "C" uint32_t bitmap_data_calculate_mip_row_byte_size(BitmapData *bitmap, int32_t level)
{
    return halo::bitmaps::bitmap_data_view(bitmap).calculate_mip_row_byte_size(level);
}

extern "C" uint32_t bitmap_data_calculate_pixel_data_size(BitmapData *bitmap)
{
    return halo::bitmaps::bitmap_data_view(bitmap).calculate_pixel_data_size();
}

extern "C" void * bitmap_data_get_row_address(BitmapData *bitmap_data, int16_t mip_level, int16_t x, int16_t y)
{
    return halo::bitmaps::bitmap_data_view(bitmap_data).get_row_address(mip_level, x, y);
}

extern "C" void * bitmap_data_get_volume_pixel_address(BitmapData *bitmap_data, int16_t x, int16_t y, int16_t z, int16_t mip_level)
{
    return halo::bitmaps::bitmap_data_view(bitmap_data).get_volume_pixel_address(x, y, z, mip_level);
}

extern "C" void * bitmap_data_get_cube_map_pixel_address(BitmapData *bitmap, int32_t mip_level, int16_t x, int16_t y, int16_t face)
{
    return halo::bitmaps::bitmap_data_view(bitmap).get_cube_map_pixel_address(mip_level, x, y, face);
}

extern "C" void * bitmap_data_get_pixel_address(BitmapData *bitmap, int32_t mip_level)
{
    return halo::bitmaps::bitmap_data_view(bitmap).get_pixel_address(mip_level);
}

extern "C" uint8_t bitmap_data_verify(BitmapData *bitmap, uint8_t require_runtime)
{
    return halo::bitmaps::bitmap_data_view(bitmap).verify(require_runtime);
}

extern "C" void bitmap_data_free(BitmapData *bitmap_data)
{
    halo::bitmaps::bitmap_data_view(bitmap_data).free();
}

extern "C" char * targa_export(BitmapData *bitmap, file_reference_record *destination)
{
    return halo::bitmaps::bitmap_data_view(bitmap).targa_export(destination);
}

extern "C" uint32_t bitmap_data_depth_valid_for_type(int32_t depth, BitmapDataType_t type)
{
    return halo::bitmaps::bitmap_data_depth_valid_for_type(depth, type);
}

extern "C" void bitmap_data_block_delete_element(TagReflexive *block, int32_t index)
{
    halo::bitmaps::bitmap_data_block_delete_element(block, index);
}

extern "C" BitmapData * bitmap_group_get_bitmap_data(datum_index bitmap_tag_index, int16_t bitmap_data_index)
{
    return halo::bitmaps::bitmap_group::get_bitmap_data(bitmap_tag_index, bitmap_data_index);
}

extern "C" BitmapData * bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag_index, int16_t frame_index, int16_t sequence_index)
{
    return halo::bitmaps::bitmap_group::sequence_get_bitmap_data(bitmap_tag_index, frame_index, sequence_index);
}

extern "C" uint8_t bitmap_group_postprocess(datum_index tag_id, uint8_t skip_hardware_textures)
{
    return halo::bitmaps::bitmap_group::postprocess(tag_id, skip_hardware_textures);
}

extern "C" void color_argb_int_to_real(ColorARGB *out, uint32_t packed)
{
    halo::bitmaps::color_codec::argb_int_to_real(out, packed);
}

extern "C" void color_rgb_int_to_real(ColorRGB *out, uint32_t packed)
{
    halo::bitmaps::color_codec::rgb_int_to_real(out, packed);
}

extern "C" void color_565_unpack_to_rgb888(uint16_t *packed, ColorARGBInt *out)
{
    halo::bitmaps::color_codec::unpack_565_to_rgb888(packed, out);
}

extern "C" real_hsv_color * color_rgb_to_hsv(ColorRGB *color, real_hsv_color *hsv)
{
    return halo::bitmaps::color_codec::rgb_to_hsv(color, hsv);
}

extern "C" ColorRGB * color_hsv_to_rgb(real_hsv_color *hsv, ColorRGB *color)
{
    return halo::bitmaps::color_codec::hsv_to_rgb(hsv, color);
}

extern "C" ColorRGB * color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, color_interpolation_flags flags, float t)
{
    return halo::bitmaps::color_codec::interpolate(color1, color0, dest, flags, t);
}

extern "C" ColorRGB * color_interpolate_argb_with_tint(color_interpolation_flags flags, ColorARGB *color1, ColorRGB *dest, ColorRGB *tint, ColorARGB *color0, float t)
{
    return halo::bitmaps::color_codec::interpolate_argb_with_tint(flags, color1, dest, tint, color0, t);
}

extern "C" void dxt1_decode_block_texel(ColorARGBInt *out, dxt_color_block *block, int32_t x, int32_t y)
{
    halo::bitmaps::dxt_decoder::decode_dxt1_texel(out, block, x, y);
}

extern "C" void dxt3_decode_alpha_texel(int32_t x, int32_t y, ColorARGBInt *texel_out, dxt3_block *block)
{
    halo::bitmaps::dxt_decoder::decode_dxt3_texel(x, y, texel_out, block);
}

extern "C" void dxt5_decode_alpha_texel(dxt5_block *block, ColorARGBInt *texel_out, int32_t x, int32_t y)
{
    halo::bitmaps::dxt_decoder::decode_dxt5_texel(block, texel_out, x, y);
}
