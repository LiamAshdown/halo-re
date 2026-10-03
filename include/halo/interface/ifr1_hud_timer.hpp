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
 * Behaviour of the original HudTimer functions.
 */
class HudTimer {
public:
    static void pause_timer(uint8_t paused);
    static void set_timer_time(int32_t minutes, int32_t seconds);
    static void draw(void);
    static uint32_t get_ticks(void);
};

}
