#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#include "units.h"
#include "cutscene.h"
#include <stdint.h>
#include <stdarg.h>

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Behaviour of the original ErrorDialogs functions.
 */
class ErrorDialogs {
public:
    static void show(int16_t error_string_index, int32_t player_index, uint8_t modal, uint8_t is_error);
};

}
