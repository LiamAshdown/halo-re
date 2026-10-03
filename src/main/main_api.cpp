/**
 * C linkage shims for the main module: one extern "C" function per original symbol, forwarding to the
 * C++ implementation in namespace halo::main or to the member function of the record it operates on.
 */

#include "tags.h"
#include "memory.h"
#include "interface.h"
#include "main.h"
#include <string.h>
#include <ctype.h>
#include "math.h"
#include "game.h"
#include "networking.h"
#include "saved_games.h"
#include "camera.h"
#include <stdint.h> 
#include "input.h"
#include "crt.h"
#include <stdio.h>
#include <stdarg.h>
#include "hs.h"
#include "cache.h"
#include "units.h"
#include "cutscene.h"
#include "win32.h"
#include "rasterizer.h"
#include "render.h"
#include "objects.h"
#include <wchar.h>
#include "bink.h"
#include "shell.h"
#include <stdlib.h>

#include "halo/main/level.hpp"
#include "halo/main/console.hpp"
#include "halo/main/main_loop.hpp"
#include "halo/main/movie.hpp"
#include "halo/main/network.hpp"
#include "halo/main/views.hpp"
#include "halo/main/timedemo.hpp"
#include "halo/main/api.hpp"

namespace halo::main {

void campaign_level_advance()
{
    halo::main::LevelControl::campaign_level_advance();
}

int campaign_level_find_index_for_path(char *path)
{
    return halo::main::LevelControl::campaign_level_find_index_for_path(path);
}

void chimera__load_ui_map(char play_title_music)
{
    halo::main::LevelControl::chimera__load_ui_map(play_title_music);
}

void credits_load_directly_for_endgame()
{
    halo::main::LevelControl::credits_load_directly_for_endgame();
}

void game_scenario_session_begin(network_scenario_load_request *request)
{
    halo::main::LevelControl::scenario_session_begin(request);
}

void game_start_new_single_player_map()
{
    halo::main::LevelControl::start_new_single_player_map();
}

void main_level_transition_update()
{
    halo::main::LevelControl::level_transition_update();
}

void main_queue_cache_file_open(char *name)
{
    halo::main::LevelControl::queue_cache_file_open(name);
}

void main_queue_map_change(char *map_name)
{
    halo::main::LevelControl::queue_map_change(map_name);
}

uint8_t main_queue_map_change_by_name_or_clear(char *name)
{
    return halo::main::LevelControl::queue_map_change_by_name_or_clear(name);
}

void main_save_map_private()
{
    halo::main::LevelControl::save_map_private();
}

void main_switch_structure_bsp_and_notify()
{
    halo::main::LevelControl::switch_structure_bsp_and_notify();
}

void chimera__exec_init()
{
    halo::main::Console::chimera__exec_init();
}

void console_autocomplete_command()
{
    halo::main::Console::autocomplete_command();
}

uint32_t console_command_context_mask(uint32_t context_flags)
{
    return halo::main::Console::command_context_mask(context_flags);
}

void console_deactivate()
{
    halo::main::Console::deactivate();
}

uint8_t console_exec_file_run(const char *file_name)
{
    return halo::main::Console::exec_file_run(file_name);
}

void console_initialize()
{
    halo::main::Console::initialize();
}

uint32_t console_paste_clipboard_text()
{
    return halo::main::Console::paste_clipboard_text();
}

char console_process_command(char *command_line, uint32_t context_flags)
{
    return halo::main::Console::process_command(command_line, context_flags);
}

uint8_t console_process_key_events()
{
    return halo::main::Console::process_key_events();
}

void console_process_rcon_command(int32_t rcon_handle, char *command_line)
{
    halo::main::Console::process_rcon_command(rcon_handle, command_line);
}

void console_toggle()
{
    halo::main::Console::toggle();
}

void game_engine_flush_pending_simulation_ticks()
{
    halo::main::MainLoop::engine_flush_pending_simulation_ticks();
}

uint32_t game_frame_rate_average_update()
{
    return halo::main::MainLoop::frame_rate_average_update();
}

void game_timer_reset()
{
    halo::main::MainLoop::timer_reset();
}

void main_ensure_local_players()
{
    halo::main::MainLoop::ensure_local_players();
}

void main_loop()
{
    halo::main::MainLoop::loop();
}

void main_loop_frame_pacer()
{
    halo::main::MainLoop::loop_frame_pacer();
}

void main_loop_shutdown_cleanup()
{
    halo::main::MainLoop::loop_shutdown_cleanup();
}

void main_menu_music_stop()
{
    halo::main::MainLoop::menu_music_stop();
}

void main_menu_return_and_reset()
{
    halo::main::MainLoop::menu_return_and_reset();
}

void movie_capture_frame_export()
{
    halo::main::MoviePlayer::capture_frame_export();
}

void movie_play_bink(const char *movie_path)
{
    halo::main::MoviePlayer::play_bink(movie_path);
}

uint32_t __stdcall network_game_client_connect_by_hostname(char *host_port_string)
{
    return halo::main::ClientConnection::game_client_connect_by_hostname(host_port_string);
}

uint8_t network_game_client_connect_to_address_async(char *address, char *password)
{
    return halo::main::ClientConnection::game_client_connect_to_address_async(address, password);
}

void network_game_client_connect_to_resolved_address()
{
    halo::main::ClientConnection::game_client_connect_to_resolved_address();
}

uint32_t network_hostname_resolve_thread_proc(char *hostname)
{
    return halo::main::ClientConnection::hostname_resolve_thread_proc(hostname);
}

char network_hostname_resolve_with_timeout(char *hostname)
{
    return halo::main::ClientConnection::hostname_resolve_with_timeout(hostname);
}

void render_frame_all_views(float time_since_tick, float time_since_frame)
{
    halo::main::RenderViews::frame_all_views(time_since_tick, time_since_frame);
}

int render_local_view_count()
{
    return halo::main::RenderViews::local_view_count();
}

void render_pregame_view_initialize()
{
    halo::main::RenderViews::pregame_view_initialize();
}

void render_view_camera_fill(observer_camera *observer, render_view *view)
{
    halo::main::RenderViews::view_camera_fill(observer, view);
}

void screenshot_render(render_view *views)
{
    halo::main::RenderViews::screenshot_render(views);
}

void viewport_split_rect_compute(int32_t view_count, int32_t view_index, Rectangle2D *window, Rectangle2D *out_viewport)
{
    halo::main::RenderViews::viewport_split_rect_compute(view_count, view_index, window, out_viewport);
}

void timedemo_benchmark_update()
{
    halo::main::Timedemo::benchmark_update();
}

}
