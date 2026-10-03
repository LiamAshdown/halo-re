/**
 * @file include/halo/interface/game_state_block.hpp
 * Reservation of the interface module's runtime state blocks from the game-state bump allocator.
 */
#pragma once

#include <stdint.h>

#include "halo/memory/api.hpp"
#include "halo/saved_games/api.hpp"

namespace halo::interface {

/**
 * Reserves sizeof(T) bytes at the game-state cursor, advances the cursor and folds the block size (not its contents) into the
 * running game-state checksum, exactly as the original allocation sequence does.
 */
template <typename T>
inline T *game_state_allocate_block() {
    auto &saved = halo::saved_games::globals();
    int32_t size = static_cast<int32_t>(sizeof(T));
    T *block = reinterpret_cast<T *>(saved.game_state_base + saved.game_state_cursor);

    saved.game_state_cursor = saved.game_state_cursor + size;
    halo::memory::crc32_update(&saved.game_state_crc, reinterpret_cast<uint8_t *>(&size), 4);
    return block;
}

}  // namespace halo::interface
