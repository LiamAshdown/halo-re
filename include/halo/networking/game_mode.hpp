#pragma once

#include <cstdint>

namespace halo::networking {

/** Values of the 16-bit network_game_mode global: how this machine takes part in the game. */
inline constexpr int16_t k_game_mode_local = 0;
inline constexpr int16_t k_game_mode_client = 1;
inline constexpr int16_t k_game_mode_host = 2;
inline constexpr int16_t k_game_mode_replay = 3;

}  // namespace halo::networking
