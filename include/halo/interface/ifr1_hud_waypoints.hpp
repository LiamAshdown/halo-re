#pragma once

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "objects.h"
#include "networking.h"
#include "interface.h"
#include <stdint.h>
#include <stdarg.h>

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Behaviour of the original HudWaypoints functions.
 */
class HudWaypoints {
public:
    static void activate_for_player(datum_index player_index, datum_index target, int16_t kind, int16_t arrow_index, float vertical_offset);
    static void activate_for_team(datum_index target, int16_t arrow_index, int16_t team, int16_t kind, float vertical_offset);
    static int16_t arrow_find(const char *name);
    static void deactivate_for_player(datum_index player_index, datum_index target, int16_t kind);
    static void deactivate_for_team(int16_t kind, int16_t team, datum_index target);
    static void draw(const real_point3d *position, int16_t local_player_index, int16_t arrow_index, int16_t visibility, uint8_t show_distance);
    static void draw_all_for_player(void);
    static void draw_one(datum_index player_index);
};

}
