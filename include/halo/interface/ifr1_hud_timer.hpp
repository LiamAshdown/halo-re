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
 * The script countdown timer shown on the HUD. An instance binds the engine's HUD messaging state and game clock, so
 * every operation works on the same fields the original free functions read.
 */
class HudTimer {
public:
    HudTimer();

    void pause(uint8_t paused);
    void set_time(int32_t minutes, int32_t seconds);
    void draw(void);
    uint32_t ticks(void) const;

private:
    hud_messaging_globals *messaging;
    game_time_globals *time;
};

}
