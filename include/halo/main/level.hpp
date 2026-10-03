#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::main {

/**
 * Map and scenario session control: queued map changes, level transitions, saving and credits.
 * Stateless service class: the functions are static members, the state they act on lives in the engine globals.
 */
struct LevelControl {
    static void campaign_level_advance(void);
    static int campaign_level_find_index_for_path(char *path);
    static void chimera__load_ui_map(char play_title_music);
    static void credits_load_directly_for_endgame(void);
    static void scenario_session_begin(network_scenario_load_request *request);
    static void start_new_single_player_map(void);
    static void level_transition_update(void);
    static void queue_cache_file_open(char *name);
    static void queue_map_change(const char *map_name);
    static uint8_t queue_map_change_by_name_or_clear(char *name);
    static void save_map_private(void);
    static void switch_structure_bsp_and_notify(void);
};

}
