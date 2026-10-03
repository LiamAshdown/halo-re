#pragma once

#include <stdint.h>
#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "projectiles.h"
#include "effects.h"
#include "networking.h"
#include "sound.h"
#include "physics.h"
#include "structures.h"
#include "rasterizer.h"
#include "render.h"
#include "camera.h"
#include "interface.h"
#include "main.h"

typedef int32_t (*effect_marker_resolver)(uint32_t object_index, const char *location,
    object_marker *out, uint32_t max_count);

