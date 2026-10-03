#pragma once

#include <cstdint>

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "scenario.h"
#include "structures.h"
#include "objects.h"
#include "game.h"
#include "units.h"

#include "halo/core/datum.hpp"
#include "halo/cache/api.hpp"
#include "halo/game/api.hpp"
#include "halo/objects/api.hpp"

namespace halo::game {

/**
 * The player record in the slot named by a player datum handle.
 *
 * Stands for the `players` data array arithmetic (data + slot * sizeof(player)) the engine code open-coded everywhere.
 */
inline player *player_at(uint32_t handle) noexcept {
    return reinterpret_cast<player *>(static_cast<uint8_t *>(globals().player_data->data) + (handle & k_datum_slot_mask) * sizeof(player));
}

/** The live object in the slot named by an object datum handle (the object header's data pointer). */
inline object *object_at(uint32_t handle) noexcept {
    return reinterpret_cast<object_header *>(objects::globals().object_data->data)[handle & k_datum_slot_mask].data;
}

/** The raw bytes of the object in the slot named by an object datum handle. */
inline uint8_t *object_bytes(uint32_t handle) noexcept { return reinterpret_cast<uint8_t *>(object_at(handle)); }

/** The object header of the slot named by an object datum handle. */
inline object_header &object_header_at(uint32_t handle) noexcept {
    return reinterpret_cast<object_header *>(objects::globals().object_data->data)[handle & k_datum_slot_mask];
}

/** The loaded tag data of a tag datum handle. */
inline uint8_t *tag_data_at(uint32_t tag) noexcept {
    return static_cast<uint8_t *>(cache::globals().tag_instances[tag & k_datum_slot_mask].data);
}

static_assert(sizeof(player) == 0x200);

}  // namespace halo::game
