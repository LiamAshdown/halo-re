/**
 * @file include/halo/sound/sound_types.hpp
 * Common includes for the sound module's C++ headers: the engine type headers and the record types of
 * types/sound.h. The global `sound` record keeps its name inside namespace halo::sound.
 */
#pragma once

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "camera.h"
#include "scenario.h"
#include "projectiles.h"
#include "sound.h"
#include "interface.h"
#include "main.h"

namespace halo::sound {
using ::sound;
}
