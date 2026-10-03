#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "objects.h"
#include "scenario.h"
#include "cutscene.h"
#include "crt.h"
#include <stdint.h>
#include "halo/cutscene/layout.hpp"

namespace halo::cutscene {

/**
 * Playback of scenario recorded animations on one unit addressed by its object handle, plus
 * the static lookups by name and the per-tick update of every playing animation.
 */
class RecordedAnimationPlayer {
public:
    explicit RecordedAnimationPlayer(datum_index value) : unit_handle(value) {}
    uint8_t start(int16_t scenario_animation_index, uint16_t extra_flags);
    uint8_t is_playing();
    recorded_animation * find_by_object(datum_index *out_index);
    static int16_t find_by_name(const char *name, Scenario *scenario);
    static void update_all();

    datum_index unit_handle;
};

}
