/**
 * The main loop, its frame pacer and shutdown, plus per-frame game timing helpers.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "game.h"
#include "main.h"
#include "units.h"
#include "cutscene.h"
#include "win32.h"
#include "networking.h"
#include "saved_games.h"
#include "input.h"
#include "rasterizer.h"
#include "render.h"
#include "objects.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h> 
#include "cache.h"

#include "halo/main/main_loop.hpp"
#include "halo/main/layout.hpp"

extern "C" { void game_engine_flush_pending_simulation_ticks(void); }
extern "C" { uint32_t game_frame_rate_average_update(void); }
extern "C" { void game_timer_reset(void); }
extern "C" { void main_ensure_local_players(void); }
extern "C" { void main_loop_frame_pacer(void); }
extern "C" { void main_loop_shutdown_cleanup(void); }
extern "C" { void main_menu_return_and_reset(void); }

extern "C" { extern main_globals main_globals_data; }
extern "C" { extern game_time_globals *game_time; }
extern "C" { extern cinematic_globals *cinematic_globals_ptr; }
extern "C" { extern void game_engine_advance_simulation_ticks(float dt); }
namespace halo::main {

/**
 * While skip_tick_count is armed and a cinematic isn't suppressing it, runs that many simulation
 * ticks back-to-back at a fixed 1/30s timestep (forcing game_time->speed to 1.0 for the
 * duration, restoring it afterward -- already 1.0 in any networked game), then clears both the
 * tick count and the pending flag.
 *
 * @address 0x4c99e0
 */
void MainLoop::engine_flush_pending_simulation_ticks(void)
{
    if (main_globals_data.skip_tick_count != 0 && cinematic_globals_ptr->in_progress != 0) {
        float saved_speed = (main_globals_data.game_connection == 1 || main_globals_data.game_connection == 2)
                                 ? 1.0f
                                 : game_time->speed;

        game_time->speed = 1.0f;
        while (main_globals_data.skip_tick_count > 0) {
            main_globals_data.skip_tick_count = main_globals_data.skip_tick_count - 1;
            game_engine_advance_simulation_ticks(0.033333335f);
        }
        game_time->speed = saved_speed;
    }
    main_globals_data.skip_tick_count = 0;
    main_globals_data.skip_ticks = 0;
}

}

extern "C" { extern main_frame_rate_average frame_rate_average_data; }
extern "C" { extern int64_t performance_frequency; }
namespace halo::main {

/**
 * Returns the mean of the first `count` recorded frame times (or 1 ms if none have been recorded
 * yet), shifts entries 0..count-2 up one slot into 1..count-1 (making room for a new
 * newest sample at index 0, which the caller is expected to fill in), grows count towards a cap
 * of 16, and refreshes sample_time_ms to the current time in milliseconds.
 *
 * @address 0x4c6e80
 */
uint32_t MainLoop::frame_rate_average_update(void)
{
    uint32_t average;
    int32_t sum;
    int32_t i;
    int64_t counter;

    if (frame_rate_average_data.count < 1) {
        average = 1;
    } else {
        sum = frame_rate_average_data.history[0];
        for (i = frame_rate_average_data.count - 1; i >= 1; i--) {
            sum = sum + frame_rate_average_data.history[i];
            frame_rate_average_data.history[i] = frame_rate_average_data.history[i - 1];
        }
        average = (uint32_t)sum / frame_rate_average_data.count;
    }

    if (frame_rate_average_data.count + 1 < k_main_frame_time_history_count + 1) {
        frame_rate_average_data.count = frame_rate_average_data.count + 1;
    } else {
        frame_rate_average_data.count = k_main_frame_time_history_count;
    }

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    frame_rate_average_data.sample_time_ms = (int32_t)((counter * 1000) / performance_frequency);

    return average;
}

}

namespace halo::main {

/**
 * Re-baselines the frame-timing globals (frame and render counters, plus frame_time_ms) to the
 * current high-resolution timestamp.
 *
 * @address 0x4c9f30
 */
void MainLoop::timer_reset(void)
{
    int64_t counter;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    main_globals_data.frame_counter_low = (uint32_t)counter;
    main_globals_data.frame_counter_high = (uint32_t)(counter >> 32);
    main_globals_data.render_counter_low = (uint32_t)counter;
    main_globals_data.render_counter_high = (uint32_t)(counter >> 32);
    main_globals_data.frame_time_ms = (uint32_t)((counter * 1000) / performance_frequency);
}

}

extern "C" { extern player_globals *local_player_globals; }
extern "C" { extern data_array *player_data; }
extern "C" { extern int16_t local_player_count; }
extern "C" { extern int32_t local_player_find_free_slot_index(void); }
extern "C" { extern datum_index player_new_network(datum_index requested_index, uint32_t machine_index, int16_t local_player_index, uint16_t *identifier_record); }
namespace halo::main {

/**
 * Ensures the current mode's local player(s) exist and are correctly slotted:
 * - on the main menu (main_menu_scenario_loaded set), ensures exactly one local player at
 * local player slot 0, detaching whatever player previously occupied that slot;
 * - otherwise, for each of local_player_count local players, finds a free local-player slot
 * index and creates (or re-keys) a network player into it the same way, but only slot 0 is
 * ever actually wired up to player_globals::local_players (types/game.h pins
 * k_maximum_local_players at 1 for this build; a nonzero slot index from
 * local_player_find_free_slot_index is silently skipped, matching the "-1 < slot && slot < 1"
 *
 * @address 0x4c8800
 */
void MainLoop::ensure_local_players(void)
{
    if (main_globals_data.main_menu_scenario_loaded == 0) {
        int16_t i;
        int32_t slot;
        datum_index new_player;
        datum_index old_player;

        for (i = 0; i < local_player_count; i = i + 1) {
            slot = local_player_find_free_slot_index();
            new_player = player_new_network(k_datum_index_none, 0, (int16_t)slot, 0);
            if (-1 < slot && slot < 1) {
                old_player = local_player_globals->local_players[slot];
                if (old_player != k_datum_index_none) {
                    player *old_p = (player *)((uint8_t *)player_data->data +
                                                datum_slot(old_player) * sizeof(player));
                    old_p->local_player_index = -1;
                }
                local_player_globals->local_players[slot] = new_player;
                if (new_player != k_datum_index_none) {
                    player *new_p = (player *)((uint8_t *)player_data->data +
                                                datum_slot(new_player) * sizeof(player));
                    new_p->local_player_index = (int16_t)slot;
                }
            }
        }
    } else {
        datum_index new_player;
        datum_index old_player;

        new_player = player_new_network(k_datum_index_none, 0, 0, 0);
        old_player = local_player_globals->local_players[0];
        if (old_player != k_datum_index_none) {
            player *old_p = (player *)((uint8_t *)player_data->data +
                                        datum_slot(old_player) * sizeof(player));
            old_p->local_player_index = -1;
        }
        local_player_globals->local_players[0] = new_player;
        if (new_player != k_datum_index_none) {
            player *new_p = (player *)((uint8_t *)player_data->data +
                                        datum_slot(new_player) * sizeof(player));
            new_p->local_player_index = 0;
        }
    }
}

}

extern "C" { extern timedemo_globals timedemo_globals_data; }
extern "C" { extern multiplayer_map_table_entry multiplayer_maps[k_main_multiplayer_map_count]; }
extern "C" { extern console_globals console_globals_data; }
extern "C" { extern int32_t game_time_force_single_tick; }
extern "C" { extern uint8_t main_unknown_696570; }
extern "C" { extern data_array *object_data; }
extern "C" { extern input_abstraction_globals input_globals; }
extern "C" { extern input_event_queue input_event_queue_active; }
extern "C" { extern char network_banlist_full_path[0x104]; }
extern "C" { extern char profile_directory[0x105]; }
extern "C" { extern growable_array ban_list; }
extern "C" { extern growable_array network_buffer_pair_pool; }
extern "C" { extern int32_t shell_nosound; }
extern "C" { extern int32_t novideo_or_connect; }
extern "C" { extern int32_t safe_mode; }
extern "C" { extern int32_t rasterizer_window_requested; }
extern "C" { extern int32_t checkfpu; }
extern "C" { extern uint8_t sound_disabled; }
extern "C" { extern game_state_proc game_state_before_save_proc; }
extern "C" { extern uint8_t game_state_revert_available; }
extern "C" { extern uint8_t game_state_write_in_progress; }
extern "C" { extern uint8_t *game_state_base; }
extern "C" { extern int32_t ui_pause_pending_count_00718fa0; }
extern "C" { extern uint8_t map_download_in_progress; }
extern "C" { extern int32_t network_console_connection_id; }
extern "C" { extern network_bandwidth_graph network_bandwidth_graph_globals; }
extern "C" { extern uint32_t network_bandwidth_graph_default_interval_ms; }
extern "C" { extern uint8_t ui_split_screen; }
extern "C" { extern widget_instance *ui_root_widget[1]; }
extern "C" { extern uint8_t shell_application_inactive; }
extern "C" { extern network_client_globals *network_client; }
extern "C" { extern network_server_globals *network_server; }
extern "C" { extern int16_t network_join_error_code; }
extern "C" { extern uint8_t network_host_handoff_requested; }
extern "C" { extern uint8_t terminal_initialized; }
extern "C" { extern uint32_t update_client_staged[8]; }
extern "C" { extern int32_t update_client_unknown_ec4; }
extern "C" { extern int32_t update_client_staged_count; }
extern "C" { extern uint32_t player_update_log_flags; }
extern "C" { extern int32_t main_render_skip_threshold_ms; }
extern "C" { extern int32_t rasterizer_present_counter_low; }
extern "C" { extern int32_t rasterizer_present_counter_high; }
extern "C" { extern rasterizer_frame_statistics rasterizer_frame_statistics_state; }
extern "C" { extern void console_initialize(void); }
extern "C" { extern uint32_t network_bandwidth_graph_reset(void); }
extern "C" { extern void ui_chat_window_reset_position(void); }
extern "C" { extern void game_initialize(void); }
extern "C" { extern void map_list_add_entry(char *path, int32_t map_id); }
extern "C" { extern void network_banlist_load(void); }
extern "C" { extern void chimera__exec_init(void); }
extern "C" { extern void game_start_new_single_player_map(void); }
extern "C" { extern uint8_t network_autojoin_from_command_line(void); }
extern "C" { extern void movie_play_bink(const char *movie_path); }
extern "C" { extern void main_switch_structure_bsp_and_notify(void); }
extern "C" { extern void game_state_perform_revert(void); }
extern "C" { extern void campaign_level_advance(void); }
extern "C" { extern uint8_t game_engine_attach_players_to_new_bsp(void); }
extern "C" { extern uint8_t game_state_queue_write(uint8_t is_checkpoint); }
extern "C" { extern void hud_display_checkpoint_message(uint8_t is_begin); }
extern "C" { extern void main_level_transition_update(void); }
extern "C" { extern uint8_t scenario_structure_bsp_switch(int16_t structure_bsp_index); }
extern "C" { extern void game_stop_current_map(void); }
extern "C" { extern void input_reset_state_and_axis_configs(void); }
extern "C" { extern void game_start_new_map(void); }
extern "C" { extern void game_engine_init_tick_record_for_mode(void); }
extern "C" { extern void game_engine_reset_all_players(void); }
extern "C" { extern uint8_t game_state_write_profile_file(int32_t size, char *name, const void *buffer); }
extern "C" { extern void console_print_error_va(uint8_t clear_first, const char *format, ...); }
extern "C" { extern void game_state_load_core(char *name); }
extern "C" { extern int16_t cache_file_download_status_get(float *progress_out); }
extern "C" { extern void cache_file_download_finish(void); }
extern "C" { extern uint8_t cache_file_open_by_name(char *name, uint8_t report_fatal_error); }
extern "C" { extern void network_game_client_connect_to_resolved_address(void); }
extern "C" { extern void input_directinput_poll_devices(void); }
extern "C" { extern void input_update_tick(void); }
extern "C" { extern void shell_pump_windows_messages(void); }
extern "C" { extern void input_queue_push_event(int16_t queue_index, ui_input_event *record); }
extern "C" { extern void network_session_host_update(void); }
extern "C" { extern void gcd_think(void); }
extern "C" { extern uint32_t network_update(void); }
extern "C" { extern void network_bandwidth_graph_instance_history_reset(network_bandwidth_graph *graph); }
extern "C" { extern void network_bandwidth_graph_tick(network_bandwidth_graph *graph); }
extern "C" { extern void network_bandwidth_rate_compute(network_bandwidth_graph *graph); }
extern "C" { extern char network_client_update_dispatch(void); }
extern "C" { extern int32_t network_host_shutdown_or_defer(void); }
extern "C" { extern void chat_close(void); }
extern "C" { extern void ui_cursor_update(void); }
extern "C" { extern void interface_tick(void); }
extern "C" { extern uint32_t time_query_performance_counter_ms(void); }
extern "C" { extern void console_process_input_events(void); }
extern "C" { extern uint8_t console_process_queued_input(void); }
extern "C" { extern void console_message_expire_old(void); }
extern "C" { extern void console_update_display(void); }
extern "C" { extern uint8_t console_process_key_events(void); }
extern "C" { extern int32_t game_engine_accumulate_simulation_ticks(float elapsed_seconds, char keep_remainder); }
extern "C" { extern void game_engine_update_local_player_control(int16_t local_player_index, float delta_time, int32_t ticks_this_frame); }
extern "C" { extern uint8_t chat_poll_hotkeys(void); }
extern "C" { extern char update_server_send_update(int32_t ticks, uint8_t frame_time_overflow); }
extern "C" { extern void *data_iterator_next(data_iterator *iterator); }
extern "C" { extern void player_update_history_log_write(uint32_t category_flags, int32_t use_filtered_mask, const char *format, ...); }
extern "C" { extern void camera_update(float dt); }
extern "C" { extern uint8_t camera_is_local_player_default_first_person(void); }
extern "C" { extern void observer_update(float dt, uint8_t add_bob); }
extern "C" { extern void game_engine_update_end_game_sequence(float delta_time); }
extern "C" { extern void main_save_map_private(void); }
extern "C" { extern void timedemo_benchmark_update(void); }
extern "C" { extern void render_frame_all_views(float time_since_tick, float time_since_frame); }
extern "C" { extern void render_pregame_view_initialize(void); }
extern "C" { extern void movie_capture_frame_export(void); }
extern "C" { extern void rasterizer_frame_statistics_sample(rasterizer_frame_statistics *statistics, uint8_t dropped); }
namespace halo::main {

/**
 * The engine main loop. Before the first frame it seeds the default scenario (b30), the timers,
 * the console, the multiplayer map list, the ban list and the -exec script, starts the first
 * session, and plays the three intro movies unless -timedemo, -novideo / -connect, safe mode or
 * -window is in effect. Each frame it then services the requests main_globals carries (bsp
 * switch, revert, level advance, coop respawn, checkpoint write, level transition, map reset,
 * core save / load, main menu return, tick skip, cache file open, connect), polls input, pumps
 * Windows messages, updates the network, paces the frame, runs the interface, tracks idle time,
 * advances the simulation (unless the console holds a local game) and renders, dropping the
 *
 * @address 0x4c7610
 */
void MainLoop::loop(void)
{
    uint8_t local_time[0x10];
    int64_t counter;
    int64_t render_time;
    uint32_t frame_average;
    uint16_t fpu_control;
    int16_t previous_frames;
    int16_t connection;
    uint8_t render_frame;
    float progress;
    uint32_t previous_queue_time;
    ui_input_event idle_event;
    int32_t idle_remaining;
    int32_t ticks;
    float delta;
    uint8_t add_bob;
    data_iterator iterator;
    player *local_player;
    player_update_history *update_history;
    object_header *unit_header;
    uint8_t *unit;
    float leftover_time;
    float frame_delta;
    uint64_t present_counter;
    int32_t elapsed_ms;
    int32_t i;

    GetLocalTime((LPSYSTEMTIME)local_time);
    strncpy(main_globals_data.scenario_path, k_default_scenario_path, k_main_path_length - 1);
    main_globals_data.scenario_path[k_main_path_length - 1] = 0;
    main_globals_data.return_to_main_menu = 1;
    main_globals_data.switch_structure_bsp_index = -1;
    main_globals_data.time_is_running = 1;
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    main_globals_data.last_activity_time_ms = (int32_t)((counter * 1000) / performance_frequency);

    console_initialize();
    network_bandwidth_graph_reset();
    ui_chat_window_reset_position();
    game_initialize();
    for (i = 0; i < k_main_multiplayer_map_count; i++) {
        map_list_add_entry((char *)(uintptr_t)multiplayer_maps[i].name, multiplayer_maps[i].map_id);
    }

    sprintf(network_banlist_full_path, "%s\\%s", profile_directory, k_ban_list_file_name);
    ban_list.element_size = k_ban_list_element_size;
    ban_list.count = 0;
    ban_list.data = 0;
    network_banlist_load();
    network_buffer_pair_pool.element_size = 8;
    network_buffer_pair_pool.count = 0;
    network_buffer_pair_pool.data = 0;

    chimera__exec_init();
    game_start_new_single_player_map();
    game_timer_reset();
    network_autojoin_from_command_line();
    sound_disabled = (uint8_t)shell_nosound;
    if (game_time_force_single_tick == 0 && novideo_or_connect == 0 && safe_mode == 0 &&
        rasterizer_window_requested == 0) {
        movie_play_bink("bungie.bik");
        movie_play_bink("gearbox.bik");
        movie_play_bink("mgs.bik");
    }
    main_unknown_696570 = 0;

    for (;;) {
        frame_average = game_frame_rate_average_update();
        if (checkfpu != 0) {
            fpu_control = k_x87_control_word;
#if defined(_MSC_VER)
            __asm { finit }
            __asm { fldcw fpu_control }
#else
            __asm__ __volatile__("finit\n\tfldcw %0" : : "m"(fpu_control));
#endif
        }

        if (main_globals_data.switch_structure_bsp_index != -1) {
            main_switch_structure_bsp_and_notify();
        }
        if (main_globals_data.lost_map != 0 && game_time->paused == 0) {
            previous_frames = main_globals_data.lost_map_frames;
            main_globals_data.lost_map_frames = (int16_t)(previous_frames + 1);
            if (previous_frames > k_main_revert_delay_frames) {
                main_globals_data.lost_map = 0;
                main_globals_data.lost_map_frames = 0;
                game_state_perform_revert();
            }
        }
        if (main_globals_data.won_map != 0) {
            campaign_level_advance();
        }
        if (main_globals_data.respawn_coop_players != 0 && game_time->paused == 0 &&
            cinematic_globals_ptr->in_progress == 0) {
            previous_frames = main_globals_data.respawn_coop_frames;
            main_globals_data.respawn_coop_frames = (int16_t)(previous_frames + 1);
            if (previous_frames > k_main_respawn_delay_frames &&
                game_engine_attach_players_to_new_bsp() != 0) {
                main_globals_data.respawn_coop_players = 0;
                main_globals_data.respawn_coop_frames = 0;
            }
        }
        if (main_globals_data.save_map_write_pending != 0) {
            game_state_before_save_proc();
            main_globals_data.time_is_running = 0;
            main_globals_data.reset_frame_timers = 0;
            game_state_revert_available = game_state_queue_write(1) != 0;
            main_globals_data.reset_frame_timers = 1;
            hud_display_checkpoint_message(0);
            main_globals_data.save_map_write_pending = 0;
        }
        if (main_globals_data.level_transition != 0) {
            main_level_transition_update();
        }
        if (main_globals_data.revert_map != 0) {
            game_state_perform_revert();
            ui_pause_pending_count_00718fa0 = k_ui_pause_pending_ticks;
            main_globals_data.revert_map = 0;
        }
        if (main_globals_data.revert_map_if_allowed != 0) {
            if (game_state_write_in_progress == 0 && cinematic_globals_ptr->skip_in_progress != 0) {
                game_state_perform_revert();
                ui_pause_pending_count_00718fa0 = k_ui_pause_pending_ticks;
                main_globals_data.revert_map = 0;
            }
            main_globals_data.revert_map_if_allowed = 0;
        }
        if (main_globals_data.reset_map != 0 && game_time->paused == 0) {
            scenario_structure_bsp_switch(0);
            game_stop_current_map();
            input_reset_state_and_axis_configs();
            memset(&input_globals.states[0], 0, sizeof(input_globals.states[0]));
            input_globals.system_key_states[0] = 0;
            input_globals.system_key_states[1] = 0;
            input_globals.idle = 1;
            input_globals.system_key_states[2] = 0;
            game_start_new_map();
            main_ensure_local_players();
            game_engine_init_tick_record_for_mode();
            game_engine_reset_all_players();
            ui_pause_pending_count_00718fa0 = k_ui_pause_pending_ticks;
            main_globals_data.reset_map = 0;
        }
        if (main_globals_data.save_core != 0) {
            if (game_state_write_profile_file(k_game_state_size, (char *)k_core_dump_file_name, game_state_base) != 0) {
                console_print_error_va(0, "saved '%s'", k_core_dump_file_name);
            } else {
                console_print_error_va(0, "error writing '%s'", k_core_dump_file_name);
            }
            main_globals_data.save_core = 0;
        }
        if (main_globals_data.load_core != 0) {
            game_state_load_core((char *)k_core_dump_file_name);
            main_globals_data.load_core = 0;
        }
        if (main_globals_data.return_to_main_menu != 0) {
            main_menu_return_and_reset();
        }
        if (main_globals_data.unknown_058 != 0) {
            main_globals_data.unknown_058 = 0;
        }
        if (main_globals_data.skip_ticks != 0) {
            game_engine_flush_pending_simulation_ticks();
        }
        if (main_globals_data.cache_file_open_pending != 0) {
            if (map_download_in_progress != 0) {
                if (cache_file_download_status_get(&progress) == 1) {
                    cache_file_download_finish();
                }
                if (map_download_in_progress != 0) {
                    goto cache_file_open_done;
                }
            }
            cache_file_open_by_name(main_globals_data.pending_cache_file_name, 0);
            main_globals_data.cache_file_open_pending = 0;
        }
    cache_file_open_done:
        if (main_globals_data.connect_pending != 0) {
            network_game_client_connect_to_resolved_address();
        }

        connection = main_globals_data.game_connection;
        input_directinput_poll_devices();
        if (game_time_force_single_tick == 0) {
            input_update_tick();
        }
        shell_pump_windows_messages();
        if (main_globals_data.quit != 0) {
            break;
        }

        if (input_event_queue_active.enabled != 0) {
            previous_queue_time = input_event_queue_active.start_time;
            QueryPerformanceCounter((LARGE_INTEGER *)&counter);
            input_event_queue_active.start_time = (uint32_t)((counter * 1000) / performance_frequency);
            if (input_event_queue_active.last_event_time < previous_queue_time && input_event_queue_active.enabled != 0) {
                memset(&idle_event, 0, sizeof(idle_event));
                input_queue_push_event(0, &idle_event);
            }
        }
        if (connection == _game_connection_network_server) {
            network_session_host_update();
            if (network_console_connection_id != -1) {
                gcd_think();
            }
        }
        network_update();
        if (connection == _game_connection_network_client ||
            connection == _game_connection_network_server ||
            (ui_split_screen == 1 && ui_root_widget[0] != 0 &&
             strcmp(ui_root_widget[0]->name, k_main_menu_widget_name) == 0)) {
            if (network_bandwidth_graph_globals.sample_interval_ms !=
                network_bandwidth_graph_default_interval_ms) {
                network_bandwidth_graph_globals.sample_interval_ms =
                    network_bandwidth_graph_default_interval_ms;
                network_bandwidth_graph_instance_history_reset(&network_bandwidth_graph_globals);
            }
            network_bandwidth_graph_tick(&network_bandwidth_graph_globals);
            network_bandwidth_rate_compute(&network_bandwidth_graph_globals);
        }

        if (shell_application_inactive != 0 && connection != _game_connection_network_client &&
            connection != _game_connection_network_server) {
            goto frame_end;
        }

        render_frame = 1;
        if (connection == _game_connection_network_client) {
            if (network_client_update_dispatch() == 0) {
                if (network_client->disconnect_reason == 8) {
                    if (network_join_error_code == -1) {
                        network_join_error_code = 4;
                    }
                } else if (network_join_error_code == -1) {
                    network_join_error_code = 6;
                }
                network_host_handoff_requested = 1;
                chat_close();
            }
        } else if (connection == _game_connection_network_server) {
            if (((network_server->flags & 4) == 0 && (uint8_t)network_client_update_dispatch() != 1) ||
                (uint8_t)network_host_shutdown_or_defer() != 1) {
                if (network_join_error_code == -1) {
                    network_join_error_code = 1;
                }
                network_host_handoff_requested = 1;
                chat_close();
            }
        } else if (connection == _game_connection_film_playback) {
            break;
        }

        main_loop_frame_pacer();
        ui_cursor_update();
        interface_tick();

        if (input_globals.idle == 0 || console_globals_data.active != 0) {
            QueryPerformanceCounter((LARGE_INTEGER *)&counter);
            main_globals_data.last_activity_time_ms =
                (int32_t)((counter * 1000) / performance_frequency);
        } else if (game_time->initialized != 0 && (game_time->active != 0 || game_time->paused != 0) &&
                   game_time->paused == 0 && cinematic_globals_ptr->in_progress != 0) {
            QueryPerformanceCounter((LARGE_INTEGER *)&counter);
            main_globals_data.last_gameplay_time_ms =
                (int32_t)((counter * 1000) / performance_frequency);
        } else if (main_globals_data.idle_timeout_ms > 0) {
            QueryPerformanceCounter((LARGE_INTEGER *)&counter);
            idle_remaining = main_globals_data.idle_timeout_ms -
                (int32_t)((counter * 1000) / performance_frequency) +
                main_globals_data.last_activity_time_ms;
            QueryPerformanceCounter((LARGE_INTEGER *)&counter);
            if (idle_remaining <= 0 &&
                main_globals_data.last_gameplay_time_ms -
                    (int32_t)((counter * 1000) / performance_frequency) +
                    k_main_idle_gameplay_grace_ms <= 0) {
                if (ui_split_screen != 0) {
                    main_globals_data.return_to_main_menu = 0;
                    main_globals_data.idle_timeout_reached = 1;
                    main_globals_data.level_transition = 1;
                } else {
                    main_globals_data.switch_structure_bsp_index = -1;
                    main_globals_data.save_map = 0;
                    main_globals_data.return_to_main_menu = 1;
                }
                main_globals_data.last_activity_time_ms = (int32_t)time_query_performance_counter_ms();
            }
        }

        if (game_time->initialized == 0 || (game_time->active == 0 && game_time->paused == 0)) {
            if (game_time_force_single_tick == 0 && shell_application_inactive == 0) {
                render_pregame_view_initialize();
            }
            if (main_globals_data.disable_frame_output == 0) {
                movie_capture_frame_export();
            }
            goto frame_end;
        }

        if (terminal_initialized != 0) {
            console_process_input_events();
            console_process_queued_input();
            if (console_globals_data.active == 0) {
                console_message_expire_old();
            }
            console_update_display();
        }
        if (console_process_key_events() == 0 || main_globals_data.game_connection != 0) {
            delta = (float)main_globals_data.time_is_running * main_globals_data.frame_delta_time;
            ticks = game_engine_accumulate_simulation_ticks(delta, 1);
            memset(update_client_staged, 0, sizeof(update_client_staged));
            update_client_staged_count = 0;
            update_client_unknown_ec4 = ticks;
            game_engine_update_local_player_control(0, delta, ticks);
            if (main_globals_data.game_connection == _game_connection_network_client ||
                (main_globals_data.game_connection == _game_connection_network_server &&
                 (network_server->flags & 4) == 0)) {
                chat_poll_hotkeys();
                if (update_server_send_update(ticks, main_globals_data.frame_time_overflow) == 0) {
                    if (network_join_error_code == -1) {
                        network_join_error_code = 1;
                    }
                    network_host_handoff_requested = 1;
                    chat_close();
                }
            }
            game_engine_advance_simulation_ticks(delta);

            if (main_globals_data.game_connection == _game_connection_network_client &&
                player_update_log_flags != 0) {
                iterator.data = player_data;
                iterator.next_index = 0;
                iterator.index = k_datum_index_none;
                iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
                while ((local_player = (player *)data_iterator_next(&iterator)) != 0) {
                    if (local_player->local_player_index == -1) {
                        continue;
                    }
                    update_history = (player_update_history *)network_client->update_history;
                    if (local_player->unit != k_datum_index_none && update_history != 0 &&
                        update_history->tail != 0) {
                        unit_header = (object_header *)object_data->data + datum_slot(local_player->unit);
                        unit = (uint8_t *)unit_header->data;
                        player_update_history_log_write(0x10, 0,
                            "[%d]: Update [%d] ([%d]): ([%f] [%f] [%f]), ([%f] [%f]), ([%f] [%f])\n",
                            game_time->game_time, update_history->tail->update_id,
                            update_history->tail->tick_count,
                            (double)((object *)unit)->position.x, (double)((object *)unit)->position.y,
                            (double)((object *)unit)->position.z, (double)((unit_data *)(unit + k_unit_data_offset))->throttle.i,
                            (double)((unit_data *)(unit + k_unit_data_offset))->throttle.j, (double)((object *)unit)->velocity.i,
                            (double)((object *)unit)->velocity.j);
                    }
                    break;
                }
            }

            render_frame = 0;
            if (shell_application_inactive == 0 &&
                (main_globals_data.main_menu_scenario_loaded != 0 ||
                 main_globals_data.time_is_running != 0)) {
                render_frame = 1;
            }
            camera_update((float)main_globals_data.time_is_running * main_globals_data.frame_delta_time);
            add_bob = camera_is_local_player_default_first_person();
            observer_update((float)main_globals_data.time_is_running * main_globals_data.frame_delta_time,
                add_bob);
            game_engine_update_end_game_sequence(
                (float)main_globals_data.time_is_running * main_globals_data.frame_delta_time);
        }

        if (main_globals_data.save_map != 0) {
            main_save_map_private();
        }
        if (main_render_skip_threshold_ms != -1) {
            if (main_render_skip_threshold_ms <= k_minimum_render_skip_threshold_ms) {
                main_render_skip_threshold_ms = k_minimum_render_skip_threshold_ms;
            }
            if (frame_average >= (uint32_t)main_render_skip_threshold_ms) {
                render_frame = 0;
            }
        }

        if (game_time_force_single_tick != 0) {
            if (game_time->game_time == timedemo_globals_data.last_game_time) {
                goto frame_end;
            }
            timedemo_globals_data.last_game_time = game_time->game_time;
            timedemo_benchmark_update();
        } else if (render_frame == 0) {
            present_counter = ((uint64_t)(uint32_t)rasterizer_present_counter_high << 32 |
                (uint32_t)rasterizer_present_counter_low) + 1;
            rasterizer_present_counter_low = (int32_t)(uint32_t)present_counter;
            rasterizer_present_counter_high = (int32_t)(uint32_t)(present_counter >> 32);
            rasterizer_frame_statistics_sample(&rasterizer_frame_statistics_state, 1);
            goto frame_end;
        }

        if (main_globals_data.disable_frame_output != 0) {
            goto frame_end;
        }
        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        if (game_time->paused == 0 && console_globals_data.active == 0) {
            leftover_time = game_time->leftover_time;
            render_time = counter - ((int64_t)main_globals_data.render_counter_high << 32 |
                main_globals_data.render_counter_low);
            frame_delta = (float)render_time / (float)performance_frequency;
            if (game_time_force_single_tick != 0) {
                frame_delta = 1.0f / 30.0f;
            }
            if (main_globals_data.game_connection == _game_connection_local) {
                if (frame_delta > 1.0f / 30.0f) {
                    frame_delta = 1.0f / 30.0f;
                }
            } else if (frame_delta > 1.0f) {
                frame_delta = 1.0f;
            }
        } else {
            leftover_time = 0.0f;
            frame_delta = 0.0f;
        }
        render_frame_all_views(leftover_time, frame_delta);
        main_globals_data.render_counter_low = (uint32_t)counter;
        main_globals_data.render_counter_high = (uint32_t)(counter >> 32);
        if (main_globals_data.disable_frame_output == 0) {
            movie_capture_frame_export();
        }

    frame_end:
        if (main_globals_data.quit != 0) {
            break;
        }
        if (main_globals_data.reset_frame_timers != 0) {
            main_globals_data.reset_frame_timers = 0;
            QueryPerformanceCounter((LARGE_INTEGER *)&counter);
            main_globals_data.frame_counter_low = (uint32_t)counter;
            main_globals_data.frame_counter_high = (uint32_t)(counter >> 32);
            main_globals_data.render_counter_low = (uint32_t)counter;
            main_globals_data.render_counter_high = (uint32_t)(counter >> 32);
            main_globals_data.frame_time_ms = (uint32_t)((counter * 1000) / performance_frequency);
            main_globals_data.time_is_running = 1;
        }
        if (shell_application_inactive != 0) {
            MsgWaitForMultipleObjects(0, 0, 0,
                (main_globals_data.game_connection > 0 && main_globals_data.game_connection <= 2) ? 20 : 100,
                0xff );
        }
        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        elapsed_ms = (int32_t)((counter * 1000) / performance_frequency) -
            frame_rate_average_data.sample_time_ms;
        frame_rate_average_data.history[0] = elapsed_ms;
        if ((uint32_t)elapsed_ms > k_main_frame_time_clamp_ms) {
            frame_rate_average_data.history[0] = k_main_frame_time_clamp_ms;
        }
    }
    main_loop_shutdown_cleanup();
}

}

extern "C" { extern uint8_t unknown_006894ba; }
extern "C" { extern uint8_t unknown_00710301; }
namespace halo::main {

/**
 * Paces the main loop to roughly 30 FPS: while capturing isn't running and either the video
 * options' frame limiter is on or a cinematic is active, busy-waits (sleeping 10ms at a time
 * once more than 12ms remains until the 1/30s mark, otherwise polling with no sleep) until at
 * least 1/30s has elapsed since the last frame. Then computes this frame's delta time: clamped
 * to 0..1s normally, but forced to a fixed 1/15s or 1/30s step whenever it would otherwise run
 * slower than that (skipped entirely while networked, movie-capturing, or during a cinematic).
 * Finally re-baselines the frame counter and frame_time_ms from the current timestamp -- or, if
 * a timedemo single-tick is queued, advances the counter by exactly one 1/30s step instead of
 *
 * @address 0x4c9f90
 */
void MainLoop::loop_frame_pacer(void)
{
    uint8_t pacing;
    int64_t now;
    double elapsed_seconds;
    float delta;

    pacing = (game_time_force_single_tick == 0 &&
              (unknown_006894ba != 0 || cinematic_globals_ptr->in_progress != 0))
                 ? 1
                 : 0;

    do {
        int64_t elapsed_ticks;
        uint32_t sleep_ms;

        QueryPerformanceCounter((LARGE_INTEGER *)&now);
        elapsed_ticks = now - (((int64_t)main_globals_data.frame_counter_high << 32) |
                                main_globals_data.frame_counter_low);
        elapsed_seconds = (double)elapsed_ticks / (double)performance_frequency;

        sleep_ms = (!pacing || (0.03333333507180214 - elapsed_seconds <= 0.012)) ? 0 : 10;
        Sleep(sleep_ms);
    } while (pacing && elapsed_seconds < 0.03333333507180214);

    delta = main_globals_data.movie_frame_delta_time;
    if (main_globals_data.movie_frame_bitmap == 0) {
        main_globals_data.frame_time_overflow = elapsed_seconds > 1.0;
        if (elapsed_seconds >= 0.0) {
            if (elapsed_seconds > 1.0) {
                elapsed_seconds = 1.0;
            }
        } else {
            elapsed_seconds = 0.0;
        }

        if (main_globals_data.game_connection != 0 || cinematic_globals_ptr->in_progress != 0) {
            goto apply;
        }
        if (unknown_00710301 == 0) {
            if (elapsed_seconds <= 0.06666666666666667) {
                goto apply;
            }
            elapsed_seconds = 0.06666666666666667;
            goto apply;
        } else {
            if (elapsed_seconds <= 0.03333333333333333) {
                goto apply;
            }
            elapsed_seconds = 0.03333333333333333;
            goto apply;
        }
    }
    elapsed_seconds = (double)delta;

apply:
    if (game_time_force_single_tick != 0) {
        int64_t frequency;
        int64_t step;
        int64_t new_counter;

        QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);
        step = frequency / 30;
        new_counter = step + (((int64_t)main_globals_data.frame_counter_high << 32) |
                               main_globals_data.frame_counter_low);
        main_globals_data.frame_counter_low = (uint32_t)new_counter;
        main_globals_data.frame_counter_high = (uint32_t)(new_counter >> 32);
        main_globals_data.frame_delta_time = 0.033333335f;
        main_globals_data.frame_time_ms = main_globals_data.frame_time_ms + k_fallback_frame_time_ms;
        return;
    }

    main_globals_data.frame_counter_low = (uint32_t)now;
    main_globals_data.frame_counter_high = (uint32_t)(now >> 32);
    main_globals_data.frame_time_ms = (uint32_t)((now * 1000) / performance_frequency);
    main_globals_data.frame_delta_time = (float)elapsed_seconds;
}

}

extern "C" { extern uint8_t network_server_host_valid; }
extern "C" { extern void map_list_free_all(void); }
extern "C" { extern void network_buffer_pair_pool_clear(void); }
extern "C" { extern void network_client_globals_dispose(void); }
extern "C" { extern void network_game_server_host_dispose(network_server_globals *server); }
extern "C" { extern void game_dispose(void); }
extern "C" { extern void console_deactivate(void); }
namespace halo::main {

/**
 * Final teardown when the main loop exits: releases the map cache index, resets the ban list,
 * disposes networking state appropriate to the current connection (client vs. host, disposing
 * the host and its server object when hosting), then stops the current map, deactivates the
 * console and closes chat either way.
 *
 * @address 0x4c9e90
 */
void MainLoop::loop_shutdown_cleanup(void)
{
    map_list_free_all();
    ban_list.element_size = -1;
    ban_list.count = -1;
    if (ban_list.data != 0) {
        GlobalFree(ban_list.data);
        ban_list.data = 0;
    }

    network_buffer_pair_pool_clear();
    if (main_globals_data.game_connection == 1) {
        network_client_globals_dispose();
    } else if (main_globals_data.game_connection == 2) {
        network_client_globals_dispose();
        if (network_server != 0) {
            network_game_server_host_dispose(network_server);
            network_server = 0;
            network_server_host_valid = 0;
            game_stop_current_map();
            game_dispose();
            console_deactivate();
            chat_close();
            return;
        }
    }
    game_stop_current_map();
    game_dispose();
    console_deactivate();
    chat_close();
}

}

extern "C" { extern uint8_t main_menu_music_pending; }
extern "C" { extern datum_index tag_lookup(tag_group group, char *path); }
extern "C" { extern void sound_looping_stop(datum_index sound_tag); }
namespace halo::main {

/**
 * Stops the main menu's title theme music if it is currently playing, and clears the "showing
 * the UI map" state (ui_split_screen, main_menu_scenario_loaded) and the menu-navigation input
 * mode bit.
 *
 * @address 0x4c8b40
 */
void MainLoop::menu_music_stop(void)
{
    if (main_menu_music_pending == 1) {
        datum_index sound_tag = tag_lookup(k_looping_sound_group, (char *)"sound\\music\\title1\\title1");
        if (sound_tag != k_datum_index_none) {
            sound_looping_stop(sound_tag);
        }
        main_menu_music_pending = 0;
    }
    ui_split_screen = 0;
    main_globals_data.main_menu_scenario_loaded = 0;
    input_globals.mode_flags = input_globals.mode_flags & (uint8_t)~_input_mode_menu_bit;
}

}

extern "C" { extern Scenario *global_scenario; }
extern "C" { extern int32_t interface_loading_screen_address_a; }
extern "C" { extern int32_t interface_loading_screen_address_b; }
extern "C" { extern int32_t join_ui_state; }
extern "C" { extern int32_t interface_loading_screen_progress; }
extern "C" { extern uint16_t progress_screen_text[0x20]; }
extern "C" { extern uint16_t progress_screen_subtext[0x20]; }
extern "C" { extern int32_t interface_loading_screen_request_id; }
extern "C" { extern uint8_t ui_network_wait_timed_out; }
extern "C" { extern uint8_t ui_network_wait_active; }
extern "C" { extern int32_t ui_network_wait_start_time; }
extern "C" { extern void chimera__load_ui_map(char play_title_music); }
extern "C" { extern void chimera__load_main_menu(void); }
extern "C" { extern void predicted_resource_list_touch(TagReflexive *resources); }
extern "C" { extern void hud_chat_listbox_clear(void); }
extern "C" { extern void update_queues_dispose(void); }
extern "C" { extern void update_server_new(void); }
extern "C" { extern void update_server_dispose(void); }
extern "C" { extern void hs_dispose_dynamic_globals(void); }
extern "C" { extern void hs_scenario_scripts_initialize(void); }
namespace halo::main {

/**
 * Tears down the current game session and returns to the main menu: (re)loads the UI map if it
 * is not already loaded, always (re)loads the main menu widget itself and touches the predicted
 * resource list if a scenario was loaded, resets the loading screen and network-wait UI state,
 * clears the chat box, tears down and re-creates the update-queue/server bookkeeping, resets the
 * game clock, disposes and re-initializes the hs dynamic globals and scenario scripts, clears
 * main_globals.return_to_main_menu now that it has been serviced, and re-arms the menu-navigation
 * input mode bit.
 *
 * @address 0x4c8a60
 */
void MainLoop::menu_return_and_reset(void)
{
    if (main_globals_data.main_menu_scenario_loaded == 0) {
        chimera__load_ui_map(0);
    }
    chimera__load_main_menu();
    if (global_scenario != 0) {
        predicted_resource_list_touch(&global_scenario->predicted_resources);
    }

    interface_loading_screen_address_a = -1;
    interface_loading_screen_address_b = -1;
    join_ui_state = 0;
    interface_loading_screen_progress = 0;
    progress_screen_text[0] = 0;
    progress_screen_subtext[0] = 0;
    interface_loading_screen_request_id = -1;

    hud_chat_listbox_clear();
    ui_network_wait_active = 0;
    ui_network_wait_start_time = -1;
    ui_network_wait_timed_out = 0;

    update_queues_dispose();
    update_server_new();
    update_server_dispose();

    if (game_time != 0) {
        game_time->initialized = 0;
        game_time->active = 0;
    }
    memset(game_time, 0, sizeof(game_time_globals));
    game_time->initialized = 1;

    game_engine_init_tick_record_for_mode();
    hs_dispose_dynamic_globals();
    hs_scenario_scripts_initialize();

    main_globals_data.return_to_main_menu = 0;
    input_globals.mode_flags = input_globals.mode_flags | 2;
}

}
