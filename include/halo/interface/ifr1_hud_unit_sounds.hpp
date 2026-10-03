#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "objects.h"
#include "units.h"
#include "cutscene.h"
#include "sound.h"
#include <stdint.h>
#include <stdarg.h>

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Behaviour of the original HudUnitSounds functions.
 */
class HudUnitSounds {
public:
    static void play(uint32_t active_mask, const TagReflexive *sounds, int32_t *handles, uint16_t *playing);
    static void update(player *p, uint8_t hud_enabled);
};

}
