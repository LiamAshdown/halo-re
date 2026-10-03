/**
 * @file src/bitmaps/bitmaps_api.cpp
 * The bitmaps module's free-function API (include/halo/bitmaps/api.hpp), forwarding to the class implementations.
 */

#include <stdarg.h>
#include "halo/bitmaps/bitmaps.hpp"
#include "halo/bitmaps/api.hpp"

namespace halo::bitmaps {

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

}
