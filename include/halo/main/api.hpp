/**
 * @file include/halo/main/api.hpp
 * Functions of the main module that other modules and the data tables call (namespace halo::main). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdint.h>

struct Rectangle2D;
struct network_scenario_load_request;
struct observer;
struct observer_camera;
struct render_view;

namespace halo::main {

void campaign_level_advance();
int campaign_level_find_index_for_path(char *path);
void chimera__load_ui_map(char play_title_music);
void credits_load_directly_for_endgame();
void game_scenario_session_begin(network_scenario_load_request *request);
void game_start_new_single_player_map();
void main_level_transition_update();
void main_queue_cache_file_open(char *name);
void main_queue_map_change(char *map_name);
uint8_t main_queue_map_change_by_name_or_clear(char *name);
void main_save_map_private();
void main_switch_structure_bsp_and_notify();
void chimera__exec_init();
void console_autocomplete_command();
uint32_t console_command_context_mask(uint32_t context_flags);
void console_deactivate();
uint8_t console_exec_file_run(const char *file_name);
void console_initialize();
uint32_t console_paste_clipboard_text();
char console_process_command(char *command_line, uint32_t context_flags);
uint8_t console_process_key_events();
void console_process_rcon_command(int32_t rcon_handle, char *command_line);
void console_toggle();
void game_engine_flush_pending_simulation_ticks();
uint32_t game_frame_rate_average_update();
void game_timer_reset();
void main_ensure_local_players();
void main_loop();
void main_loop_frame_pacer();
void main_loop_shutdown_cleanup();
void main_menu_music_stop();
void main_menu_return_and_reset();
void movie_capture_frame_export();
void movie_play_bink(const char *movie_path);
uint32_t __stdcall network_game_client_connect_by_hostname(char *host_port_string);
uint8_t network_game_client_connect_to_address_async(char *address, char *password);
void network_game_client_connect_to_resolved_address();
uint32_t network_hostname_resolve_thread_proc(char *hostname);
char network_hostname_resolve_with_timeout(char *hostname);
void render_frame_all_views(float time_since_tick, float time_since_frame);
int render_local_view_count();
void render_pregame_view_initialize();
void render_view_camera_fill(observer_camera *observer, render_view *view);
void screenshot_render(render_view *views);
void viewport_split_rect_compute(int32_t view_count, int32_t view_index, Rectangle2D *window, Rectangle2D *out_viewport);
void timedemo_benchmark_update();

}
