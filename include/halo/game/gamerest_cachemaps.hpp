#pragma once

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

namespace halo::game {

/**
 * Map switching by cache-file path.
 */
class CacheFileMaps {
public:
    CacheFileMaps() = delete;

    static void switch_map_by_path(char *path, uint8_t apply_state);
};

}  // namespace halo::game
