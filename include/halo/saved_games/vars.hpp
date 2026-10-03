/**
 * @file include/halo/saved_games/vars.hpp
 * Addresses of the engine variables the saved_games module owns, by name. Other files bind a typed reference to the entry they use
 * (halo::link::ref<T>); the link names themselves appear only in standalone/data/link/saved_games_vars.hpp.
 */
#pragma once

namespace halo::saved_games {

/** Address table of the engine variables owned by the saved_games module. */
struct Vars {
    void *cache_file_current_header_crc32;
    void *checkpoint_sort_newest_first;
    void *control_gamepad_action_scan_buttons;
    void *control_gamepad_axis_scan_table;
    void *control_gamepad_button_scan_table;
    void *control_gamepad_pov_scan_table;
    void *control_mouse_axis_scan_table;
    void *default_game_variant_count;
    void *default_game_variant_procs;
    void *default_player_profile_initialized;
    void *default_player_profiles_directory;
    void *default_playlists_directory;
    void *file_enumeration_find_data;
    void *file_enumeration_flags_value;
    void *file_enumeration_handles;
    void *file_enumeration_path;
    void *file_enumeration_pos;
    void *file_root_template;
    void *game_state_after_load_procs;
    void *game_state_base;
    void *game_state_core_directory;
    void *game_state_crc;
    void *game_state_cursor;
    void *game_state_header_ptr;
    void *game_state_header_valid;
    void *game_state_persistent_storage_path;
    void *game_state_revert_proc;
    void *game_state_revert_time;
    void *game_state_size;
    void *game_state_snapshot_source;
    void *game_state_write_completed;
    void *game_state_write_event;
    void *game_state_write_in_progress;
    void *game_state_write_is_checkpoint;
    void *input_device_to_slot;
    void *last_game_variant_path;
    void *last_multiplayer_map_path;
    void *last_profile_path;
    void *player_color_table;
    void *player_profile_thread;
    void *player_profiles_directory;
    void *playlists_directory;
    void *profile_directory;
    void *profile_load_complete;
    void *saved_directory;
    void *saved_game_display_name_buffer;
    void *saved_game_files_initialized;
    void *saved_game_files_mutex;
    void *saved_game_index_file_open;
    void *saved_game_root_directory;
    void *saved_player_profile_slots_handle;
    void *savegame_index_dirty;
    void *savegame_index_write_count;
    void *savegames_directory;
    void *unknown_0072132a;
    void *variant_write_request_state;
    void *variant_write_thread;
};

/** The singleton address table; its storage is constant-initialised. */
const Vars &vars();

}  // namespace halo::saved_games
