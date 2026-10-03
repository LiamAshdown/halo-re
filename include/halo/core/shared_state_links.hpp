#pragma once

#include <stdint.h>
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/saved_games/vars.hpp"
#include "halo/shell/vars.hpp"

struct Globals;
struct ScenarioStructureBSP;
struct player_globals;

/**
 * Game-state link references shared by the rasterizer and sound state headers, defined once instead of behind include guards.
 */
inline auto &game_state_base = halo::link::ref<uint8_t *>(halo::saved_games::vars().game_state_base);
inline auto &game_state_crc = halo::link::ref<uint32_t>(halo::saved_games::vars().game_state_crc);
inline auto &game_state_cursor = halo::link::ref<int32_t>(halo::saved_games::vars().game_state_cursor);
inline auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);
inline auto &global_structure_bsp = halo::link::ref<ScenarioStructureBSP *>(halo::ai::vars().global_structure_bsp);
inline auto &local_player_globals = halo::link::ref<player_globals *>(halo::game::vars().local_player_globals);
inline auto &shell_window = halo::link::ref<void *>(halo::shell::vars().shell_window);
