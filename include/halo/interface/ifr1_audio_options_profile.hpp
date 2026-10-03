#pragma once

#include "tags.h"
#include "memory.h"
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
 * Applies the audio settings stored in a player profile to the sound system.
 */
class AudioOptionsProfile {
public:
    static uint32_t apply_from_profile(widget_instance *widget);
};

}
