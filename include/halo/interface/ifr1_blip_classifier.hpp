#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "interface.h"
#include <stdint.h>
#include <stdarg.h>

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Behaviour of the original BlipClassifier functions.
 */
class BlipClassifier {
public:
    static uint8_t type_get(int16_t local_player_index, datum_index object_index);
};

}
