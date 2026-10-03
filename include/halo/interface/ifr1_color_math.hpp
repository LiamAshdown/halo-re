#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
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
 * Behaviour of the original ColorMath functions.
 */
class ColorMath {
public:
    static uint32_t argb_scale_alpha(uint32_t packed_color, float scale);
    static uint32_t pack_argb_from_real(ColorARGB *color);
    static uint32_t rgb_float_to_int(const float *rgb);
    static void cyclic_color(int16_t table_index, int16_t color_index, ColorARGB *out);
};

}
