/**
 * @file src/main/vars.cpp
 * Binds halo::main::Vars to the engine variables the data image defines under their original link names.
 */

#include "halo/main/vars.hpp"
#include "link/main_vars.hpp"
#include "halo/main/api.hpp"

namespace halo::main {

const Vars &vars()
{
    static const Vars table{
        campaign_level_short_names,
        checkfpu,
        connect_thread,
        console_active,
        console_debug_flag_0,
        console_debug_flag_5,
        console_debug_word_8,
        console_default_color,
        console_globals_data,
        console_message_head,
        console_message_tail,
        console_rcon_handle,
        console_win32_attached,
        directsound_quality,
        error_file_logging_enabled,
        frame_rate_average_data,
        game_screen_rect,
        game_state_before_save_proc,
        game_state_revert_available,
        game_window_top_left,
        graphics_driver_version,
        hostname_resolve_complete,
        hostname_resolve_result,
        input_globals,
        interface_loading_screen_address_a,
        interface_loading_screen_address_b,
        main_globals_byte_0071973a,
        main_globals_byte_0071974f,
        main_globals_data,
        main_menu_music_pending,
        main_render_skip_threshold_ms,
        main_unknown_696570,
        map_path_prefix,
        movie_playback_abort,
        multiplayer_maps,
        network_bandwidth_graph_default_interval_ms,
        network_bandwidth_graph_globals,
        network_buffer_pair_pool,
        novideo_or_connect,
        player_update_log_flags,
        pregame_render_view,
        progress_screen_subtext,
        progress_screen_text,
        rasterizer_device_lost,
        render_view_local_player_sticky,
        render_views,
        screenshot_scale,
        screenshots,
        shell_application_inactive,
        shell_startup_tick_count,
        terminal_initialized,
        terminal_messages,
        timedemo_globals_data,
        timedemo_last_frame_index,
        timedemo_pixel_shader_version,
        ui_pause_pending_count_00718fa0,
        unknown_00710301,
        unknown_0071973b,
        unknown_00719769,
        unknown_0071976a,
        unknown_00873d30,
    };
    return table;
}

}  // namespace halo::main
