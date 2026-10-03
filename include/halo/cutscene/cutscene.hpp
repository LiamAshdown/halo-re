#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "game.h"
#include "ai.h"
#include "cutscene.h"
#include "rasterizer.h"
#include "render.h"
#include "networking.h"
#include "cache.h"
#include "interface.h"

namespace halo::cutscene {
namespace {

/**
 * Cutscene mode switching: entering and leaving cutscene state, queueing title cards and
 * drawing the letterbox bars.
 */
class CutsceneDirector {
public:
    static void start();
    static void stop();
    static void title_queue(int16_t title_index, float delay_seconds);
    static void letterbox();
};

}
}
