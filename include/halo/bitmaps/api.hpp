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

const char * targa_export(BitmapData *bitmap, file_reference_record *destination);
uint32_t bitmap_data_depth_valid_for_type(int32_t depth, BitmapDataType_t type);
void bitmap_data_block_delete_element(TagReflexive *block, int32_t index);
BitmapData * bitmap_group_get_bitmap_data(datum_index bitmap_tag_index, int16_t bitmap_data_index);
BitmapData * bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag_index, int16_t frame_index, int16_t sequence_index);
uint8_t bitmap_group_postprocess(datum_index tag_id, uint8_t skip_hardware_textures);
void color_argb_int_to_real(ColorARGB *out, uint32_t packed);
void color_rgb_int_to_real(ColorRGB *out, uint32_t packed);
ColorRGB * color_interpolate(ColorRGB *color1, ColorRGB *color0, ColorRGB *dest, color_interpolation_flags flags, float t);

}
