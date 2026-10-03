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
 * Behaviour of the original AudioOptionsProfile functions.
 */
class AudioOptionsProfile {
public:
    static uint32_t apply_from_profile(widget_instance *widget);
};

}
