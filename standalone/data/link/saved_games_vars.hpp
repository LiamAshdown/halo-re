/**
 * @file standalone/data/link/saved_games_vars.hpp
 * Link names of the engine variables owned by the saved_games module (halo::saved_games::vars()). The data image defines them under these C
 * names; only src/saved_games/vars.cpp includes this header, so the names are declared as untyped storage.
 */
#pragma once

extern "C" {
extern char cache_file_current_header_crc32[];
extern char checkpoint_sort_newest_first[];
extern char control_gamepad_action_scan_buttons[];
extern char control_gamepad_axis_scan_table[];
extern char control_gamepad_button_scan_table[];
extern char control_gamepad_pov_scan_table[];
extern char control_mouse_axis_scan_table[];
extern char default_game_variant_count[];
extern char default_game_variant_procs[];
extern char default_player_profile_initialized[];
extern char default_player_profiles_directory[];
extern char default_playlists_directory[];
extern char file_enumeration_find_data[];
extern char file_enumeration_flags_value[];
extern char file_enumeration_handles[];
extern char file_enumeration_path[];
extern char file_enumeration_pos[];
extern char file_root_template[];
extern char game_state_after_load_procs[];
extern char game_state_base[];
extern char game_state_core_directory[];
extern char game_state_crc[];
extern char game_state_cursor[];
extern char game_state_header_ptr[];
extern char game_state_header_valid[];
extern char game_state_persistent_storage_path[];
extern char game_state_revert_proc[];
extern char game_state_revert_time[];
extern char game_state_size[];
extern char game_state_snapshot_source[];
extern char game_state_write_completed[];
extern char game_state_write_event[];
extern char game_state_write_in_progress[];
extern char game_state_write_is_checkpoint[];
extern char input_device_to_slot[];
extern char last_game_variant_path[];
extern char last_multiplayer_map_path[];
extern char last_profile_path[];
extern char player_color_table[];
extern char player_profile_thread[];
extern char player_profiles_directory[];
extern char playlists_directory[];
extern char profile_directory[];
extern char profile_load_complete[];
extern char saved_directory[];
extern char saved_game_display_name_buffer[];
extern char saved_game_files_initialized[];
extern char saved_game_files_mutex[];
extern char saved_game_index_file_open[];
extern char saved_game_root_directory[];
extern char saved_player_profile_slots_handle[];
extern char savegame_index_dirty[];
extern char savegame_index_write_count[];
extern char savegames_directory[];
extern char unknown_0072132a[];
extern char variant_write_request_state[];
extern char variant_write_thread[];
}
