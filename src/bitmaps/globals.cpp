/**
 * @file src/bitmaps/globals.cpp
 * Binds halo::bitmaps::Globals to the engine variables the data image defines under their original link names.
 */

#include "halo/bitmaps/bitmaps.hpp"
#include "halo/cache/api.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/bitmaps/globals.hpp"
#include "link/bitmaps.hpp"

namespace halo::bitmaps {

Globals &Service::instance()
{
    static Globals state{
        ::bitmap_format_bits_per_pixel,
        ::bitmap_group_debug_dump,
    };
    return state;
}

}  // namespace halo::bitmaps
