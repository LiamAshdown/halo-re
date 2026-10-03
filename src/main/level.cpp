/**
 * Map and scenario session control: queued map changes, level transitions, saving and credits.
 */

#include "tags.h"
#include "halo/scenario/api.hpp"
#include "memory.h"
#include "interface.h"
#include "main.h"
#include <string.h>
#include <ctype.h>
#include "math.h"
#include "game.h"
#include "camera.h"
#include "networking.h"
#include "cache.h"
#include "saved_games.h"
#include "win32.h"
#include "crt.h"
#include <stdio.h>
#include <wchar.h>

#include "halo/main/level.hpp"
#include "halo/cache/api.hpp"
#include "halo/input/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/main/layout.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/main/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/directinput.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/scenario/scenario.hpp"


extern "C" { extern main_globals main_globals_data; }
extern "C" { extern char *campaign_level_paths[k_main_campaign_level_count]; }
namespace halo::main {

/**
 * Determines the next single-player campaign level after finishing the current one (by finding
 * the current scenario's campaign index and advancing by one), marks it visited for every local
 * player, and either queues it to load, rolls the end credits (once past the last level), or --
 * in the unreachable-in-practice fallback -- returns to the main menu.
 *
 * @address 0x4c9bd0
 */
void LevelControl::campaign_level_advance(void)
{
    int16_t next_index;
    int16_t i;

    main_globals_data.return_to_main_menu = 1;
    main_globals_data.won_map = 0;

    next_index = (int16_t)(halo::main::campaign_level_find_index_for_path(main_globals_data.scenario_path) + 1);
    if (next_index > 9) {
        next_index = -1;
    }

    for (i = 0; i < halo::game::globals().local_player_count; i++) {
        halo::saved_games::player_profile_mark_level_visited_and_select(i);
    }

    if (next_index == -1) {
        halo::main::credits_load_directly_for_endgame();
        return;
    }
    if (next_index >= 0 && next_index < 10) {
        halo::main::main_queue_map_change(campaign_level_paths[next_index]);
        main_globals_data.restore_checkpoint_on_load = 0;
        return;
    }

    main_globals_data.switch_structure_bsp_index = -1;
    main_globals_data.save_map = 0;
    main_globals_data.return_to_main_menu = 1;
}

}

extern "C" { extern char campaign_level_short_names[k_main_campaign_level_count][4]; }
namespace halo::main {

/**
 * Lowercases a copy of `path` and checks it against each of the ten campaign level short codes
 * ("a10".."d40"), starting from the last table entry ("a10") and working backward to the first
 * ("d40"), returning the matching campaign order index (0 for "a10" through 9 for "d40"), or -1
 * if none of them appear in the path.
 *
 * @address 0x4c8b90
 */
int LevelControl::campaign_level_find_index_for_path(char *path)
{
    char lower_path[128];
    char *c;
    int i;

    strncpy(lower_path, path, sizeof(lower_path) - 1);
    lower_path[sizeof(lower_path) - 1] = 0;
    for (c = lower_path; *c != 0; c++) {
        *c = (char)tolower((uint8_t)*c);
    }

    for (i = k_main_campaign_level_count - 1; i >= 0; i--) {
        if (strstr(lower_path, campaign_level_short_names[i]) != 0) {
            return (k_main_campaign_level_count - 1) - i;
        }
    }
    return -1;
}

}

extern "C" { extern uint8_t player_profile_cache_initialized; }
extern "C" { extern uint8_t player_profile_cache[0xc0 * 4]; }
extern "C" { extern uint8_t game_engine_active_variant[0x26 * 4]; }
extern "C" { extern uint8_t ui_split_screen; }
extern "C" { extern uint8_t *main_game_globals; }
namespace halo::main {

/**
 * Loads the front-end map ("levels\\ui\\ui") as a scenario session: switches the cache file to
 * it, tears down whatever map/game engine was running, resets the camera to the scripted
 * (cutscene) director, and starts game_scenario_session_begin on a freshly built scenario load
 * request. Optionally starts the main menu title music afterward.
 *
 * @address 0x4c8930
 */
void LevelControl::chimera__load_ui_map(char play_title_music)
{
    network_scenario_load_request request;

    halo::game::cache_file_switch_map_by_path((char *)k_ui_scenario_path, 1);

    memset(&request, 0, sizeof(request));
    request.difficulty = 1;
    request.salt = k_scenario_load_request_salt;
    strncpy(request.map_name, k_ui_scenario_path, k_main_path_length - 1);
    request.map_name[k_main_path_length - 1] = 0;

    halo::game::cache_file_switch_map_by_path(request.map_name, 1);
    halo::game::game_stop_current_map();
    halo::game::game_unload_map();

    if (halo::game::globals().current_engine != 0) {
        if (halo::game::globals().current_engine->dispose != 0) {
            ((void (*)(void))halo::game::globals().current_engine->dispose)();
        }
        halo::game::globals().current_engine = 0;
    }
    if (player_profile_cache_initialized == 1) {
        memset(player_profile_cache, 0, sizeof(player_profile_cache));
        player_profile_cache_initialized = 0;
    }
    memset(game_engine_active_variant, 0, sizeof(game_engine_active_variant));

    main_globals_data.main_menu_scenario_loaded = 1;
    halo::main::game_scenario_session_begin(&request);

    *halo::camera::globals().hs_camera_control_pointer = 1;
    halo::camera::globals().directors[0].pov_proc = halo::camera::camera_debug_compute_pov;
    halo::camera::globals().directors[0].look_scale = 1.0f;
    halo::camera::globals().directors[0].unknown_c0 = 0;
    halo::camera::globals().camera_script.camera_control = 1;
    halo::camera::globals().camera_script.changed = 1;
    halo::camera::camera_debug_start(0, 0, k_datum_index_none);

    ui_split_screen = 1;
    main_globals_data.unknown_06b = 1;

    if (play_title_music != 0 && halo::scenario::globals().scenario != 0) {
        halo::cache::predicted_resource_list_touch(&halo::scenario::globals().scenario->predicted_resources);
    }
}

}

extern "C" { extern saved_player_profile_slot profile_globals_block[k_maximum_local_player_profiles]; }
extern "C" { extern int32_t hud_text_message_cycle_state_00719230; }
namespace halo::main {

/**
 * Marks the current profile as having reached the end credits, saves it if a profile is
 * currently selected, returns to the main menu, and immediately shows the end-game credits
 * screen widget (with the main menu's own widget as its history entry, so backing out returns
 * there) and marks the HUD text message cycle as active.
 *
 * @address 0x4c8d40
 */
void LevelControl::credits_load_directly_for_endgame(void)
{
    datum_index main_menu_tag;

    profile_globals_block[0].profile.flags = profile_globals_block[0].profile.flags | _saved_player_profile_end_credits_reached_bit;
    if (profile_globals_block[0].handle != -1) {
        halo::saved_games::player_profile_write_data(profile_globals_block[0].handle, &profile_globals_block[0].profile);
    }
    halo::main::main_menu_return_and_reset();
    main_menu_tag = halo::cache::tag_lookup(k_ui_widget_definition_group, (char *)"ui\\shell\\main_menu\\main_menu");
    halo::interface::chimera__load_ui_widget((char *)"ui\\shell\\main_menu\\credits_screen", k_datum_index_none,
                             (widget_instance *)0, (uint16_t)-1, main_menu_tag, k_datum_index_none,
                             -1);
    hud_text_message_cycle_state_00719230 = 1;
}

}

extern "C" { extern uint8_t console_debug_flag_0; }
extern "C" { extern int16_t console_debug_word_8; }
extern "C" { extern int32_t ui_pause_pending_count_00718fa0; }
extern "C" { extern int32_t join_ui_state; }
extern "C" { extern int32_t interface_loading_screen_address_a; }
extern "C" { extern int32_t interface_loading_screen_address_b; }
extern "C" { extern void main_ensure_local_players(void); }
extern "C" { extern void game_state_load_checkpoint(void); }
namespace halo::main {

/**
 * Loads a scenario per `request`, seeds the game timer and pending-pause bookkeeping, and (on
 * the first session only) ensures the local players exist. Copies `request` into the persistent
 * main_game_globals buffer, calls scenario_load, resets the level/save/won/lost/respawn/
 * core-load request flags, re-baselines last_activity_time_ms from QueryPerformanceCounter,
 * restores a checkpoint if one was requested, and -- outside a networked game with the loading
 * screen up -- re-arms the loading screen's minimum-display-time deadline.
 *
 * @address 0x4c95f0
 */
void LevelControl::scenario_session_begin(network_scenario_load_request *request)
{
    int64_t counter;
    int64_t counter_ms;
    uint8_t already_initialized;

    halo::input::input_reset_state_and_axis_configs();
    halo::input::input_bind_capture_reset();
    halo::game::cache_file_switch_map_by_path(request->map_name, 1);

    memcpy(main_game_globals + 8, request, sizeof(network_scenario_load_request));

    if (halo::scenario::scenario_load(request->map_name) == 0) {
        if (*main_game_globals == 0) {
            goto after_load;
        }
    } else {
        *main_game_globals = 1;
    }
    halo::game::game_start_new_map();

after_load:
    already_initialized = console_debug_flag_0 != 0;
    console_debug_flag_0 = 0;
    console_debug_word_8 = 0;
    if (!already_initialized) {
        halo::main::main_ensure_local_players();
        halo::game::game_engine_init_tick_record_for_mode();
    }
    halo::game::game_engine_reset_all_players();

    main_globals_data.switch_structure_bsp_index = -1;
    main_globals_data.reset_map = 0;
    main_globals_data.level_transition = 0;
    main_globals_data.revert_map = 0;
    main_globals_data.revert_map_if_allowed = 0;
    main_globals_data.save_map = 0;
    main_globals_data.won_map = 0;
    main_globals_data.lost_map = 0;
    main_globals_data.respawn_coop_players = 0;
    main_globals_data.save_core = 0;
    main_globals_data.load_core = main_globals_data.load_core_next_session;
    main_globals_data.load_core_next_session = 0;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    counter_ms = counter * 1000;
    main_globals_data.last_activity_time_ms = (int32_t)(counter_ms / halo::cseries::globals().performance_frequency);

    if (main_globals_data.restore_checkpoint_on_load != 0) {
        halo::saved_games::game_state_load_checkpoint();
    }

    ui_pause_pending_count_00718fa0 = k_ui_pause_pending_ticks;
    if (main_globals_data.game_connection == 0 && join_ui_state != 0) {
        int32_t now = halo::cseries::time_query_performance_counter_ms();
        uint32_t extra = 0;

        if (interface_loading_screen_address_b != -1) {
            uint32_t elapsed = (uint32_t)(now - interface_loading_screen_address_b);
            if (elapsed < k_loading_screen_maximum_delay_ms && join_ui_state != 1) {
                extra = (uint32_t)(interface_loading_screen_address_b - now) + k_loading_screen_maximum_delay_ms;
                if (extra > k_loading_screen_maximum_delay_ms) {
                    extra = k_loading_screen_maximum_delay_ms;
                }
            }
        }
        interface_loading_screen_address_a = (int32_t)(extra + k_loading_screen_extra_delay_ms + now);
    }
}

}

extern "C" { extern int16_t pending_difficulty; }
namespace halo::main {

/**
 * Starts loading the currently selected map (scenario_path) as a new single-player game
 * session, unless film playback was requested (in which case it switches to that connection
 * mode and returns to the main menu instead) or a return-to-main-menu is already pending (in
 * which case it just services that instead).
 *
 * @address 0x4c9dd0
 */
void LevelControl::start_new_single_player_map(void)
{
    network_scenario_load_request request;

    if (main_globals_data.start_film_playback != 0) {
        main_globals_data.game_connection = _game_connection_film_playback;
        main_globals_data.return_to_main_menu = 1;
        halo::main::main_menu_return_and_reset();
        return;
    }
    if (main_globals_data.return_to_main_menu != 0) {
        halo::main::main_menu_return_and_reset();
        return;
    }

    memset(&request, 0, sizeof(request));
    main_globals_data.game_connection = _game_connection_local;
    request.salt = k_scenario_load_request_salt;
    strncpy(request.map_name, main_globals_data.scenario_path, k_main_path_length - 1);
    request.map_name[k_main_path_length - 1] = 0;
    request.difficulty = pending_difficulty;

    halo::game::cache_file_switch_map_by_path(request.map_name, 1);
    halo::game::game_stop_current_map();
    halo::main::game_scenario_session_begin(&request);
}

}

extern "C" { extern char map_path_prefix[]; }
extern "C" { extern uint8_t main_menu_music_pending; }
extern "C" { extern uint8_t ui_input_batch_mode; }
extern "C" { extern float ui_unknown_718fa8; }
extern "C" { extern int32_t interface_loading_screen_progress; }
extern "C" { extern uint16_t progress_screen_text[0x20]; }
extern "C" { extern uint16_t progress_screen_subtext[0x20]; }
extern "C" { extern int32_t interface_loading_screen_request_id; }
extern "C" { extern int32_t _access(const char *path, int32_t mode); }
namespace halo::main {

/**
 * Drives a queued level change: first confirms the target map file exists (aborting the
 * transition if not), resets the loading screen when starting a local (non-networked) session,
 * then runs and tracks a 1 second fade-out of the main menu's music/UI (only once the menu is
 * actually shown) before proceeding. Once the fade has completed (or there was nothing to fade),
 * unless an idle timeout deferred it, stops the map/game engine and starts loading
 * scenario_path as a fresh session for every local player.
 *
 * @address 0x4c9770
 */
void LevelControl::level_transition_update(void)
{
    if (main_globals_data.level_transition_fade_end_ms == 0) {
        char basename[256];
        char *slash;
        char *name;
        char map_path[512];

        strncpy(basename, main_globals_data.scenario_path, k_main_path_length - 1);
        basename[k_main_path_length - 1] = 0;
        slash = strrchr(basename, '\\');
        name = (slash == 0) ? basename : slash + 1;

        sprintf(map_path, "%s%s%s.map", map_path_prefix, "maps\\", name);
        if (_access(map_path, 0) != 0) {
            main_globals_data.level_transition = 0;
            return;
        }
    }

    if (main_globals_data.game_connection == 0) {
        interface_loading_screen_address_a = -1;
        interface_loading_screen_address_b = -1;
        interface_loading_screen_progress = 0;
        progress_screen_text[0] = 0;
        progress_screen_subtext[0] = 0;
        interface_loading_screen_request_id = -1;
        join_ui_state = 1;
    }

    if (main_globals_data.main_menu_scenario_loaded == 1) {
        if (main_globals_data.level_transition_fade_end_ms == 0) {
            if (main_menu_music_pending != 1) {
                goto after_fade;
            }
            main_globals_data.level_transition_fade_end_ms = main_globals_data.frame_time_ms + 1000;
            halo::interface::main_menu_on_shown(1000);
            ui_input_batch_mode = 1;
            ui_unknown_718fa8 = 0.0f;
        } else {
            float remaining = (float)(int32_t)(main_globals_data.level_transition_fade_end_ms -
                                                main_globals_data.frame_time_ms);
            if ((int32_t)(main_globals_data.level_transition_fade_end_ms -
                           main_globals_data.frame_time_ms) < 0) {
                remaining = remaining + 4.2949673e+09f;
            }
            ui_unknown_718fa8 = 1.0f - remaining * 0.001f;
        }
        if (main_globals_data.frame_time_ms < main_globals_data.level_transition_fade_end_ms) {
            return;
        }
    } else {
        main_globals_data.level_transition_fade_end_ms = 0;
    }

after_fade:
    if (main_globals_data.idle_timeout_reached == 0) {
        ui_unknown_718fa8 = -1.0f;
        halo::main::main_menu_music_stop();
        ui_input_batch_mode = 0;

        if (halo::game::globals().game_time->initialized != 0 &&
            (halo::game::globals().game_time->active != 0 || halo::game::globals().game_time->paused != 0) &&
            main_globals_data.game_connection == 0) {
            network_scenario_load_request request;
            int16_t i;

            memset(&request, 0, sizeof(request));
            request.salt = k_scenario_load_request_salt;
            strncpy(request.map_name, main_globals_data.scenario_path, k_main_path_length - 1);
            request.map_name[k_main_path_length - 1] = 0;
            request.difficulty = pending_difficulty;

            halo::game::game_stop_current_map();
            halo::game::cache_file_switch_map_by_path(request.map_name, 1);
            halo::game::game_unload_map();
            halo::main::game_scenario_session_begin(&request);

            for (i = 0; i < halo::game::globals().local_player_count; i++) {
                halo::saved_games::player_profile_select_local_slot(i);
            }
        }
        main_globals_data.level_transition_fade_end_ms = 0;
        return;
    }

    main_globals_data.return_to_main_menu = 0;
    main_globals_data.unknown_05a = 1;
    main_globals_data.level_transition_fade_end_ms = 0;
}

}

namespace halo::main {

/**
 * Stages name as the cache file for the main loop to open next frame, or clears the request
 * when name is NULL.
 *
 * Original register convention: EAX -> name.
 *
 * @address 0x4c8900
 */
void LevelControl::queue_cache_file_open(char *name)
{
    if (name != 0) {
        strncpy(main_globals_data.pending_cache_file_name, name, k_main_path_length - 1);
        main_globals_data.cache_file_open_pending = 1;
    } else {
        main_globals_data.pending_cache_file_name[0] = 0;
        main_globals_data.cache_file_open_pending = 0;
    }
}

}

namespace halo::main {

/**
 * Stages map_name as the next scenario to load and arms restore_checkpoint_on_load, clearing any
 * pending "confirm quit" prompt first. If the simulation is currently active or paused in a
 * local (non-networked) game, also arms main_globals.level_transition.
 *
 * Original register convention: EAX -> map_name.
 *
 * @address 0x4c8740
 */
void LevelControl::queue_map_change(char *map_name)
{
    main_globals_data.return_to_main_menu = 0;
    strncpy(main_globals_data.scenario_path, map_name, k_main_path_length - 1);
    main_globals_data.scenario_path[k_main_path_length - 1] = 0;
    main_globals_data.restore_checkpoint_on_load = 1;
    if (halo::game::globals().game_time->initialized != 0 &&
        (halo::game::globals().game_time->active != 0 || halo::game::globals().game_time->paused != 0) &&
        main_globals_data.game_connection == 0) {
        main_globals_data.level_transition = 1;
    }
}

}

namespace halo::main {

/**
 * Stages name (or clears the staging buffer when name is NULL) as the multiplayer map to load,
 * refreshes the loading screen's cached friendly display name for it, and kicks off
 * cache_file_request_map on the staged buffer either way.
 * BEFORE the name == NULL check below (confirmed in objdump: the call at 0x4c87ac precedes the
 * `test edi,edi` at 0x4c87b4) -- a literal NULL name would fault inside strncpy on retail
 * hardware. Preserved exactly as disassembled; presumably no real call site at 0x45fc81/
 * 0x47f55d exercises the NULL path with strncpy still faulting in practice.
 *
 * Original register convention: EDI -> name.
 *
 * @address 0x4c87a0
 */
uint8_t LevelControl::queue_map_change_by_name_or_clear(char *name)
{
    strncpy(main_globals_data.multiplayer_map_name, name, k_main_path_length - 1);
    main_globals_data.multiplayer_map_name[k_main_path_length - 1] = 0;
    if (name == 0) {
        progress_screen_subtext[0] = 0;
    } else {
        halo::interface::map_list_get_friendly_level_name((wchar_t *)progress_screen_subtext, name, 0x40);
    }
    return halo::cache::cache_file_request_map(main_globals_data.multiplayer_map_name, 0);
}

}

namespace halo::main {

/**
 * Services a pending main_globals.save_map request: if it doesn't require a safe moment, saves
 * immediately (reporting "unsafe save" if debug_game_save is set); otherwise counts attempts,
 * gives up (reporting an error) after 0xf0 attempts when save_map_with_timeout is set, waits out
 * a retry countdown, then polls game_safe_to_save, requiring three consecutive safe checks
 * before actually committing. On commit (unless a timedemo tick is in flight), shows the
 * checkpoint HUD message and arms save_map_write_pending; either way clears save_map.
 *
 * @address 0x4c9a70
 */
void LevelControl::save_map_private(void)
{
    uint8_t ready_to_save;

    if (halo::game::globals().game_time->paused != 0) {
        return;
    }

    ready_to_save = 0;

    if (main_globals_data.save_map_require_safe == 0) {
        if (main_globals_data.debug_game_save != 0) {
            halo::main::console_print_error_va(0, "unsafe save");
        }
    } else {
        int32_t next_attempt_count = main_globals_data.save_map_attempt_count + 1;

        if (main_globals_data.save_map_attempt_count > k_main_save_give_up_attempts - 1 &&
            main_globals_data.save_map_with_timeout != 0) {
            main_globals_data.save_map_attempt_count = next_attempt_count;
            if (main_globals_data.debug_game_save == 0) {
                main_globals_data.save_map = 0;
                return;
            }
            halo::main::console_print_error_va(0, "gave up trying to save");
            main_globals_data.save_map = 0;
            return;
        }

        if (main_globals_data.save_map_retry_countdown > 0) {
            main_globals_data.save_map_retry_countdown =
                main_globals_data.save_map_retry_countdown - 1;
            main_globals_data.save_map_attempt_count = next_attempt_count;
            return;
        }

        main_globals_data.save_map_retry_countdown =
            main_globals_data.save_map_retry_countdown - 1;
        main_globals_data.save_map_attempt_count = next_attempt_count;

        if (halo::game::game_safe_to_save() == 0) {
            main_globals_data.save_map_safe_streak = 0;
        } else {
            int16_t new_streak = main_globals_data.save_map_safe_streak + 1;
            uint8_t streak_reached = main_globals_data.save_map_safe_streak > 2;

            main_globals_data.save_map_safe_streak = new_streak;
            if (streak_reached) {
                ready_to_save = 1;
            }
        }

        main_globals_data.save_map_retry_countdown = 10;
        if (!ready_to_save) {
            return;
        }
    }

    if (halo::game::globals().time_force_single_tick == 0) {
        halo::interface::hud_display_checkpoint_message(1);
        main_globals_data.save_map_write_pending = 1;
    }
    main_globals_data.save_map = 0;
}

}

extern "C" { extern uint8_t *hud_messaging; }
namespace halo::main {

/**
 * Performs the queued structure BSP switch, resets switch_structure_bsp_index, clears a small
 * per-message "active" byte in each of 4 stride-0x8c hud_messaging entries, and if a HUD message
 * id is queued (hud_globals_tag_data->loading_end_text != -1), fetches and displays it (for local player 0 when one exists).
 *
 * @address 0x4c9b60
 */
void LevelControl::switch_structure_bsp_and_notify(void)
{
    int16_t message_id;
    uint8_t *entry;
    int32_t i;

    halo::scenario::structure_bsp_switcher::switch_to(main_globals_data.switch_structure_bsp_index);
    main_globals_data.switch_structure_bsp_index = -1;

    message_id = (int16_t)halo::interface::globals().hud_globals_tag_data->loading_end_text;

    entry = hud_messaging + k_hud_messaging_active_offset;
    for (i = k_hud_messaging_entry_count; i != 0; i--) {
        *entry = 0;
        entry = entry + k_hud_messaging_entry_size;
    }

    if (message_id != -1) {
        int16_t local_player_index = -1;
        uint16_t *text;

        if (halo::game::globals().local_player_globals->local_players[0] != k_datum_index_none) {
            local_player_index = 0;
        }
        text = halo::interface::hud_get_message_string(message_id);
        halo::interface::chimera__hud_message(local_player_index, (const wchar_t *)text);
    }
}

}
