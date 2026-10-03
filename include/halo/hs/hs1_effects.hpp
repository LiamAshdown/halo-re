#pragma once

#include <stddef.h>
#include <stdint.h>

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"
#include "hs.h"

namespace halo::hs {

/**
 * Script-driven effect and damage spawning helpers used by the hs evaluators.
 */
class ScriptEffects {
public:
    static void damage_apply_at_location(int16_t location_index, uint32_t damage_effect);
    static void damage_apply_with_sound(datum_index object_index, uint32_t damage_effect);
    static void effect_spawn_at_location(int16_t location_index, uint32_t effect);
    static void effect_spawn_on_marker(datum_index object_index, datum_index effect, char *marker_name);
};

}
