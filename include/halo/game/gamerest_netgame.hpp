#pragma once

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "items.h"
#include "game.h"
#include "networking.h"

namespace halo::game {

/**
 * Capture-the-flag engine behaviour for flag objects.
 */
class CtfEngine {
public:
    CtfEngine() = delete;

    static void flag_tick(uint32_t flag_handle, object *flag_obj);
    static void clear_carrier(datum_index flag_object_index, real_point3d *position);
};

/**
 * Netgame equipment filtering and server-side object type change broadcast.
 */
class NetgameRules {
public:
    NetgameRules() = delete;

    static uint8_t equipment_game_type_matches(int16_t *types, int32_t count, int32_t current_engine_index);
    static void broadcast_object_type_changes();
};

}  // namespace halo::game
