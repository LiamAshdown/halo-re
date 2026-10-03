/**
 * @file include/halo/bitmaps/globals.hpp
 * The bitmaps module's engine globals as one service object. The variables live at fixed addresses in the data
 * image (standalone/data) under their original link names; Globals holds a reference to each, so no other file
 * declares them.
 */
#pragma once

#include <stdint.h>

namespace halo::bitmaps {

struct Globals {
    int8_t (&bitmap_format_bits_per_pixel)[k_bitmap_data_format_count];
    uint8_t &bitmap_group_debug_dump;
};

/**
 * The bitmaps service singleton. instance() builds the Globals reference table on first use (Meyers singleton); the state it
 * refers to lives in the data image. globals() is the short form every caller uses.
 */
class Service {
public:
    static Globals &instance();
};

inline Globals &globals() { return Service::instance(); }

}  // namespace halo::bitmaps
