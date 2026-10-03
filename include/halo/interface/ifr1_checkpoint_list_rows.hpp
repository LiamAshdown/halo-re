#pragma once

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <stdint.h>
#include <stdarg.h>

#ifdef interface
#undef interface
#endif

namespace halo::interface {

/**
 * Behaviour of the original CheckpointListRows functions.
 */
class CheckpointListRows {
public:
    static uint8_t add_row(int32_t index, const char *name, int32_t level_index, int32_t difficulty, int32_t game_time, const void *time, void *user_data);
};

}
