#pragma once

#include "halo/game/game2_types.hpp"

namespace halo::game {

/**
 * Top-level game lifecycle: initialize, map start/stop/unload, per-tick simulation and save/pause gates.
 */
class GameLifecycle {
public:
    static ScenarioPlayerStartingLocation * get_player_starting_location(int16_t index);
    static void initialize(void);
    static uint32_t no_player_is_dead(void);
    static uint32_t safe_to_pause(void);
    static uint8_t safe_to_save(void);
    static void set_local_player(datum_index player_handle, int16_t local_player_index);
    static void simulate_tick(uint32_t predict_pass);
    static void start_new_map(void);
    static void stop_current_map(void);
    static void time_format_minutes_seconds(uint32_t ticks, uint32_t count, wchar_t *dest);
    static void time_format_minutes_seconds_ascii(uint32_t ticks, uint32_t count, char *dest);
    static void unload_map(void);
};

}
