/**
 * @file include/halo/bitmaps/bitmaps_c_api.h
 * The C ABI of the bitmaps module: every original function with its original signature and C linkage.
 * Defined in src/bitmaps/bitmaps_c_api.cpp; documented on the halo::bitmaps C++ API.
 */
#pragma once

#include "halo/bitmaps/bitmaps.hpp"

#ifdef __cplusplus
extern "C" {
#endif

int32_t bitmap_data_calculate_mip_depth(BitmapData *bitmap, int32_t level);
uint32_t bitmap_data_calculate_mip_dimension(BitmapData *bitmap, int32_t level);
uint32_t bitmap_data_calculate_mip_level_pixel_count(BitmapData *bitmap, int32_t level);
uint32_t bitmap_data_calculate_mip_level_byte_size(BitmapData *bitmap, int32_t level);
uint32_t bitmap_data_calculate_mip_row_byte_size(BitmapData *bitmap, int32_t level);
uint32_t bitmap_data_calculate_pixel_data_size(BitmapData *bitmap);
void * bitmap_data_get_row_address(BitmapData *bitmap_data, int16_t mip_level, int16_t x, int16_t y);
void * bitmap_data_get_volume_pixel_address(BitmapData *bitmap_data, int16_t x, int16_t y, int16_t z, int16_t mip_level);
void * bitmap_data_get_cube_map_pixel_address(BitmapData *bitmap, int32_t mip_level, int16_t x, int16_t y, int16_t face);
void * bitmap_data_get_pixel_address(BitmapData *bitmap, int32_t mip_level);
uint8_t bitmap_data_verify(BitmapData *bitmap, uint8_t require_runtime);
void bitmap_data_free(BitmapData *bitmap_data);
char * targa_export(BitmapData *bitmap, file_reference_record *destination);
uint32_t bitmap_data_depth_valid_for_type(int32_t depth, BitmapDataType_t type);
void bitmap_data_block_delete_element(TagReflexive *block, int32_t index);
BitmapData * bitmap_group_get_bitmap_data(datum_index bitmap_tag_index, int16_t bitmap_data_index);
BitmapData * bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag_index, int16_t frame_index, int16_t sequence_index);
uint8_t bitmap_group_postprocess(datum_index tag_id, uint8_t skip_hardware_textures);
void color_argb_int_to_real(ColorARGB *out, uint32_t packed);
void color_rgb_int_to_real(ColorRGB *out, uint32_t packed);
void color_565_unpack_to_rgb888(uint16_t *packed, ColorARGBInt *out);
real_hsv_color * color_rgb_to_hsv(ColorRGB *color, real_hsv_color *hsv);
ColorRGB * color_hsv_to_rgb(real_hsv_color *hsv, ColorRGB *color);
ColorRGB * color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, color_interpolation_flags flags, float t);
ColorRGB * color_interpolate_argb_with_tint(color_interpolation_flags flags, ColorARGB *color1, ColorRGB *dest, ColorRGB *tint, ColorARGB *color0, float t);
void dxt1_decode_block_texel(ColorARGBInt *out, dxt_color_block *block, int32_t x, int32_t y);
void dxt3_decode_alpha_texel(int32_t x, int32_t y, ColorARGBInt *texel_out, dxt3_block *block);
void dxt5_decode_alpha_texel(dxt5_block *block, ColorARGBInt *texel_out, int32_t x, int32_t y);

#ifdef __cplusplus
}
#endif
