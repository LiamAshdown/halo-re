// main_loop  (Ghidra: game_state_save_core 0x4c7610 plus the split-off tail weapon_stop_reload
//   0x4c7f10; renamed, CEA main_loop)
// address 0x4c7610, size 3364 bytes (0x4c7610..0x4c8333; Ghidra's 0x4c7f10 "function" is the
//   `add esp,4` after the call to game_engine_advance_simulation_ticks inside this body)
// name confidence: 0.8   rewrite confidence: 0.7
// evidence: rewritten from the raw disassembly 0x4c7610..0x4c8333 (phase 4 review), with the
//   Ghidra decompile kept below. types/main.h main_globals / main_frame_rate_average /
//   timedemo_globals / multiplayer_map_table_entry; out/phase4/main_types_notes.md. The only
//   caller is shell_winmain. Strings: levels\b30\b30, banned.txt, bungie.bik, gearbox.bik,
//   mgs.bik, core.bin, saved '%s', error writing '%s', the_main_menu, and the player update log
//   format 0x0066b3a8.
// register convention: cdecl, no arguments (frame aligned to 8, `and esp,-8`). Register
//   arguments passed to callees, all read off this body:
//   map_list_add_entry 0x4950c0        EAX = name, stack = map_id (0x4c76b1)
//   hud_display_checkpoint_message     DL = 0 (0x4c786d `xor dl,dl`)
//   scenario_structure_bsp_switch      SI = 0 (0x4c7905 `xor esi,esi`; the callee compares si)
//   game_state_write_profile_file      EDI = 0x440000, stack = "core.bin", game_state_base
//   console_print_error_va             AL = 0, stack = format, "core.bin"
//   game_state_load_core               EAX = "core.bin"
//   cache_file_download_status_get     EAX = &progress (0x4c7a1e)
//   cache_file_open_by_name            EAX = pending_cache_file_name, stack = 0
//   input_queue_push_event             EAX = 0, EDI = &event (8 zero bytes)
//   network_bandwidth_graph_instance_history_reset / network_bandwidth_rate_compute  ESI = graph;
//   network_bandwidth_graph_tick       EAX = graph
//   data_iterator_next                 EDI = iterator
//   player_update_history_log_write    EAX = 0x10, ECX = 0, stack = format, ...
//   rasterizer_frame_statistics_sample EBX = 0x007c30a0, stack = 1
// UNSURE: the player update log block reads unit fields +0x5c..+0x6c and +0x278/+0x27c that are
//   only named by the log format; 0x006894bc (render skip threshold, ms), 0x006894b0 (bandwidth
//   graph sample interval default) and 0x0069fdfc have no established owner; the idle quit
//   (main_globals.idle_timeout_ms) is never armed in this build. The data iterator frame is
//   0x10 bytes: types/memory.h data_iterator, whose +0x0c signature is data ^ 'iter'.
// reconciled: R33 game_time_globals.unknown_00 -> initialized (uint8 at +0x00, same byte)
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original; the separate iterator_signature local is folded into it
// reconciled: R10 profile_directory is char[0x105] (k_profile_directory_storage_size; shell zeroes 0x41 dwords + 1 byte at 0x540ef9)

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include "rasterizer.h"
#include "render.h"
#include "objects.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h> // uintptr_t
#include "units.h"
#include "cutscene.h"
#include "fn_game.h"
#include "fn_gamespy.h"
#include "fn_camera.h"
#include "fn_cache.h"
#include "fn_scenario.h"

extern main_globals main_globals_data;                      // 0x00719700
extern main_frame_rate_average frame_rate_average_data;     // 0x00719ab0
extern timedemo_globals timedemo_globals_data;              // 0x00719afc
extern multiplayer_map_table_entry multiplayer_maps[k_main_multiplayer_map_count]; // 0x0068e588
extern console_globals console_globals_data;                // 0x006b7020
extern int32_t game_time_force_single_tick;                 // 0x007196d8, -timedemo frame counter
extern uint8_t main_unknown_696570;                         // 0x00696570
extern int64_t performance_frequency;               // 0x006ac8f8, foreign (math module)
extern game_time_globals *game_time;                        // 0x006f1d6c, foreign (game module)
extern cinematic_globals *cinematic_globals_ptr; // 0x006f187c
extern data_array *player_data;                             // 0x0087a480, foreign (game module)
extern data_array *object_data;                             // 0x008603b0, foreign (objects module)
extern input_abstraction_globals input_globals;             // 0x00710328, foreign (input module)
extern input_event_queue input_event_queue_active;                       // 0x00712cc0, foreign (input module)
extern char network_banlist_full_path[0x104];               // 0x0071c308, foreign (networking)
extern char profile_directory[0x105];                       // 0x006ac900, foreign (shell)
extern growable_array ban_list;                             // 0x006b859c, foreign (networking)
extern growable_array network_buffer_pair_pool;             // 0x006b85a8, foreign (networking), UNSURE name
extern int32_t shell_nosound;                               // 0x007196e4, foreign (shell)
extern int32_t novideo_or_connect;                          // 0x007196e8, foreign (shell)
extern int32_t safe_mode;                                   // 0x007196f4, foreign (shell)
extern int32_t rasterizer_window_requested;                 // 0x0071d1a8, foreign (rasterizer)
extern int32_t checkfpu;                                    // 0x0071d1a4, foreign (shell)
extern uint8_t sound_disabled;                              // 0x007252b6, foreign (sound)
extern game_state_proc game_state_before_save_proc;         // 0x0069e7ac, foreign (saved_games)
extern uint8_t game_state_revert_available;                 // 0x006e2dd9, foreign (saved_games)
extern uint8_t game_state_write_in_progress;                // 0x006e3000, foreign (saved_games)
extern uint8_t *game_state_base;                            // 0x006e2dc8, foreign (saved_games)
extern int32_t ui_pause_pending_count_00718fa0;             // 0x00718fa0, foreign, TYPES-GAP
extern uint8_t map_download_in_progress;                    // 0x006ac470, foreign (cache)
extern int32_t network_console_connection_id;                            // 0x0069fdfc, foreign, UNSURE
extern network_bandwidth_graph network_bandwidth_graph_globals; // 0x00719ce0, foreign (networking)
extern uint32_t network_bandwidth_graph_default_interval_ms;  // 0x006894b0, foreign, UNSURE name
extern uint8_t ui_split_screen;                             // 0x00718fc9, foreign (interface)
extern widget_instance *ui_root_widget[1];                  // 0x00718f94, foreign (interface)
extern uint8_t shell_application_inactive;                  // 0x00721e8c, foreign (shell)
extern network_client_globals *network_client;              // 0x0071c2d8, foreign (networking)
extern network_server_globals *network_server;              // 0x0071c2d4, foreign (networking)
extern int16_t network_join_error_code;                     // 0x00718fa4, foreign (interface)
extern uint8_t network_host_handoff_requested;                      // 0x0071c2de, foreign (networking)
extern uint8_t terminal_initialized;                        // 0x006b2efc, foreign (interface)
extern uint32_t update_client_staged[8];               // 0x006f7ea4, foreign, UNSURE identity
extern int32_t update_client_unknown_ec4;                   // 0x006f7ec4, foreign, UNSURE
extern int32_t update_client_staged_count;                  // 0x006f7ecc, foreign (networking)
extern uint32_t player_update_log_flags;                    // 0x00710310, foreign (read as a dword here)
extern int32_t main_render_skip_threshold_ms;               // 0x006894bc, UNSURE owner; -1 disables
extern int32_t rasterizer_present_counter_low;              // 0x0069c648, foreign (rasterizer)
extern int32_t rasterizer_present_counter_high;             // 0x0069c64c
extern rasterizer_frame_statistics rasterizer_frame_statistics_state; // 0x007c30a0, foreign (render)

extern void console_initialize(void);                                       // 0x4c62d0 (Ghidra splits it at 0x4c62f0 / 0x4c6340)
extern uint32_t network_bandwidth_graph_reset(void);                        // 0x4d7980, foreign (networking)
extern void ui_chat_window_reset_position(void);                            // 0x4aa6b0, foreign (interface)

extern void map_list_add_entry(char *path, int32_t map_id);                 // 0x4950c0, foreign (interface)
    // blam-cc: EAX -> path, stack -> map_id
extern void network_banlist_load(void);                                     // 0x4e3160, foreign (networking)
extern void chimera__exec_init(void);                                       // this module, 0x4c6390
extern void game_start_new_single_player_map(void);                         // this module, 0x4c9dd0
extern void game_timer_reset(void);                                         // this module, 0x4c9f30
extern uint8_t network_autojoin_from_command_line(void);                    // 0x4c9c80, foreign (interface)
extern void movie_play_bink(const char *movie_path);                        // this module, 0x43ed20
extern uint32_t game_frame_rate_average_update(void);                       // this module, 0x4c6e80
extern void main_switch_structure_bsp_and_notify(void);                     // this module, 0x4c9b60
extern void game_state_perform_revert(void);                                // 0x538200, foreign (saved_games)
extern void campaign_level_advance(void);                                   // this module, 0x4c9bd0

extern uint8_t game_state_queue_write(uint8_t is_checkpoint);               // 0x538700, returns in AL
extern void hud_display_checkpoint_message(uint8_t is_begin);               // 0x4aa310, foreign (interface)
    // blam-cc: DL -> is_begin
extern void main_level_transition_update(void);                             // this module, 0x4c9770

    // blam-cc: SI -> structure_bsp_index

extern void input_reset_state_and_axis_configs(void);                       // 0x490aa0, foreign (input)

extern void main_ensure_local_players(void);                                // this module, 0x4c8800


extern uint8_t game_state_write_profile_file(int32_t size, char *name, const void *buffer); // 0x5393d0
    // blam-cc: EDI -> size, stack -> name, buffer
extern void console_print_error_va(uint8_t clear_first, const char *format, ...); // this module, 0x4c67c0
    // blam-cc: AL -> clear_first
extern void game_state_load_core(char *name);                               // 0x538390, foreign (saved_games)
    // blam-cc: EAX -> name
extern void main_menu_return_and_reset(void);                               // this module, 0x4c8a60
extern void game_engine_flush_pending_simulation_ticks(void);               // this module, 0x4c99e0
extern int16_t cache_file_download_status_get(float *progress_out);        // 0x4434a0, foreign (cache)
    // blam-cc: EAX -> progress_out


    // blam-cc: EAX -> name, stack -> report_fatal_error
extern void network_game_client_connect_to_resolved_address(void);          // this module, 0x4c8660
extern void input_directinput_poll_devices(void);                           // 0x490760, foreign (input)
extern void input_update_tick(void);                                        // 0x48b4b0, foreign (input)
extern void shell_pump_windows_messages(void);                              // 0x541a20, foreign (shell)
extern void input_queue_push_event(int16_t queue_index, ui_input_event *record); // 0x492340, foreign (input)
    // blam-cc: EAX -> queue_index, EDI -> record
extern void network_session_host_update(void);                                             // 0x577940, foreign

extern uint32_t network_update(void);                                       // 0x4418d0, foreign (networking)
extern void network_bandwidth_graph_instance_history_reset(network_bandwidth_graph *graph); // 0x4d8080
    // blam-cc: ESI -> graph
extern void network_bandwidth_graph_tick(network_bandwidth_graph *graph);   // 0x4d84d0, blam-cc: EAX -> graph
extern void network_bandwidth_rate_compute(network_bandwidth_graph *graph); // 0x4d8540, blam-cc: ESI -> graph
extern char network_client_update_dispatch(void);                           // 0x4dded0, foreign (networking)
extern int32_t network_host_shutdown_or_defer(void);                        // 0x4ddd90, result tested in AL
extern void chat_close(void);                                               // 0x4aa900, foreign (interface)
extern void main_loop_frame_pacer(void);                                    // this module, 0x4c9f90
extern void ui_cursor_update(void);                                         // 0x4972c0, foreign (interface)
extern void interface_tick(void);                                           // 0x497e80, foreign (interface)
extern uint32_t time_query_performance_counter_ms(void);                    // 0x449210, foreign (math)
extern void console_process_input_events(void);                             // 0x496c80, foreign (interface)
extern uint8_t console_process_queued_input(void);                          // 0x4965e0, foreign (interface)
extern void console_message_expire_old(void);                               // 0x4966e0, foreign (interface)
extern void console_update_display(void);                                   // 0x496d40, foreign (interface)
extern uint8_t console_process_key_events(void);                            // this module, 0x4c65c0

extern void game_engine_update_local_player_control(int16_t local_player_index, float delta_time,
    int32_t ticks_this_frame);                                              // 0x471ae0, foreign (game)
extern uint8_t chat_poll_hotkeys(void);                                     // 0x4aaa90, foreign (interface)
extern char update_server_send_update(int32_t ticks, uint8_t frame_time_overflow); // 0x4ddfb0, foreign (networking)

extern void *data_iterator_next(data_iterator *iterator);                   // 0x4d05d0, blam-cc: EDI -> iterator
extern void player_update_history_log_write(uint32_t category_flags, int32_t use_filtered_mask,
    const char *format, ...);                                               // 0x4e5ea0, blam-cc: EAX, ECX


extern void main_save_map_private(void);                                    // this module, 0x4c9a70
extern void timedemo_benchmark_update(void);                                // this module, 0x4c6f30
extern void render_frame_all_views(float time_since_tick, float time_since_frame); // this module, 0x4c9260
extern void render_pregame_view_initialize(void);                           // this module, 0x4c8f20
extern void movie_capture_frame_export(void);                               // this module, 0x4c9530
extern void rasterizer_frame_statistics_sample(rasterizer_frame_statistics *statistics, uint8_t dropped); // 0x512530
    // blam-cc: EBX -> statistics, stack -> dropped
extern void main_loop_shutdown_cleanup(void);                               // this module, 0x4c9e90

// The engine main loop. Before the first frame it seeds the default scenario (b30), the timers,
// the console, the multiplayer map list, the ban list and the -exec script, starts the first
// session, and plays the three intro movies unless -timedemo, -novideo / -connect, safe mode or
// -window is in effect. Each frame it then services the requests main_globals carries (bsp
// switch, revert, level advance, coop respawn, checkpoint write, level transition, map reset,
// core save / load, main menu return, tick skip, cache file open, connect), polls input, pumps
// Windows messages, updates the network, paces the frame, runs the interface, tracks idle time,
// advances the simulation (unless the console holds a local game) and renders, dropping the
// frame when the running frame time average reaches main_render_skip_threshold_ms. It returns
// after main_loop_shutdown_cleanup once quit is set or a film playback connection is selected.
void main_loop(void)
{
    uint8_t local_time[0x10];                 // SYSTEMTIME [esp+0x78], unused afterwards
    int64_t counter;
    int64_t render_time;
    uint32_t frame_average;                   // [esp+0x24]
    uint16_t fpu_control;                     // [esp+0x20]
    int16_t previous_frames;
    int16_t connection;                       // ebp, read before the input poll
    uint8_t render_frame;                     // [esp+0x17]
    float progress;
    uint32_t previous_queue_time;
    ui_input_event idle_event;
    int32_t idle_remaining;
    int32_t ticks;                            // esi
    float delta;                              // edi
    uint8_t add_bob;
    data_iterator iterator;                   // [esp+0x78] (reuses the SYSTEMTIME slot)
    player *local_player;
    player_update_history *update_history;
    object_header *unit_header;
    uint8_t *unit;
    float leftover_time;                      // [esp+0x1c]
    float frame_delta;                        // [esp+0x18]
    uint64_t present_counter;
    int32_t elapsed_ms;
    int32_t i;

    GetLocalTime((LPSYSTEMTIME)local_time);
    strncpy(main_globals_data.scenario_path, "levels\\b30\\b30", 0xff);
    main_globals_data.scenario_path[0xff] = 0;
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

    sprintf(network_banlist_full_path, "%s\\%s", profile_directory, "banned.txt");
    ban_list.element_size = 0x38;
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
            // -checkfpu: reinitialise the x87 unit every frame with the invalid operation
            // exception unmasked and 24 bit precision (control word 0x7e).
            fpu_control = 0x7e;
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
            ui_pause_pending_count_00718fa0 = 0x1e;
            main_globals_data.revert_map = 0;
        }
        if (main_globals_data.revert_map_if_allowed != 0) {
            if (game_state_write_in_progress == 0 && cinematic_globals_ptr->skip_in_progress != 0) {
                game_state_perform_revert();
                ui_pause_pending_count_00718fa0 = 0x1e;
                main_globals_data.revert_map = 0;
            }
            main_globals_data.revert_map_if_allowed = 0;
        }
        if (main_globals_data.reset_map != 0 && game_time->paused == 0) {
            scenario_structure_bsp_switch(0);
            game_stop_current_map();
            input_reset_state_and_axis_configs();
            memset(&input_globals.states[0], 0, sizeof(input_globals.states[0])); // 0x00712498..0x007124bf
            input_globals.system_key_states[0] = 0;   // WORD store at 0x007127d0
            input_globals.system_key_states[1] = 0;
            input_globals.idle = 1;
            input_globals.system_key_states[2] = 0;   // 0x007127d2
            game_start_new_map();
            main_ensure_local_players();
            game_engine_init_tick_record_for_mode();
            game_engine_reset_all_players();
            ui_pause_pending_count_00718fa0 = 0x1e;
            main_globals_data.reset_map = 0;
        }
        if (main_globals_data.save_core != 0) {
            if (game_state_write_profile_file(0x440000, "core.bin", game_state_base) != 0) {
                console_print_error_va(0, "saved '%s'", "core.bin");
            } else {
                console_print_error_va(0, "error writing '%s'", "core.bin");
            }
            main_globals_data.save_core = 0;
        }
        if (main_globals_data.load_core != 0) {
            game_state_load_core("core.bin");
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
             strcmp(ui_root_widget[0]->name, "the_main_menu") == 0)) {
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
            // no game is running: pregame view only
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

            // 0x4c7f10 (Ghidra weapon_stop_reload) starts here
            if (main_globals_data.game_connection == _game_connection_network_client &&
                player_update_log_flags != 0) {
                iterator.data = player_data;
                iterator.next_index = 0;          // WORD store
                iterator.index = (datum_index)-1;
                iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
                while ((local_player = (player *)data_iterator_next(&iterator)) != 0) {
                    if (local_player->local_player_index == -1) {
                        continue;
                    }
                    update_history = (player_update_history *)network_client->update_history;
                    if (local_player->unit != (datum_index)-1 && update_history != 0 &&
                        update_history->tail != 0) {
                        unit_header = (object_header *)object_data->data + (local_player->unit & 0xffff);
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
            if (main_render_skip_threshold_ms <= 0x14) {
                main_render_skip_threshold_ms = 0x14;
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
            // dropped frame: count it as presented and sample the statistics as dropped
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
                frame_delta = 1.0f / 30.0f;       // 0x00672acc
            }
            if (main_globals_data.game_connection == _game_connection_local) {
                if (frame_delta > 1.0f / 30.0f) {
                    frame_delta = 1.0f / 30.0f;   // 0x3d088889
                }
            } else if (frame_delta > 1.0f) {      // 0x00672ac4
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
                0xff /* QS_ALLINPUT */);
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

#if 0
Original Ghidra decompilation (0x4c7610, Ghidra name game_state_save_core). The Ghidra 0x4c7f10
"weapon_stop_reload" is the same body entered at the return address of the call to
game_engine_advance_simulation_ticks (0x4c7f0b call, 0x4c7f10 add esp,4); it has no callers and
its decompile is the tail of the listing below (from `if ((DAT_00719720 == 1) && ...` on), so
it is not repeated.

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl game_state_save_core(void)

{
  int iVar1;
  float delta_time;
  uint uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  char cVar5;
  short sVar6;
  uint uVar7;
  int iVar8;
  uint *puVar9;
  undefined **ppuVar10;
  char *pcVar11;
  char *pcVar12;
  bool bVar13;
  undefined8 uVar14;
  DWORD dwMilliseconds;
  undefined4 uStack_74;
  LARGE_INTEGER local_68;
  LARGE_INTEGER LStack_60;
  LARGE_INTEGER LStack_58;
  LARGE_INTEGER LStack_50;
  LARGE_INTEGER LStack_48;
  LARGE_INTEGER LStack_40;
  LARGE_INTEGER LStack_38;
  int iStack_30;
  int iStack_2c;
  LARGE_INTEGER LStack_28;
  LARGE_INTEGER LStack_20;
  _SYSTEMTIME local_18;

  GetLocalTime(&local_18);
  _strncpy(&DAT_00719779,"levels\\b30\\b30",0xff);
  DAT_00719878 = 0;
  DAT_00719754._3_1_ = '\x01';
  DAT_00719754._0_2_ = -1;
  DAT_00719769 = 1;
  QueryPerformanceCounter(&local_68);
  uVar14 = __allmul(CONCAT22(local_68.s.LowPart._2_2_,local_68.s.LowPart._0_2_),
                    CONCAT22(local_68.s.HighPart._2_2_,local_68.s.HighPart._0_2_),1000,0);
  DAT_00719764 = __alldiv(uVar14,DAT_006ac8f8,DAT_006ac8fc);
  FUN_004c62d0();
  FUN_004d7980();
  FUN_004aa6b0();
  particle_systems_initialize();
  ppuVar10 = &PTR_s_beavercreek_0068e58c;
  do {
    chimera__load_multiplayer_maps();
    ppuVar10 = ppuVar10 + 3;
  } while ((int)ppuVar10 < 0x68e670);
  _sprintf(&DAT_0071c308,"%s\\%s",&DAT_006ac900);
  _DAT_006b859c = 0x38;
  DAT_006b85a0 = 0;
  DAT_006b85a4 = 0;
  network_banlist_load();
  _DAT_006b85a8 = 8;
  DAT_006b85ac = 0;
  DAT_006b85b0 = 0;
  chimera__exec_init();
  game_start_new_single_player_map();
  game_timer_reset();
  FUN_004c9c80();
  DAT_007252b6 = (undefined1)DAT_007196e4;
  if ((((DAT_007196d8 == 0) && (DAT_007196e8 == 0)) && (DAT_007196f4 == 0)) && (DAT_0071d1a8 == 0))
  {
    movie_play_bink();
    movie_play_bink();
    movie_play_bink();
  }
  DAT_00696570 = 0;
  do {
    uVar7 = game_frame_rate_average_update();
    if ((short)DAT_00719754 != -1) {
      FUN_004c9b60();
    }
    if (((DAT_0071974f != '\0') && (DAT_006f1d6c[2] == '\0')) &&
       (bVar13 = 0x5a < DAT_00719770, DAT_00719770 = DAT_00719770 + 1, bVar13)) {
      DAT_0071974f = '\0';
      DAT_00719770 = 0;
      game_state_perform_revert();
    }
    if (DAT_0071974e != '\0') {
      campaign_level_advance();
    }
    if (((DAT_00719750 != '\0') && (DAT_006f1d6c[2] == '\0')) &&
       ((*(char *)(DAT_006f187c + 9) == '\0' &&
        ((sVar6 = DAT_00719772 + 1, bVar13 = 0x5a < DAT_00719772, DAT_00719772 = sVar6, bVar13 &&
         (cVar5 = FUN_00473e90(), cVar5 != '\0')))))) {
      DAT_00719750 = '\0';
      DAT_00719772 = 0;
    }
    if (DAT_0071973f != '\0') {
      (*(code *)PTR_FUN_0069e7ac)();
      DAT_00719769 = 0;
      DAT_0071976a = 0;
      iVar8 = game_state_queue_write('\x01');
      DAT_006e2dd9 = (char)iVar8 != '\0';
      DAT_0071976a = '\x01';
      FUN_004aa310();
      DAT_0071973f = '\0';
    }
    if (DAT_00719739 != '\0') {
      main_level_transition_update();
    }
    if (DAT_0071973a != '\0') {
      game_state_perform_revert();
      DAT_00718fa0 = 0x1e;
      DAT_0071973a = '\0';
    }
    if (DAT_0071973b != '\0') {
      if ((DAT_006e3000 == '\0') && (*(char *)(DAT_006f187c + 10) != '\0')) {
        game_state_perform_revert();
        DAT_00718fa0 = 0x1e;
        DAT_0071973a = '\0';
      }
      DAT_0071973b = '\0';
    }
    if ((DAT_00719738 != '\0') && (DAT_006f1d6c[2] == '\0')) {
      scenario_structure_bsp_switch();
      FUN_0045b370();
      input_reset_state_and_axis_configs();
      DAT_00712498 = 0;
      DAT_0071249c = 0;
      _DAT_007124a0 = 0;
      _DAT_007124a4 = 0;
      _DAT_007124a8 = 0;
      _DAT_007124ac = 0;
      _DAT_007124b0 = 0;
      _DAT_007124b4 = 0;
      _DAT_007127d0 = 0;
      _DAT_007124b8 = 0;
      DAT_00712540 = '\x01';
      DAT_007127d2 = 0;
      _DAT_007124bc = 0;
      FUN_0045b050();
      FUN_004c8800();
      FUN_00470ae0();
      FUN_0045b8b0();
      DAT_00718fa0 = 0x1e;
      DAT_00719738 = '\0';
    }
    if (DAT_00719751 != '\0') {
      cVar5 = game_state_write_profile_file("core.bin");
      if (cVar5 == '\0') {
        uVar14 = 0x66b2f8006709ac;
      }
      else {
        uVar14 = 0x66b2f8006709c0;
      }
      console_print_error_va(uVar14);
      DAT_00719751 = '\0';
    }
    if (DAT_00719752 != '\0') {
      game_state_load_core();
      DAT_00719752 = '\0';
    }
    if (DAT_00719754._3_1_ != '\0') {
      main_menu_return_and_reset();
    }
    if (DAT_00719758 != '\0') {
      DAT_00719758 = '\0';
    }
    if (DAT_0071976c != '\0') {
      game_engine_flush_pending_simulation_ticks();
    }
    if (DAT_00719774 != '\0') {
      if (DAT_006ac470 != '\0') {
        sVar6 = cache_file_download_status_get();
        if (sVar6 == 1) {
          cache_file_download_finish();
        }
        if (DAT_006ac470 != '\0') goto LAB_004c7a4e;
      }
      cache_file_open_by_name();
      DAT_00719774 = '\0';
    }
LAB_004c7a4e:
    if (DAT_00719a79 != '\0') {
      network_game_client_connect_to_resolved_address();
    }
    sVar6 = DAT_00719720;
    input_directinput_poll_devices();
    if (DAT_007196d8 == 0) {
      input_update_tick();
    }
    shell_pump_windows_messages();
    uVar2 = DAT_00712cc8;
    if (DAT_0071975b != '\0') {
LAB_004c8327:
      main_loop_shutdown_cleanup();
      return;
    }
    if (DAT_00712cc0 != '\0') {
      QueryPerformanceCounter(&LStack_60);
      uVar14 = __allmul(LStack_60.s.LowPart,LStack_60.s.HighPart,1000,0);
      DAT_00712cc8 = __alldiv(uVar14,DAT_006ac8f8,DAT_006ac8fc);
      if ((DAT_00712cc4 < uVar2) && (DAT_00712cc0 != '\0')) {
        local_68.s.LowPart._2_2_ = 0;
        local_68.s.HighPart._0_2_ = 0;
        local_68.s.LowPart._0_2_ = 0;
        local_68.s.HighPart._2_2_ = 0;
        FUN_00492340();
      }
    }
    if ((sVar6 == 2) && (FUN_00577940(), DAT_0069fdfc != -1)) {
      FUN_0061b7e0();
    }
    network_update();
    if ((sVar6 == 1) || (sVar6 == 2)) {
LAB_004c7b44:
      if (DAT_00719ce8 != DAT_006894b0) {
        DAT_00719ce8 = DAT_006894b0;
        FUN_004d8080();
      }
      FUN_004d84d0();
      network_bandwidth_rate_compute();
    }
    else if ((DAT_00718fc9 == '\x01') && (DAT_00718f94 != 0)) {
      iVar8 = 0xe;
      bVar13 = true;
      pcVar11 = *(char **)(DAT_00718f94 + 4);
      pcVar12 = "the_main_menu";
      do {
        if (iVar8 == 0) break;
        iVar8 = iVar8 + -1;
        bVar13 = *pcVar11 == *pcVar12;
        pcVar11 = pcVar11 + 1;
        pcVar12 = pcVar12 + 1;
      } while (bVar13);
      if (bVar13) goto LAB_004c7b44;
    }
    if (((DAT_00721e8c != '\0') && (sVar6 != 1)) && (sVar6 != 2)) goto LAB_004c822b;
    bVar13 = true;
    if (sVar6 == 1) {
      cVar5 = FUN_004dded0();
      if (cVar5 == '\0') {
        if (*(short *)(DAT_0071c2d8 + 0xedc) == 8) {
          if (DAT_00718fa4 == -1) {
            DAT_00718fa4 = 4;
            DAT_0071c2de = 1;
            chat_close();
            goto LAB_004c7c44;
          }
        }
        else if (DAT_00718fa4 == -1) {
          DAT_00718fa4 = 6;
        }
        DAT_0071c2de = 1;
        chat_close();
      }
    }
    else if (sVar6 == 2) {
      if ((((*(byte *)(DAT_0071c2d4 + 6) >> 2 & 1) == 0) &&
          (cVar5 = FUN_004dded0(), cVar5 != '\x01')) || (cVar5 = FUN_004ddd90(), cVar5 != '\x01')) {
        if (DAT_00718fa4 == -1) {
          DAT_00718fa4 = 1;
        }
        DAT_0071c2de = 1;
        chat_close();
      }
    }
    else if (sVar6 == 3) goto LAB_004c8327;
LAB_004c7c44:
    main_loop_frame_pacer();
    FUN_004972c0();
    interface_tick();
    if ((DAT_00712540 == '\0') || (DAT_006b7020 != '\0')) {
      QueryPerformanceCounter(&LStack_40);
      uVar14 = __allmul(LStack_40.s.LowPart,LStack_40.s.HighPart,1000,0);
      DAT_00719764 = __alldiv(uVar14,DAT_006ac8f8,DAT_006ac8fc);
    }
    else if (((*DAT_006f1d6c == '\0') ||
             (((DAT_006f1d6c[1] == '\0' && (DAT_006f1d6c[2] == '\0')) || (DAT_006f1d6c[2] != '\0')))
             ) || (*(char *)(DAT_006f187c + 9) == '\0')) {
      if (0 < DAT_0071975c) {
        QueryPerformanceCounter(&LStack_58);
        uVar14 = __allmul(LStack_58.s.LowPart,LStack_58.s.HighPart,1000,0);
        iVar8 = __alldiv(uVar14,DAT_006ac8f8,DAT_006ac8fc);
        iVar8 = (DAT_0071975c - iVar8) + DAT_00719764;
        QueryPerformanceCounter(&LStack_50);
        if (iVar8 < 1) {
          uVar14 = __allmul(LStack_50.s.LowPart,LStack_50.s.HighPart,1000,0);
          iVar8 = __alldiv(uVar14,DAT_006ac8f8,DAT_006ac8fc);
          if (DAT_00719760 - iVar8 == -10000 || (DAT_00719760 - iVar8) + 10000 < 0) {
            if (DAT_00718fc9 == '\0') {
              DAT_00719754._0_2_ = -1;
              DAT_0071973c = '\0';
              DAT_00719754._3_1_ = '\x01';
              DAT_00719764 = FUN_00449210();
            }
            else {
              DAT_00719754._3_1_ = '\0';
              DAT_00719759 = 1;
              DAT_00719739 = '\x01';
              DAT_00719764 = FUN_00449210();
            }
          }
        }
      }
    }
    else {
      QueryPerformanceCounter(&LStack_48);
      uVar14 = __allmul(LStack_48.s.LowPart,LStack_48.s.HighPart,1000,0);
      DAT_00719760 = __alldiv(uVar14,DAT_006ac8f8,DAT_006ac8fc);
    }
    if ((*DAT_006f1d6c == '\0') || ((DAT_006f1d6c[1] == '\0' && (DAT_006f1d6c[2] == '\0')))) {
      if ((DAT_007196d8 == 0) && (DAT_00721e8c == '\0')) {
        FUN_004c8f20();
      }
LAB_004c81ee:
      if (DAT_00719aa8 == '\0') {
        movie_capture_frame_export();
      }
    }
    else {
      if (DAT_006b2efc != '\0') {
        console_process_input_events();
        console_process_queued_input();
        if (DAT_006b7020 == '\0') {
          chimera__console_fade_fn();
        }
        FUN_00496d40();
      }
      cVar5 = FUN_004c65c0();
      if ((cVar5 == '\0') || (DAT_00719720 != 0)) {
        delta_time = (float)DAT_00719769 * _DAT_0071971c;
        puVar9 = (uint *)FUN_00470b30(delta_time);
        DAT_006f7ea4 = 0;
        DAT_006f7ea8 = 0;
        DAT_006f7eac = 0;
        _DAT_006f7eb0 = 0;
        _DAT_006f7eb4 = 0;
        _DAT_006f7eb8 = 0;
        _DAT_006f7ebc = 0;
        DAT_006f7ecc = 0;
        _DAT_006f7ec0 = 0;
        _DAT_006f7ec4 = puVar9;
        FUN_00471ae0();
        if ((DAT_00719720 == 1) ||
           ((DAT_00719720 == 2 && ((*(byte *)(DAT_0071c2d4 + 6) >> 2 & 1) == 0)))) {
          FUN_004aaa90();
          cVar5 = update_server_send_update(puVar9,DAT_00719718);
          if (cVar5 == '\0') {
            if (DAT_00718fa4 == -1) {
              DAT_00718fa4 = 1;
            }
            DAT_0071c2de = 1;
            chat_close();
          }
        }
        game_engine_advance_simulation_ticks(delta_time);
        if ((DAT_00719720 == 1) && (_DAT_00710310 != 0)) {
          local_18.wYear = (undefined2)DAT_0087a480;
          local_18.wMonth = DAT_0087a480._2_2_;
          local_18._12_4_ = DAT_0087a480 ^ 0x69746572;
          local_18.wDayOfWeek = 0;
          local_18.wHour = 0xffff;
          local_18.wMinute = 0xffff;
          iVar8 = data_iterator_next();
          while (iVar8 != 0) {
            if (*(short *)(iVar8 + 2) != -1) {
              if (((*(uint *)(iVar8 + 0x34) != 0xffffffff) &&
                  (iVar1 = *(int *)(DAT_0071c2d8 + 0xf48), iVar1 != 0)) &&
                 (*(int *)(iVar1 + 8) != 0)) {
                iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                (*(uint *)(iVar8 + 0x34) & 0xffff) * 0xc);
                player_update_history_log_write
                          ("[%d]: Update [%d] ([%d]): ([%f] [%f] [%f]), ([%f] [%f]), ([%f] [%f])\n",
                           *(undefined4 *)(DAT_006f1d6c + 0xc),**(undefined4 **)(iVar1 + 8),
                           (*(undefined4 **)(iVar1 + 8))[1],(double)*(float *)(iVar8 + 0x5c),
                           (double)*(float *)(iVar8 + 0x60),(double)*(float *)(iVar8 + 100),
                           (double)*(float *)(iVar8 + 0x278),(double)*(float *)(iVar8 + 0x27c),
                           (double)*(float *)(iVar8 + 0x68),(double)*(float *)(iVar8 + 0x6c));
              }
              break;
            }
            iVar8 = data_iterator_next();
          }
        }
        bVar13 = false;
        if ((DAT_00721e8c == '\0') && ((DAT_00719754._2_1_ != '\0' || (DAT_00719769 != 0)))) {
          bVar13 = true;
        }
        camera_update();
        FUN_004455f0();
        camera_shake_tick((float)DAT_00719769 * _DAT_0071971c);
        game_engine_update_end_game_sequence();
      }
      if (DAT_0071973c != '\0') {
        main_save_map_private();
      }
      if (DAT_006894bc != 0xffffffff) {
        if ((int)DAT_006894bc < 0x15) {
          DAT_006894bc = 0x14;
        }
        if (DAT_006894bc <= uVar7) {
          bVar13 = false;
        }
      }
      if (DAT_007196d8 == 0) {
        if (bVar13) goto LAB_004c80f5;
        bVar13 = 0xfffffffe < DAT_0069c648;
        DAT_0069c648 = DAT_0069c648 + 1;
        DAT_0069c64c = DAT_0069c64c + (uint)bVar13;
        rasterizer_frame_statistics_sample();
      }
      else if (*(int *)(DAT_006f1d6c + 0xc) != DAT_00719b60) {
        DAT_00719b60 = *(int *)(DAT_006f1d6c + 0xc);
        timedemo_benchmark_update();
LAB_004c80f5:
        if (DAT_00719aa8 == '\0') {
          QueryPerformanceCounter(&LStack_38);
          uVar4 = LStack_38.s.HighPart;
          uVar3 = LStack_38.s.LowPart;
          if ((DAT_006f1d6c[2] == '\0') && (DAT_006b7020 == '\0')) {
            uStack_74 = *(undefined4 *)(DAT_006f1d6c + 0x1c);
            iStack_30 = LStack_38.s.LowPart - DAT_00719710;
            iStack_2c = (LStack_38.s.HighPart - DAT_00719714) -
                        (uint)(LStack_38.s.LowPart < DAT_00719710);
          }
          else {
            uStack_74 = 0;
          }
          render_frame_all_views(uStack_74);
          DAT_00719710 = uVar3;
          DAT_00719714 = uVar4;
          goto LAB_004c81ee;
        }
      }
    }
LAB_004c822b:
    if (DAT_0071975b != '\0') goto LAB_004c8327;
    if (DAT_0071976a != '\0') {
      DAT_0071976a = '\0';
      QueryPerformanceCounter(&LStack_28);
      DAT_00719700 = LStack_28.s.LowPart;
      DAT_00719704 = LStack_28.s.HighPart;
      DAT_00719710 = LStack_28.s.LowPart;
      DAT_00719714 = LStack_28.s.HighPart;
      uVar14 = __allmul(LStack_28.s.LowPart,LStack_28.s.HighPart,1000,0);
      DAT_00719708 = __alldiv(uVar14,DAT_006ac8f8,DAT_006ac8fc);
      DAT_00719769 = 1;
    }
    if (DAT_00721e8c != '\0') {
      if ((DAT_00719720 < 1) || (2 < DAT_00719720)) {
        dwMilliseconds = 100;
      }
      else {
        dwMilliseconds = 0x14;
      }
      MsgWaitForMultipleObjects(0,(HANDLE *)0x0,0,dwMilliseconds,0xff);
    }
    QueryPerformanceCounter(&LStack_20);
    uVar14 = __allmul(LStack_20.s.LowPart,LStack_20.s.HighPart,1000,0);
    iVar8 = __alldiv(uVar14,DAT_006ac8f8,DAT_006ac8fc);
    DAT_00719ab8 = iVar8 - _DAT_00719ab0;
    if (200 < DAT_00719ab8) {
      DAT_00719ab8 = 200;
    }
  } while( true );
}

Disassembly facts Ghidra dropped or got wrong (0x4c7610..0x4c8333):
  4c778b  cmp [0x71d1a4],0 ; je ; mov [esp+0x20],0x7e ; finit ; fldcw [esp+0x20]  (-checkfpu, every frame)
  4c786d  xor dl,dl before call 0x4aa310                       hud_display_checkpoint_message(DL = 0)
  4c7905  xor esi,esi before call 0x53eeb0                     bsp index 0 in SI
  4c7990  push [0x6e2dc8] ; push "core.bin" ; mov edi,0x440000 ; call 0x5393d0
  4c79d6  mov eax,"core.bin" ; call 0x538390
  4c7a3a  push 0 ; mov eax,0x719979 ; call 0x443360
  4c7add  8 zero bytes at [esp+0x28] ; xor eax,eax ; lea edi,[esp+0x28] ; call 0x492340
  4c7c92..4c7d75  the idle test is only reached when the game is not in active gameplay
  4c8044  camera_update(delta) ; 4c806b observer_update(delta, camera_is_local...()) ;
          4c808b game_engine_update_end_game_sequence(delta), each delta recomputed from
          time_is_running * frame_delta_time
  4c80b9  cmp [esp+0x24],eax ; jb   unsigned average against the skip threshold
  4c814d..4c81a7  the render delta is (now - render_counter) / frequency, 1/30 under
          -timedemo, then clamped to 1/30 in a local game or to 1.0 otherwise; zero while
          paused or while the console is open
  4c81fd  64 bit present counter + 1 ; mov ebx,0x7c30a0 ; push 1 ; call 0x512530
  4c8308  cmp eax,0xc8 ; jbe  unsigned clamp of the newest frame time sample
#endif
