#pragma once

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <stdint.h>
#include <stdarg.h>

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Lookup of bitmap offsets inside a bitmap group sequence.
 */
class BitmapSequenceLookup {
public:
    static int32_t get_bitmap_offset(datum_index bitmap_tag, int16_t sequence_index, int16_t frame_index);
};

}
