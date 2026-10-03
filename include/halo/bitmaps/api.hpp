/**
 * @file include/halo/bitmaps/api.hpp
 * Functions of the bitmaps module that other modules and the data tables call (namespace halo::bitmaps). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdarg.h>
#include <stdint.h>

struct BitmapData;
struct ColorARGB;
struct ColorARGBInt;
struct ColorRGB;
struct TagReflexive;
struct dxt3_block;
struct dxt5_block;
struct dxt_color_block;
struct file_reference_record;
struct real_hsv_color;
enum color_interpolation_flags : int;
typedef int16_t BitmapDataType_t;
typedef uint32_t datum_index;

namespace halo::bitmaps {

int32_t bitmap_data_calculate_mip_depth(BitmapData *bitmap, int32_t level);
uint32_t bitmap_data_calculate_mip_dimension(BitmapData *bitmap, int32_t level);
uint32_t bitmap_data_calculate_mip_level_pixel_count(BitmapData *bitmap, int32_t level);
uint32_t bitmap_data_calculate_mip_level_byte_size(BitmapData *bitmap, int32_t level);
uint32_t bitmap_data_calculate_mip_row_byte_size(BitmapData *bitmap, int32_t level);
uint32_t bitmap_data_calculate_pixel_data_size(BitmapData *bitmap);
void * bitmap_data_get_row_address(BitmapData *bitmap_data, int16_t mip_level, int16_t x, int16_t y);
void * bitmap_data_get_cube_map_pixel_address(BitmapData *bitmap, int32_t mip_level, int16_t x, int16_t y, int16_t face);
void * bitmap_data_get_pixel_address(BitmapData *bitmap, int32_t mip_level);
void bitmap_data_free(BitmapData *bitmap_data);
char * targa_export(BitmapData *bitmap, file_reference_record *destination);
uint32_t bitmap_data_depth_valid_for_type(int32_t depth, BitmapDataType_t type);
void bitmap_data_block_delete_element(TagReflexive *block, int32_t index);
BitmapData * bitmap_group_get_bitmap_data(datum_index bitmap_tag_index, int16_t bitmap_data_index);
BitmapData * bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag_index, int16_t frame_index, int16_t sequence_index);
uint8_t bitmap_group_postprocess(datum_index tag_id, uint8_t skip_hardware_textures);
void color_argb_int_to_real(ColorARGB *out, uint32_t packed);
void color_rgb_int_to_real(ColorRGB *out, uint32_t packed);
ColorRGB * color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, color_interpolation_flags flags, float t);
ColorRGB * color_interpolate_argb_with_tint(color_interpolation_flags flags, ColorARGB *color1, ColorRGB *dest, ColorRGB *tint, ColorARGB *color0, float t);
void dxt1_decode_block_texel(ColorARGBInt *out, dxt_color_block *block, int32_t x, int32_t y);
void dxt3_decode_alpha_texel(int32_t x, int32_t y, ColorARGBInt *texel_out, dxt3_block *block);
void dxt5_decode_alpha_texel(dxt5_block *block, ColorARGBInt *texel_out, int32_t x, int32_t y);

}
