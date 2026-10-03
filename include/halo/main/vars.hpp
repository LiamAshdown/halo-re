/**
 * @file include/halo/main/vars.hpp
 * Addresses of the engine variables the main module owns, by name. Other files bind a typed reference to the entry they use
 * (halo::link::ref<T>); the link names themselves appear only in standalone/data/link/main_vars.hpp.
 */
#pragma once

namespace halo::main {

/** Address table of the engine variables owned by the main module. */
struct Vars {
    void *campaign_level_short_names;
    void *checkfpu;
    void *connect_thread;
    void *console_active;
    void *console_debug_flag_0;
    void *console_debug_flag_5;
    void *console_debug_word_8;
    void *console_default_color;
    void *console_globals_data;
    void *console_message_head;
    void *console_message_tail;
    void *console_rcon_handle;
    void *console_win32_attached;
    void *directsound_quality;
    void *error_file_logging_enabled;
    void *frame_rate_average_data;
    void *game_screen_rect;
    void *game_state_before_save_proc;
    void *game_state_revert_available;
    void *game_window_top_left;
    void *graphics_driver_version;
    void *hostname_resolve_complete;
    void *hostname_resolve_result;
    void *input_globals;
    void *interface_loading_screen_address_a;
    void *interface_loading_screen_address_b;
    void *main_globals_byte_0071973a;
    void *main_globals_byte_0071974f;
    void *main_globals_data;
    void *main_menu_music_pending;
    void *main_render_skip_threshold_ms;
    void *main_unknown_696570;
    void *map_path_prefix;
    void *movie_playback_abort;
    void *multiplayer_maps;
    void *network_bandwidth_graph_default_interval_ms;
    void *network_bandwidth_graph_globals;
    void *network_buffer_pair_pool;
    void *novideo_or_connect;
    void *player_update_log_flags;
    void *pregame_render_view;
    void *progress_screen_subtext;
    void *progress_screen_text;
    void *rasterizer_device_lost;
    void *render_view_local_player_sticky;
    void *render_views;
    void *screenshot_scale;
    void *screenshots;
    void *shell_application_inactive;
    void *shell_startup_tick_count;
    void *terminal_initialized;
    void *terminal_messages;
    void *timedemo_globals_data;
    void *timedemo_last_frame_index;
    void *timedemo_pixel_shader_version;
    void *ui_pause_pending_count_00718fa0;
    void *unknown_00710301;
    void *unknown_0071973b;
    void *unknown_00719769;
    void *unknown_0071976a;
    void *unknown_00873d30;
};

/** The singleton address table; its storage is constant-initialised. */
const Vars &vars();

}  // namespace halo::main
