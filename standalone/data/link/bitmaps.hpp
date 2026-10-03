/**
 * @file standalone/data/link/bitmaps.hpp
 * Link names of the engine variables the bitmaps module binds in halo::bitmaps::Globals (src/bitmaps/globals.cpp). The variables are
 * defined in standalone/data under these C names; this header is included by that one file only.
 */
#pragma once

extern "C" {
extern int8_t bitmap_format_bits_per_pixel[k_bitmap_data_format_count];
extern uint8_t bitmap_group_debug_dump;
}
