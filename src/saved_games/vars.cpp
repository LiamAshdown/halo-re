/**
 * @file src/saved_games/vars.cpp
 * Binds halo::saved_games::Vars to the engine variables the data image defines under their original link names.
 */

#include "halo/saved_games/vars.hpp"
#include "link/saved_games_vars.hpp"
#include "halo/saved_games/api.hpp"

namespace halo::saved_games {

const Vars &vars()
{
    static const Vars table{
        cache_file_current_header_crc32,
        checkpoint_sort_newest_first,
        control_gamepad_action_scan_buttons,
        control_gamepad_axis_scan_table,
        control_gamepad_button_scan_table,
        control_gamepad_pov_scan_table,
        control_mouse_axis_scan_table,
        default_game_variant_count,
        default_game_variant_procs,
        default_player_profile_initialized,
        default_player_profiles_directory,
        default_playlists_directory,
        file_enumeration_find_data,
        file_enumeration_flags_value,
        file_enumeration_handles,
        file_enumeration_path,
        file_enumeration_pos,
        file_root_template,
        game_state_after_load_procs,
        game_state_base,
        game_state_core_directory,
        game_state_crc,
        game_state_cursor,
        game_state_header_ptr,
        game_state_header_valid,
        game_state_persistent_storage_path,
        game_state_revert_proc,
        game_state_revert_time,
        game_state_size,
        game_state_snapshot_source,
        game_state_write_completed,
        game_state_write_event,
        game_state_write_in_progress,
        game_state_write_is_checkpoint,
        input_device_to_slot,
        last_game_variant_path,
        last_multiplayer_map_path,
        last_profile_path,
        player_color_table,
        player_profile_thread,
        player_profiles_directory,
        playlists_directory,
        profile_directory,
        profile_load_complete,
        saved_directory,
        saved_game_display_name_buffer,
        saved_game_files_initialized,
        saved_game_files_mutex,
        saved_game_index_file_open,
        saved_game_root_directory,
        saved_player_profile_slots_handle,
        savegame_index_dirty,
        savegame_index_write_count,
        savegames_directory,
        unknown_0072132a,
        variant_write_request_state,
        variant_write_thread,
    };
    return table;
}

}  // namespace halo::saved_games
