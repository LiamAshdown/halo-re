/**
 * @file standalone/data/link/shaders.hpp
 * Link names of the engine variables the shaders module binds in halo::shaders::Globals (src/shaders/globals.cpp). The variables are
 * defined in standalone/data under these C names; this header is included by that one file only.
 */
#pragma once

extern "C" {
extern int32_t numeric_countdown_timer_remaining_ms;
extern game_time_globals *game_time;
extern uint8_t numeric_countdown_timer_running;
extern int32_t numeric_countdown_timer_last_update_ms;
}
