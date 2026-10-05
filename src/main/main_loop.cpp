/**
 * The main loop, its frame pacer and shutdown, plus per-frame game timing helpers.
 */

#include "halo/math/constants.hpp"
#include "tags.h"
#include "halo/shell/standalone.hpp"
#include <stdlib.h>
#include "halo/scenario/api.hpp"
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
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/input/api.hpp"
#include "halo/cutscene/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/main/layout.hpp"
#include "halo/render/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/main/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/devices.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/scenario/scenario.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/units/records.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/hs/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/main/vars.hpp"
#include "halo/networking/vars.hpp"
#include "halo/saved_games/vars.hpp"
#include "halo/shell/vars.hpp"
#include "../gamespy/gamespy_calls.hpp"
#include "halo/units/api.hpp"
#include "halo/platform/time.hpp"
#include "halo/platform/memory.hpp"
#include "halo/platform/system.hpp"
#include "halo/platform/window.hpp"


static auto &main_globals_data = halo::link::ref<main_globals>(halo::main::vars().main_globals_data);
namespace halo::main {

/**
 * While skip_tick_count is armed and a cinematic isn't suppressing it, runs that many simulation
 * ticks back-to-back at a fixed 1/30s timestep (forcing halo::game::globals().game_time->speed to 1.0 for the
 * duration, restoring it afterward -- already 1.0 in any networked game), then clears both the
 * tick count and the pending flag.
 *
 * @address 0x4c99e0
 */
void MainLoop::engine_flush_pending_simulation_ticks(void)
{
    if (main_globals_data.skip_tick_count != 0 && halo::cutscene::globals().cinematic_globals->in_progress != 0) {
        float saved_speed = (main_globals_data.game_connection == 1 || main_globals_data.game_connection == 2)
                                 ? 1.0f
                                 : halo::game::globals().game_time->speed;

        halo::game::globals().game_time->speed = 1.0f;
        while (main_globals_data.skip_tick_count > 0) {
            main_globals_data.skip_tick_count = main_globals_data.skip_tick_count - 1;
            halo::game::game_engine_advance_simulation_ticks(halo::math::k_seconds_per_tick);
        }
        halo::game::globals().game_time->speed = saved_speed;
    }
    main_globals_data.skip_tick_count = 0;
    main_globals_data.skip_ticks = 0;
}

}

static auto &frame_rate_average_data = halo::link::ref<main_frame_rate_average>(halo::main::vars().frame_rate_average_data);
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

    halo::platform::read_performance_counter(&counter);
    frame_rate_average_data.sample_time_ms = (int32_t)((counter * 1000) / halo::cseries::globals().performance_frequency);

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

    halo::platform::read_performance_counter(&counter);
    main_globals_data.frame_counter_low = (uint32_t)counter;
    main_globals_data.frame_counter_high = (uint32_t)(counter >> 32);
    main_globals_data.render_counter_low = (uint32_t)counter;
    main_globals_data.render_counter_high = (uint32_t)(counter >> 32);
    main_globals_data.frame_time_ms = (uint32_t)((counter * 1000) / halo::cseries::globals().performance_frequency);
}

}

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

        for (i = 0; i < halo::game::globals().local_player_count; i = i + 1) {
            slot = halo::game::local_player_find_free_slot_index();
            new_player = halo::game::player_new_network(k_datum_index_none, 0, (int16_t)slot, 0);
            if (-1 < slot && slot < 1) {
                old_player = halo::game::globals().local_player_globals->local_players[slot];
                if (old_player != k_datum_index_none) {
                    player *old_p = (player *)((uint8_t *)halo::game::globals().player_data->data +
                                                datum_slot(old_player) * sizeof(player));
                    old_p->local_player_index = -1;
                }
                halo::game::globals().local_player_globals->local_players[slot] = new_player;
                if (new_player != k_datum_index_none) {
                    player *new_p = (player *)((uint8_t *)halo::game::globals().player_data->data +
                                                datum_slot(new_player) * sizeof(player));
                    new_p->local_player_index = (int16_t)slot;
                }
            }
        }
    } else {
        datum_index new_player;
        datum_index old_player;

        new_player = halo::game::player_new_network(k_datum_index_none, 0, 0, 0);
        old_player = halo::game::globals().local_player_globals->local_players[0];
        if (old_player != k_datum_index_none) {
            player *old_p = (player *)((uint8_t *)halo::game::globals().player_data->data +
                                        datum_slot(old_player) * sizeof(player));
            old_p->local_player_index = -1;
        }
        halo::game::globals().local_player_globals->local_players[0] = new_player;
        if (new_player != k_datum_index_none) {
            player *new_p = (player *)((uint8_t *)halo::game::globals().player_data->data +
                                        datum_slot(new_player) * sizeof(player));
            new_p->local_player_index = 0;
        }
    }
}

}

static auto &timedemo_globals_data = halo::link::ref<timedemo_globals>(halo::main::vars().timedemo_globals_data);
static auto &multiplayer_maps = halo::link::ref<multiplayer_map_table_entry [k_main_multiplayer_map_count]>(halo::main::vars().multiplayer_maps);
static auto &console_globals_data = halo::link::ref<console_globals>(halo::main::vars().console_globals_data);
static auto &main_unknown_696570 = halo::link::ref<uint8_t>(halo::main::vars().main_unknown_696570);
static auto &input_globals = halo::link::ref<input_abstraction_globals>(halo::main::vars().input_globals);
static auto &input_event_queue_active = halo::link::ref<input_event_queue>(halo::ui::vars().input_event_queue_active);
static auto &network_banlist_full_path = halo::link::ref<char [0x104]>(halo::networking::vars().network_banlist_full_path);
static auto &profile_directory = halo::link::ref<char [0x105]>(halo::saved_games::vars().profile_directory);
static auto &ban_list = halo::link::ref<growable_array>(halo::networking::vars().ban_list);
static auto &network_buffer_pair_pool = halo::link::ref<growable_array>(halo::main::vars().network_buffer_pair_pool);
static auto &novideo_or_connect = halo::link::ref<int32_t>(halo::main::vars().novideo_or_connect);
static auto &safe_mode = halo::link::ref<int32_t>(halo::shell::vars().safe_mode);
static auto &checkfpu = halo::link::ref<int32_t>(halo::main::vars().checkfpu);
static auto &game_state_before_save_proc = halo::link::ref<game_state_proc>(halo::main::vars().game_state_before_save_proc);
static auto &game_state_revert_available = halo::link::ref<uint8_t>(halo::main::vars().game_state_revert_available);
static auto &ui_pause_pending_count_00718fa0 = halo::link::ref<int32_t>(halo::main::vars().ui_pause_pending_count_00718fa0);
static auto &network_console_connection_id = halo::link::ref<int32_t>(halo::networking::vars().network_console_connection_id);
static auto &network_bandwidth_graph_globals = halo::link::ref<network_bandwidth_graph>(halo::main::vars().network_bandwidth_graph_globals);
static auto &network_bandwidth_graph_default_interval_ms = halo::link::ref<uint32_t>(halo::main::vars().network_bandwidth_graph_default_interval_ms);
static auto &ui_split_screen = halo::link::ref<uint8_t>(halo::ui::vars().ui_split_screen);
static auto &ui_root_widget = halo::link::ref<widget_instance *[1]>(halo::ui::vars().ui_root_widget);
static auto &shell_application_inactive = halo::link::ref<uint8_t>(halo::main::vars().shell_application_inactive);
static auto &terminal_initialized = halo::link::ref<uint8_t>(halo::main::vars().terminal_initialized);
static auto &update_client_staged = halo::link::ref<uint32_t [8]>(halo::game::vars().update_client_staged);
static auto &update_client_ticks_remaining = halo::link::ref<int32_t>(halo::game::vars().update_client_ticks_remaining);
static auto &update_client_staged_count = halo::link::ref<int32_t>(halo::game::vars().update_client_staged_count);
static auto &player_update_log_flags = halo::link::ref<uint32_t>(halo::main::vars().player_update_log_flags);
static auto &main_render_skip_threshold_ms = halo::link::ref<int32_t>(halo::main::vars().main_render_skip_threshold_ms);
namespace halo::main {
namespace {

/**
 * Services the network client or server connection for one frame. Returns true when the frame loop must stop (film playback).
 */
bool frame_update_network(int16_t connection)
{
    if (connection == _game_connection_network_client) {
        if (halo::networking::network_client_update_dispatch() == 0) {
            if (halo::networking::globals().client->disconnect_reason == 8) {
                if (halo::networking::globals().join_error_code == -1) {
                    halo::networking::globals().join_error_code = 4;
                }
            } else if (halo::networking::globals().join_error_code == -1) {
                halo::networking::globals().join_error_code = 6;
            }
            halo::networking::globals().host_handoff_requested = 1;
            halo::interface::chat_close();
        }
    } else if (connection == _game_connection_network_server) {
        if (((halo::networking::globals().server->flags & 4) == 0 && (uint8_t)halo::networking::network_client_update_dispatch() != 1) ||
            (uint8_t)halo::networking::network_host_shutdown_or_defer() != 1) {
            if (halo::networking::globals().join_error_code == -1) {
                halo::networking::globals().join_error_code = 1;
            }
            halo::networking::globals().host_handoff_requested = 1;
            halo::interface::chat_close();
        }
    } else if (connection == _game_connection_film_playback) {
        return true;
    }

    return false;
}

/**
 * Refreshes the last activity / gameplay timestamps, or returns to the main menu once the idle timeout has elapsed.
 */
void frame_track_idle_time()
{
    int64_t counter;
    int32_t idle_remaining;

    if (input_globals.idle == 0 || console_globals_data.active != 0) {
        halo::platform::read_performance_counter(&counter);
        main_globals_data.last_activity_time_ms =
            (int32_t)((counter * 1000) / halo::cseries::globals().performance_frequency);
    } else if (halo::game::globals().game_time->initialized != 0 && (halo::game::globals().game_time->active != 0 || halo::game::globals().game_time->paused != 0) &&
               halo::game::globals().game_time->paused == 0 && halo::cutscene::globals().cinematic_globals->in_progress != 0) {
        halo::platform::read_performance_counter(&counter);
        main_globals_data.last_gameplay_time_ms =
            (int32_t)((counter * 1000) / halo::cseries::globals().performance_frequency);
    } else if (main_globals_data.idle_timeout_ms > 0) {
        halo::platform::read_performance_counter(&counter);
        idle_remaining = main_globals_data.idle_timeout_ms -
            (int32_t)((counter * 1000) / halo::cseries::globals().performance_frequency) +
            main_globals_data.last_activity_time_ms;
        halo::platform::read_performance_counter(&counter);
        if (idle_remaining <= 0 &&
            main_globals_data.last_gameplay_time_ms -
                (int32_t)((counter * 1000) / halo::cseries::globals().performance_frequency) +
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
            main_globals_data.last_activity_time_ms = (int32_t)halo::cseries::time_query_performance_counter_ms();
        }
    }
}

/**
 * Logs the first local player's unit position and throttle to the player update history log.
 */
void frame_log_player_update_history()
{
    data_iterator iterator;
    player *local_player;
    player_update_history *update_history;
    object_header *unit_header;
    object *unit;

    if (main_globals_data.game_connection == _game_connection_network_client &&
        player_update_log_flags != 0) {
        iterator.data = halo::game::globals().player_data;
        iterator.next_index = 0;
        iterator.index = k_datum_index_none;
        iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
        while ((local_player = (player *)halo::memory::data_iterator_next(&iterator)) != 0) {
            if (local_player->local_player_index == -1) {
                continue;
            }
            update_history = (player_update_history *)halo::networking::globals().client->update_history;
            if (local_player->unit != k_datum_index_none && update_history != 0 &&
                update_history->tail != 0) {
                unit_header = (object_header *)halo::objects::globals().object_data->data + datum_slot(local_player->unit);
                unit = unit_header->data;
                halo::networking::player_update_history_log_write(0x10, 0,
                    "[%d]: Update [%d] ([%d]): ([%f] [%f] [%f]), ([%f] [%f]), ([%f] [%f])\n",
                    halo::game::globals().game_time->game_time, update_history->tail->update_id,
                    update_history->tail->tick_count,
                    (double)unit->position.x, (double)unit->position.y,
                    (double)unit->position.z, (double)halo::units::unit_data_of(unit)->throttle.i,
                    (double)halo::units::unit_data_of(unit)->throttle.j, (double)unit->velocity.i,
                    (double)unit->velocity.j);
            }
            break;
        }
    }
}

/**
 * Accumulates simulation ticks, sends the client update, advances the simulation and updates the camera and observer.
 */
void frame_simulate(uint8_t &render_frame)
{
    float delta;
    int32_t ticks;
    uint8_t add_bob;

    if (halo::main::console_process_key_events() == 0 || main_globals_data.game_connection != 0) {
        delta = (float)main_globals_data.time_is_running * main_globals_data.frame_delta_time;
        ticks = halo::game::game_engine_accumulate_simulation_ticks(delta, 1);
        memset(update_client_staged, 0, sizeof(update_client_staged));
        update_client_staged_count = 0;
        update_client_ticks_remaining = ticks;
        halo::game::game_engine_update_local_player_control(0, delta, ticks);
        if (main_globals_data.game_connection == _game_connection_network_client ||
            (main_globals_data.game_connection == _game_connection_network_server &&
             (halo::networking::globals().server->flags & 4) == 0)) {
            halo::interface::chat_poll_hotkeys();
            if (halo::networking::update_server_send_update(ticks, main_globals_data.frame_time_overflow) == 0) {
                if (halo::networking::globals().join_error_code == -1) {
                    halo::networking::globals().join_error_code = 1;
                }
                halo::networking::globals().host_handoff_requested = 1;
                halo::interface::chat_close();
            }
        }
        halo::game::game_engine_advance_simulation_ticks(delta);

        frame_log_player_update_history();

        render_frame = 0;
        if (shell_application_inactive == 0 &&
            (main_globals_data.main_menu_scenario_loaded != 0 ||
             main_globals_data.time_is_running != 0)) {
            render_frame = 1;
        }
        halo::camera::camera_update((float)main_globals_data.time_is_running * main_globals_data.frame_delta_time);
        add_bob = halo::camera::camera_is_local_player_default_first_person();
        halo::camera::observer_update((float)main_globals_data.time_is_running * main_globals_data.frame_delta_time,
            add_bob);
        halo::game::game_engine_update_end_game_sequence(
            (float)main_globals_data.time_is_running * main_globals_data.frame_delta_time);
    }
}

/**
 * Applies the render skip threshold, handles timedemo and skipped frames, then renders all views.
 */
void frame_render(uint8_t render_frame, uint32_t frame_average)
{
    int64_t counter;
    int64_t render_time;
    uint64_t present_counter;
    float leftover_time;
    float frame_delta;

    if (main_globals_data.save_map != 0) {
        halo::main::main_save_map_private();
    }
    if (main_render_skip_threshold_ms != -1) {
        if (main_render_skip_threshold_ms <= k_minimum_render_skip_threshold_ms) {
            main_render_skip_threshold_ms = k_minimum_render_skip_threshold_ms;
        }
        if (frame_average >= (uint32_t)main_render_skip_threshold_ms) {
            render_frame = 0;
        }
    }

    if (halo::game::globals().time_force_single_tick != 0) {
        if (halo::game::globals().game_time->game_time == timedemo_globals_data.last_game_time) {
            return;
        }
        timedemo_globals_data.last_game_time = halo::game::globals().game_time->game_time;
        halo::main::timedemo_benchmark_update();
    } else if (render_frame == 0) {
        present_counter = ((uint64_t)(uint32_t)halo::rasterizer::globals().present_counter_high << 32 |
            (uint32_t)halo::rasterizer::globals().present_counter_low) + 1;
        halo::rasterizer::globals().present_counter_low = (int32_t)(uint32_t)present_counter;
        halo::rasterizer::globals().present_counter_high = (int32_t)(uint32_t)(present_counter >> 32);
        halo::render::rasterizer_frame_statistics_sample(&halo::rasterizer::globals().frame_statistics, 1);
        return;
    }

    if (main_globals_data.disable_frame_output != 0) {
        return;
    }
    halo::platform::read_performance_counter(&counter);
    if (halo::game::globals().game_time->paused == 0 && console_globals_data.active == 0) {
        leftover_time = halo::game::globals().game_time->leftover_time;
        render_time = counter - ((int64_t)main_globals_data.render_counter_high << 32 |
            main_globals_data.render_counter_low);
        frame_delta = (float)render_time / (float)halo::cseries::globals().performance_frequency;
        if (halo::game::globals().time_force_single_tick != 0) {
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
    halo::main::render_frame_all_views(leftover_time, frame_delta);
    main_globals_data.render_counter_low = (uint32_t)counter;
    main_globals_data.render_counter_high = (uint32_t)(counter >> 32);
    if (main_globals_data.disable_frame_output == 0) {
        halo::main::movie_capture_frame_export();
    }
}


namespace {

/**
 * Optional heap diagnostic, enabled with the environment variable HALO_HEAPCHECK=1: validates the process heap and logs the
 * first checkpoint at which it is found corrupt, so a heap overrun can be narrowed to the frame phase that caused it.
 */
void heap_checkpoint(const char *phase)
{
    static const bool enabled = getenv("HALO_HEAPCHECK") != nullptr;
    static bool reported = false;
    static uint32_t frame = 0;

    if (!enabled || reported) {
        return;
    }
    if (phase[0] == 'b') {
        frame++;
    }
    if (!halo::platform::heap_validate()) {
        reported = true;
        halo::shell::standalone_log("HEAP CORRUPT detected at %s of main loop frame %u", phase, frame);
    }
}

}  // namespace

/**
 * Runs one iteration of the frame body: network update, pacing, interface tick, idle tracking, simulation and render.
 * Returns true when the main loop must stop.
 */
bool update_and_render_frame(int16_t connection, uint32_t frame_average)
{
    uint8_t render_frame;

    if (shell_application_inactive != 0 && connection != _game_connection_network_client &&
        connection != _game_connection_network_server) {
        return false;
    }

    heap_checkpoint("begin");
    render_frame = 1;
    if (frame_update_network(connection)) {
        return true;
    }
    heap_checkpoint("network update");

    halo::main::main_loop_frame_pacer();
    halo::interface::ui_cursor_update();
    halo::interface::interface_tick();
    heap_checkpoint("interface tick");

    frame_track_idle_time();

    if (halo::game::globals().game_time->initialized == 0 || (halo::game::globals().game_time->active == 0 && halo::game::globals().game_time->paused == 0)) {
        if (halo::game::globals().time_force_single_tick == 0 && shell_application_inactive == 0) {
            halo::main::render_pregame_view_initialize();
        }
        if (main_globals_data.disable_frame_output == 0) {
            halo::main::movie_capture_frame_export();
        }
        return false;
    }

    if (terminal_initialized != 0) {
        halo::interface::console_process_input_events();
        halo::interface::console_process_queued_input();
        if (console_globals_data.active == 0) {
            halo::interface::console_message_expire_old();
        }
        halo::interface::console_update_display();
    }
    if (halo::main::console_process_key_events() == 0 || main_globals_data.game_connection != 0) {
        frame_simulate(render_frame);
    }
    heap_checkpoint("simulate");

    frame_render(render_frame, frame_average);
    heap_checkpoint("render");
    return false;
}

}
}

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
    uint32_t frame_average;
    uint16_t fpu_control;
    int16_t previous_frames;
    int16_t connection;
    float progress;
    uint32_t previous_queue_time;
    ui_input_event idle_event;
    int32_t elapsed_ms;
    int32_t i;

    halo::platform::local_time(reinterpret_cast<system_time *>(local_time));
    strncpy(main_globals_data.scenario_path, k_default_scenario_path, k_main_path_length - 1);
    main_globals_data.scenario_path[k_main_path_length - 1] = 0;
    main_globals_data.return_to_main_menu = 1;
    main_globals_data.switch_structure_bsp_index = -1;
    main_globals_data.time_is_running = 1;
    halo::platform::read_performance_counter(&counter);
    main_globals_data.last_activity_time_ms = (int32_t)((counter * 1000) / halo::cseries::globals().performance_frequency);

    halo::main::console_initialize();
    halo::networking::network_bandwidth_graph_reset();
    halo::interface::ui_chat_window_reset_position();
    halo::game::game_initialize();
    for (i = 0; i < k_main_multiplayer_map_count; i++) {
        halo::interface::map_list_add_entry((char *)(uintptr_t)multiplayer_maps[i].name, multiplayer_maps[i].map_id);
    }

    sprintf(network_banlist_full_path, "%s\\%s", profile_directory, k_ban_list_file_name);
    ban_list.element_size = k_ban_list_element_size;
    ban_list.count = 0;
    ban_list.data = 0;
    halo::networking::network_banlist_load();
    network_buffer_pair_pool.element_size = 8;
    network_buffer_pair_pool.count = 0;
    network_buffer_pair_pool.data = 0;

    halo::main::chimera__exec_init();
    halo::main::game_start_new_single_player_map();
    halo::main::game_timer_reset();
    halo::interface::network_autojoin_from_command_line();
    halo::sound::globals().disabled = (uint8_t)halo::shell::globals().nosound;
    main_unknown_696570 = 0;

    for (;;) {
        frame_average = halo::main::game_frame_rate_average_update();
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
            halo::main::main_switch_structure_bsp_and_notify();
        }
        if (main_globals_data.lost_map != 0 && halo::game::globals().game_time->paused == 0) {
            previous_frames = main_globals_data.lost_map_frames;
            main_globals_data.lost_map_frames = (int16_t)(previous_frames + 1);
            if (previous_frames > k_main_revert_delay_frames) {
                main_globals_data.lost_map = 0;
                main_globals_data.lost_map_frames = 0;
                halo::saved_games::game_state_perform_revert();
            }
        }
        if (main_globals_data.won_map != 0) {
            halo::main::campaign_level_advance();
        }
        if (main_globals_data.respawn_coop_players != 0 && halo::game::globals().game_time->paused == 0 &&
            halo::cutscene::globals().cinematic_globals->in_progress == 0) {
            previous_frames = main_globals_data.respawn_coop_frames;
            main_globals_data.respawn_coop_frames = (int16_t)(previous_frames + 1);
            if (previous_frames > k_main_respawn_delay_frames &&
                halo::game::game_engine_attach_players_to_new_bsp() != 0) {
                main_globals_data.respawn_coop_players = 0;
                main_globals_data.respawn_coop_frames = 0;
            }
        }
        if (main_globals_data.save_map_write_pending != 0) {
            game_state_before_save_proc();
            main_globals_data.time_is_running = 0;
            main_globals_data.reset_frame_timers = 0;
            game_state_revert_available = halo::saved_games::game_state_queue_write(1) != 0;
            main_globals_data.reset_frame_timers = 1;
            halo::interface::hud_display_checkpoint_message(0);
            main_globals_data.save_map_write_pending = 0;
        }
        if (main_globals_data.level_transition != 0) {
            halo::main::main_level_transition_update();
        }
        if (main_globals_data.revert_map != 0) {
            halo::saved_games::game_state_perform_revert();
            ui_pause_pending_count_00718fa0 = k_ui_pause_pending_ticks;
            main_globals_data.revert_map = 0;
        }
        if (main_globals_data.revert_map_if_allowed != 0) {
            if (halo::saved_games::globals().game_state_write_in_progress == 0 && halo::cutscene::globals().cinematic_globals->skip_in_progress != 0) {
                halo::saved_games::game_state_perform_revert();
                ui_pause_pending_count_00718fa0 = k_ui_pause_pending_ticks;
                main_globals_data.revert_map = 0;
            }
            main_globals_data.revert_map_if_allowed = 0;
        }
        if (main_globals_data.reset_map != 0 && halo::game::globals().game_time->paused == 0) {
            halo::scenario::structure_bsp_switcher::switch_to(0);
            halo::game::game_stop_current_map();
            halo::input::GameActions::reset_state_and_axis_configs();
            memset(&input_globals.states[0], 0, sizeof(input_globals.states[0]));
            input_globals.system_key_states[0] = 0;
            input_globals.system_key_states[1] = 0;
            input_globals.idle = 1;
            input_globals.system_key_states[2] = 0;
            halo::game::game_start_new_map();
            halo::main::main_ensure_local_players();
            halo::game::game_engine_init_tick_record_for_mode();
            halo::game::game_engine_reset_all_players();
            ui_pause_pending_count_00718fa0 = k_ui_pause_pending_ticks;
            main_globals_data.reset_map = 0;
        }
        if (main_globals_data.save_core != 0) {
            if (halo::saved_games::game_state_write_profile_file(k_game_state_size, (char *)k_core_dump_file_name, halo::saved_games::globals().game_state_base) != 0) {
                halo::main::console_print_error_va(0, "saved '%s'", k_core_dump_file_name);
            } else {
                halo::main::console_print_error_va(0, "error writing '%s'", k_core_dump_file_name);
            }
            main_globals_data.save_core = 0;
        }
        if (main_globals_data.load_core != 0) {
            halo::saved_games::game_state_load_core((char *)k_core_dump_file_name);
            main_globals_data.load_core = 0;
        }
        if (main_globals_data.return_to_main_menu != 0) {
            halo::main::main_menu_return_and_reset();
        }
        if (main_globals_data.unknown_058 != 0) {
            main_globals_data.unknown_058 = 0;
        }
        if (main_globals_data.skip_ticks != 0) {
            halo::main::game_engine_flush_pending_simulation_ticks();
        }
        if (main_globals_data.cache_file_open_pending != 0) {
            if (halo::cache::globals().map_download_in_progress != 0) {
                if (halo::cache::cache_file_download_status_get(&progress, 0) == 1) {
                    halo::cache::cache_file_download_finish();
                }
            }
            if (halo::cache::globals().map_download_in_progress == 0) {
                halo::cache::cache_file_open_by_name(main_globals_data.pending_cache_file_name, 0);
                main_globals_data.cache_file_open_pending = 0;
            }
        }
        if (main_globals_data.connect_pending != 0) {
            halo::main::network_game_client_connect_to_resolved_address();
        }

        connection = main_globals_data.game_connection;
        halo::input::InputDevices::poll();
        if (halo::game::globals().time_force_single_tick == 0) {
            halo::input::InputSystem::update_tick();
        }
        halo::shell::shell_pump_windows_messages();
        if (main_globals_data.quit != 0) {
            break;
        }

        if (input_event_queue_active.enabled != 0) {
            previous_queue_time = input_event_queue_active.start_time;
            halo::platform::read_performance_counter(&counter);
            input_event_queue_active.start_time = (uint32_t)((counter * 1000) / halo::cseries::globals().performance_frequency);
            if (input_event_queue_active.last_event_time < previous_queue_time && input_event_queue_active.enabled != 0) {
                memset(&idle_event, 0, sizeof(idle_event));
                halo::input::UiEvents::queue_push_event(0, &idle_event);
            }
        }
        if (connection == _game_connection_network_server) {
            halo::networking::network_session_host_update();
            if (network_console_connection_id != -1) {
                gcd_think();
            }
        }
        halo::networking::network_update();
        if (connection == _game_connection_network_client ||
            connection == _game_connection_network_server ||
            (ui_split_screen == 1 && ui_root_widget[0] != 0 &&
             strcmp(ui_root_widget[0]->name, k_main_menu_widget_name) == 0)) {
            if (network_bandwidth_graph_globals.sample_interval_ms !=
                network_bandwidth_graph_default_interval_ms) {
                network_bandwidth_graph_globals.sample_interval_ms =
                    network_bandwidth_graph_default_interval_ms;
                halo::networking::network_bandwidth_graph_instance_history_reset(&network_bandwidth_graph_globals);
            }
            halo::networking::network_bandwidth_graph_tick(&network_bandwidth_graph_globals);
            halo::networking::network_bandwidth_rate_compute(&network_bandwidth_graph_globals);
        }

        if (update_and_render_frame(connection, frame_average)) {
            break;
        }

        if (main_globals_data.quit != 0) {
            break;
        }
        if (main_globals_data.reset_frame_timers != 0) {
            main_globals_data.reset_frame_timers = 0;
            halo::platform::read_performance_counter(&counter);
            main_globals_data.frame_counter_low = (uint32_t)counter;
            main_globals_data.frame_counter_high = (uint32_t)(counter >> 32);
            main_globals_data.render_counter_low = (uint32_t)counter;
            main_globals_data.render_counter_high = (uint32_t)(counter >> 32);
            main_globals_data.frame_time_ms = (uint32_t)((counter * 1000) / halo::cseries::globals().performance_frequency);
            main_globals_data.time_is_running = 1;
        }
        if (shell_application_inactive != 0) {
            halo::platform::wait_for_messages((main_globals_data.game_connection > 0 && main_globals_data.game_connection <= 2) ? 20 : 100);
        }
        halo::platform::read_performance_counter(&counter);
        elapsed_ms = (int32_t)((counter * 1000) / halo::cseries::globals().performance_frequency) -
            frame_rate_average_data.sample_time_ms;
        frame_rate_average_data.history[0] = elapsed_ms;
        if ((uint32_t)elapsed_ms > k_main_frame_time_clamp_ms) {
            frame_rate_average_data.history[0] = k_main_frame_time_clamp_ms;
        }
    }
    halo::main::main_loop_shutdown_cleanup();
}

}

static auto &framerate_throttle = halo::link::ref<uint8_t>(halo::hs::vars().framerate_throttle);
static auto &unknown_00710301 = halo::link::ref<uint8_t>(halo::main::vars().unknown_00710301);
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

    pacing = (halo::game::globals().time_force_single_tick == 0 &&
              (framerate_throttle != 0 || halo::cutscene::globals().cinematic_globals->in_progress != 0))
                 ? 1
                 : 0;

    do {
        int64_t elapsed_ticks;
        uint32_t sleep_ms;

        halo::platform::read_performance_counter(&now);
        elapsed_ticks = now - (((int64_t)main_globals_data.frame_counter_high << 32) |
                                main_globals_data.frame_counter_low);
        elapsed_seconds = (double)elapsed_ticks / (double)halo::cseries::globals().performance_frequency;

        sleep_ms = (!pacing || (0.03333333507180214 - elapsed_seconds <= 0.012)) ? 0 : 10;
        halo::platform::sleep_milliseconds(sleep_ms);
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

        if (main_globals_data.game_connection == 0 && halo::cutscene::globals().cinematic_globals->in_progress == 0) {
            if (unknown_00710301 == 0) {
                if (elapsed_seconds > 0.06666666666666667) {
                    elapsed_seconds = 0.06666666666666667;
                }
            } else if (elapsed_seconds > 0.03333333333333333) {
                elapsed_seconds = 0.03333333333333333;
            }
        }
    } else {
        elapsed_seconds = (double)delta;
    }

    if (halo::game::globals().time_force_single_tick != 0) {
        int64_t frequency;
        int64_t step;
        int64_t new_counter;

        halo::platform::read_performance_frequency(&frequency);
        step = frequency / 30;
        new_counter = step + (((int64_t)main_globals_data.frame_counter_high << 32) |
                               main_globals_data.frame_counter_low);
        main_globals_data.frame_counter_low = (uint32_t)new_counter;
        main_globals_data.frame_counter_high = (uint32_t)(new_counter >> 32);
        main_globals_data.frame_delta_time = halo::math::k_seconds_per_tick;
        main_globals_data.frame_time_ms = main_globals_data.frame_time_ms + k_fallback_frame_time_ms;
        return;
    }

    main_globals_data.frame_counter_low = (uint32_t)now;
    main_globals_data.frame_counter_high = (uint32_t)(now >> 32);
    main_globals_data.frame_time_ms = (uint32_t)((now * 1000) / halo::cseries::globals().performance_frequency);
    main_globals_data.frame_delta_time = (float)elapsed_seconds;
}

}

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
    halo::interface::map_list_free_all();
    ban_list.element_size = -1;
    ban_list.count = -1;
    if (ban_list.data != 0) {
        halo::platform::heap_free(ban_list.data);
        ban_list.data = 0;
    }

    halo::networking::network_buffer_pair_pool_clear();
    if (main_globals_data.game_connection == 1) {
        halo::networking::network_client_globals_dispose();
    } else if (main_globals_data.game_connection == 2) {
        halo::networking::network_client_globals_dispose();
        if (halo::networking::globals().server != 0) {
            halo::networking::network_game_server_host_dispose(halo::networking::globals().server);
            halo::networking::globals().server = 0;
            halo::networking::globals().server_host_valid = 0;
            halo::game::game_stop_current_map();
            halo::game::game_dispose();
            halo::main::console_deactivate();
            halo::interface::chat_close();
            return;
        }
    }
    halo::game::game_stop_current_map();
    halo::game::game_dispose();
    halo::main::console_deactivate();
    halo::interface::chat_close();
}

}

static auto &main_menu_music_pending = halo::link::ref<uint8_t>(halo::main::vars().main_menu_music_pending);
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
        datum_index sound_tag = halo::cache::tag_lookup(k_looping_sound_group, "sound\\music\\title1\\title1");
        if (sound_tag != k_datum_index_none) {
            halo::sound::sound_looping_stop(sound_tag);
        }
        main_menu_music_pending = 0;
    }
    ui_split_screen = 0;
    main_globals_data.main_menu_scenario_loaded = 0;
    input_globals.mode_flags = input_globals.mode_flags & (uint8_t)~_input_mode_menu_bit;
}

}

static auto &interface_loading_screen_address_a = halo::link::ref<int32_t>(halo::main::vars().interface_loading_screen_address_a);
static auto &interface_loading_screen_address_b = halo::link::ref<int32_t>(halo::main::vars().interface_loading_screen_address_b);
static auto &join_ui_state = halo::link::ref<int32_t>(halo::networking::vars().join_ui_state);
static auto &interface_loading_screen_progress = halo::link::ref<int32_t>(halo::networking::vars().interface_loading_screen_progress);
static auto &progress_screen_text = halo::link::ref<uint16_t [0x20]>(halo::main::vars().progress_screen_text);
static auto &progress_screen_subtext = halo::link::ref<uint16_t [0x20]>(halo::main::vars().progress_screen_subtext);
static auto &interface_loading_screen_request_id = halo::link::ref<int32_t>(halo::networking::vars().interface_loading_screen_request_id);
static auto &ui_network_wait_timed_out = halo::link::ref<uint8_t>(halo::ui::vars().ui_network_wait_timed_out);
static auto &ui_network_wait_active = halo::link::ref<uint8_t>(halo::ui::vars().ui_network_wait_active);
static auto &ui_network_wait_start_time = halo::link::ref<int32_t>(halo::ui::vars().ui_network_wait_start_time);
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
        halo::main::chimera__load_ui_map(0);
    }
    halo::interface::chimera__load_main_menu();
    if (halo::scenario::globals().scenario != 0) {
        halo::cache::predicted_resource_list_touch(&halo::scenario::globals().scenario->predicted_resources);
    }

    interface_loading_screen_address_a = -1;
    interface_loading_screen_address_b = -1;
    join_ui_state = 0;
    interface_loading_screen_progress = 0;
    progress_screen_text[0] = 0;
    progress_screen_subtext[0] = 0;
    interface_loading_screen_request_id = -1;

    halo::interface::hud_chat_listbox_clear();
    ui_network_wait_active = 0;
    ui_network_wait_start_time = -1;
    ui_network_wait_timed_out = 0;

    halo::game::update_queues_dispose();
    halo::game::update_server_new();
    halo::game::update_server_dispose();

    if (halo::game::globals().game_time != 0) {
        halo::game::globals().game_time->initialized = 0;
        halo::game::globals().game_time->active = 0;
    }
    memset(halo::game::globals().game_time, 0, sizeof(game_time_globals));
    halo::game::globals().game_time->initialized = 1;

    halo::game::game_engine_init_tick_record_for_mode();
    halo::hs::hs_dispose_dynamic_globals();
    halo::hs::hs_scenario_scripts_initialize();

    main_globals_data.return_to_main_menu = 0;
    input_globals.mode_flags = input_globals.mode_flags | 2;
}

}
