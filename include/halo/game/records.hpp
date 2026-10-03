#pragma once

#include <cstdint>

#include "halo/core/datum.hpp"
#include "halo/game/api.hpp"

namespace halo::game {

/**
 * The player record in the slot named by a player datum handle.
 *
 * Stands for the `players` data array arithmetic (data + slot * sizeof(player)) the engine code open-coded everywhere.
 */
inline player *player_at(uint32_t handle) noexcept {
    return reinterpret_cast<player *>(static_cast<uint8_t *>(globals().player_data->data) + (handle & k_datum_slot_mask) * sizeof(player));
}

static_assert(sizeof(player) == 0x200);

}  // namespace halo::game
