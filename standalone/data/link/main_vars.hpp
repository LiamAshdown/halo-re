/**
 * @file standalone/data/link/main_vars.hpp
 * Link names of the engine variables owned by the main module (halo::main::vars()). The data image defines them under these C
 * names; only src/main/vars.cpp includes this header, so the names are declared as untyped storage.
 */
#pragma once

extern "C" {
extern char campaign_level_short_names[];
extern char checkfpu[];
extern char connect_thread[];
extern char console_active[];
extern char console_debug_flag_0[];
extern char console_debug_flag_5[];
extern char console_debug_word_8[];
extern char console_default_color[];
extern char console_globals_data[];
extern char console_message_head[];
extern char console_message_tail[];
extern char console_rcon_handle[];
extern char console_win32_attached[];
extern char directsound_quality[];
extern char error_file_logging_enabled[];
extern char frame_rate_average_data[];
extern char game_screen_rect[];
extern char game_state_before_save_proc[];
extern char game_state_revert_available[];
extern char game_window_top_left[];
extern char graphics_driver_version[];
extern char hostname_resolve_complete[];
extern char hostname_resolve_result[];
extern char input_globals[];
extern char interface_loading_screen_address_a[];
extern char interface_loading_screen_address_b[];
extern char main_globals_byte_0071973a[];
extern char main_globals_byte_0071974f[];
extern char main_globals_data[];
extern char main_menu_music_pending[];
extern char main_render_skip_threshold_ms[];
extern char main_unknown_696570[];
extern char map_path_prefix[];
extern char movie_playback_abort[];
extern char multiplayer_maps[];
extern char network_bandwidth_graph_default_interval_ms[];
extern char network_bandwidth_graph_globals[];
extern char network_buffer_pair_pool[];
extern char novideo_or_connect[];
extern char player_update_log_flags[];
extern char pregame_render_view[];
extern char progress_screen_subtext[];
extern char progress_screen_text[];
extern char rasterizer_device_lost[];
extern char render_view_local_player_sticky[];
extern char render_views[];
extern char screenshot_scale[];
extern char screenshots[];
extern char shell_application_inactive[];
extern char shell_startup_tick_count[];
extern char terminal_initialized[];
extern char terminal_messages[];
extern char timedemo_globals_data[];
extern char timedemo_last_frame_index[];
extern char timedemo_pixel_shader_version[];
extern char ui_pause_pending_count_00718fa0[];
extern char unknown_00710301[];
extern char unknown_0071973b[];
extern char unknown_00719769[];
extern char unknown_0071976a[];
extern char unknown_00873d30[];
}
