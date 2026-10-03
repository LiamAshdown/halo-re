/**
 * Game engine lifetime: new game set-up, end-game sequence stages and per-frame effects.
 */

#include "win32.h"
#include "halo/networking/game_mode.hpp"
#include "halo/game/records.hpp"
#include "halo/rasterizer/render_device.hpp"
#include "halo/game/constants.hpp"
#include "halo/core/datum.hpp"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "networking.h"
#include <stdint.h>

#include "halo/game/game1_lifecycle.hpp"
#include "halo/game/legacy_globals.hpp"
#include "halo/memory/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/main/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/effects/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/main/vars.hpp"
#include "halo/networking/vars.hpp"

static auto &current_game_engine = halo::link::ref<game_engine_definition *>(halo::game::vars().current_game_engine);
static auto &player_profile_cache_initialized = halo::link::ref<uint8_t>(halo::game::vars().player_profile_cache_initialized);
static auto &player_profile_cache = halo::link::ref<player_profile [16]>(halo::game::vars().player_profile_cache);
static auto &weather_particle_data = halo::link::ref<void *>(halo::game::vars().weather_particle_data);
static auto &effect_data = halo::link::ref<uint32_t>(halo::effects::vars().effect_data);
static auto &effect_location_data = halo::link::ref<uint32_t>(halo::effects::vars().effect_location_data);
static auto &particle_data = halo::link::ref<uint32_t>(halo::effects::vars().particle_data);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &team_data = halo::link::ref<data_array *>(halo::game::vars().team_data);
static auto &local_player_globals = halo::link::ref<player_globals *>(halo::game::vars().local_player_globals);
static auto &widget_memory_pool = halo::link::ref<heap *>(halo::ui::vars().widget_memory_pool);
static auto &ui_root_widget = halo::link::ref<uint32_t [13]>(halo::ui::vars().ui_root_widget);
static auto &sound_class_gains = halo::link::ref<uint32_t>(halo::game::vars().sound_class_gains);
static auto &rasterizer_device = halo::link::ref<uint32_t>(halo::game::vars().rasterizer_device);
static auto &rasterizer_decal_vertex_cache = halo::link::ref<void **>(halo::effects::vars().rasterizer_decal_vertex_cache);
static auto &object_render_state_cache = halo::link::ref<uint32_t>(halo::game::vars().object_render_state_cache);
static auto &terminal_messages = halo::link::ref<data_array *>(halo::main::vars().terminal_messages);
static auto &game_state_write_buffer = halo::link::ref<void *>(halo::game::vars().game_state_write_buffer);
static auto &input_event_queue_active = halo::link::ref<uint32_t [0x43]>(halo::ui::vars().input_event_queue_active);
static auto &input_globals = halo::link::ref<uint32_t [0x97c]>(halo::main::vars().input_globals);
static auto &profile_globals_block = halo::link::ref<uint32_t [0x1829]>(halo::ui::vars().profile_globals_block);
static auto &game_state_write_buffer_allocated = halo::link::ref<uint8_t>(halo::game::vars().game_state_write_buffer_allocated);
static auto &game_state_persistent_storage = halo::link::ref<void *>(halo::game::vars().game_state_persistent_storage);
static auto &game_state_persistent_storage_created = halo::link::ref<uint8_t>(halo::game::vars().game_state_persistent_storage_created);
static auto &game_engine_state_value = halo::link::ref<game_engine_state>(halo::game::vars().game_engine_state_value);
static auto &network_server = halo::link::ref<network_server_globals *>(halo::networking::vars().network_server);
static auto &game_engine_end_game_timer = halo::link::ref<float>(halo::game::vars().game_engine_end_game_timer);
static auto &game_engine_post_game_fade = halo::link::ref<float>(halo::game::vars().game_engine_post_game_fade);
static auto &game_engine_dedicated_idle = halo::link::ref<uint8_t>(halo::game::vars().game_engine_dedicated_idle);
static auto &game_engine_dedicated_idle_timer = halo::link::ref<float>(halo::game::vars().game_engine_dedicated_idle_timer);
static auto &game_engine_variant = halo::link::ref<game_variant>(halo::game::vars().game_engine_variant);
static auto &game_engine_map_table_value = halo::link::ref<uint8_t>(halo::game::vars().game_engine_map_table_value);
static auto &server_end_game_requested = halo::link::ref<uint8_t>(halo::game::vars().g_006f1d25);
static auto &network_build_string = halo::link::ref<char []>(halo::networking::vars().network_build_string);
struct MapGameTableEntry {
    uint8_t game_engine_value;
    uint8_t unknown_01[11];
};
static_assert(sizeof(MapGameTableEntry) == 12, "per-map game engine table stride (retail reads a byte at stride 12)");
static auto &map_per_map_table = halo::link::ref<MapGameTableEntry [0x13]>(halo::game::vars().map_per_map_table);
static auto &network_session_host_state = halo::link::ref<uint8_t>(halo::networking::vars().network_session_host_state);
static auto &multiplayer_sound_queue = halo::link::ref<multiplayer_sound_request [5]>(halo::game::vars().multiplayer_sound_queue);
static auto &multiplayer_sound_queue_count = halo::link::ref<int32_t>(halo::game::vars().multiplayer_sound_queue_count);
static auto &custom_waypoints = halo::link::ref<custom_waypoint [k_maximum_custom_waypoints]>(halo::game::vars().custom_waypoints);
static_assert(sizeof(custom_waypoint [k_maximum_custom_waypoints]) == 0x400, "custom waypoint table size");
static_assert(sizeof(player_profile [16]) == 0xc0 * sizeof(uint32_t), "player profile cache size");
static auto &game_engine_auto_team_counter = halo::link::ref<int32_t>(halo::game::vars().game_engine_auto_team_counter);
static auto &game_engine_ctf_reset_ticks = halo::link::ref<int32_t>(halo::game::vars().game_engine_ctf_reset_ticks);
static auto &network_client = halo::link::ref<uint8_t *>(halo::networking::vars().network_client);

namespace halo::game::engine1 {

/**
 * Disposes the per-game state: the dynamic script globals, the widgets, the looping sound data and the active
 * game engine through its dispose callback.
 *
 * @address 0x45acd0
 */
void Lifecycle::dispose(void)
{
    uint32_t i;

    halo::hs::hs_dispose_dynamic_globals();
    halo::interface::widget_close_all();
    if (widget_memory_pool->base != 0) {
        GlobalFree(widget_memory_pool->base);
    }
    widget_memory_pool->base = 0;
    widget_memory_pool->size = 0;

    for (i = 0; i < 13; i = i + 1) {
        ui_root_widget[i] = 0;
    }
    halo::sound::globals().game_looping_sound_data = (data_array *)0;
    sound_class_gains = 0;

    if (current_game_engine != (game_engine_definition *)0) {
        if (current_game_engine->dispose != nullptr) {
            ((void (*)(void))current_game_engine->dispose)();
        }
        current_game_engine = (game_engine_definition *)0;
    }

    if (player_profile_cache_initialized == 1) {
        memset(player_profile_cache, 0, sizeof(player_profile_cache));
        player_profile_cache_initialized = 0;
    }

    if (weather_particle_data != nullptr) {
        memset(weather_particle_data, 0, 14 * sizeof(uint32_t));
        GlobalFree(weather_particle_data);
        weather_particle_data = nullptr;
    }
    effect_data = 0;
    effect_location_data = 0;
    particle_data = 0;
    halo::effects::globals().contrail_point_data = (data_array *)0;
    halo::effects::globals().contrail_data = (data_array *)0;
    player_data = (data_array *)0;
    team_data = (data_array *)0;
    local_player_globals = (player_globals *)0;
    halo::effects::globals().decal_data = (data_array *)0;

    if (rasterizer_device != 0 && rasterizer_decal_vertex_cache != nullptr) {
        halo::rasterizer::render_device().release(rasterizer_decal_vertex_cache);
        rasterizer_decal_vertex_cache = nullptr;
    }
    object_render_state_cache = 0;
    halo::objects::objects_dispose();

    if (halo::main::globals().console_win32_attached != 0) {
        halo::main::globals().console_win32_attached = 0;
    }
    if (terminal_messages != (data_array *)0) {
        memset(terminal_messages, 0, sizeof(*terminal_messages));
        GlobalFree(terminal_messages);
    }
    halo::main::globals().terminal_initialized = 0;
    halo::saved_games::saved_game_files_dispose();

    for (i = 0; i < 0x43; i = i + 1) {
        input_event_queue_active[i] = 0;
    }
    for (i = 0; i < 0x97c; i = i + 1) {
        input_globals[i] = 0;
    }
    for (i = 0; i < 0x1829; i = i + 1) {
        profile_globals_block[i] = 0;
    }
    GlobalFree(game_state_write_buffer);
    game_state_write_buffer_allocated = 0;
    CloseHandle(game_state_persistent_storage);
    game_state_persistent_storage_created = 0;

    halo::networking::network_shutdown();
}

/**
 * After a structure-BSP switch, attempts to give every player without a unit a valid unit and attach it to the
 * new BSP's parent object, retrying as needed.
 *
 * The candidate parent is the player's own unit when its root is a grounded vehicle, as in retail.
 *
 * @address 0x473e90
 */
uint8_t Lifecycle::attach_players_to_new_bsp(void)
{
    object_iterator obj_iter;
    data_iterator player_iter;
    player *plr;
    datum_index unit_handle, walk, next, root, best_root, player_handle;
    object *root_obj;
    biped_data *biped;
    vehicle_data *vehicle;
    uint8_t success;

    local_player_globals->mode = 0;

    if (local_player_globals->teleported == 0) {
        obj_iter.type_mask = _object_mask_projectile;
        obj_iter.flags_mask = 0;
        obj_iter.index = 0;
        obj_iter.handle = (datum_index)-1;
        if (halo::objects::object_iterator_next(&obj_iter) != (object *)0 || halo::units::unit_any_dying_or_seat_transition() != 0) {
            local_player_globals->mode = 1;
            return 0;
        }
        if (local_player_globals->teleported == 0 && halo::ai::ai_scan_for_recent_combat_activity(1) != 0) {
            local_player_globals->mode = 2;
            return 0;
        }
    }

    player_iter.data = player_data;
    player_iter.next_index = 0;
    player_iter.index = (datum_index)-1;
    player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
    best_root = (datum_index)-1;
    success = 0;

    plr = (player *)halo::memory::data_iterator_next(&player_iter);
    if (plr != (player *)0) {
        do {
            unit_handle = plr->unit;
            root = best_root;
            if (unit_handle != (datum_index)-1) {
                walk = unit_handle;
                do {
                    root = walk;
                    next = halo::game::object_at(walk)->parent_object;
                    walk = next;
                } while (next != (datum_index)-1);

                if (root == unit_handle) {
                    root_obj = halo::objects::object_try_and_get(unit_handle, _object_mask_biped);
                    if (root_obj != (object *)0) {
                        biped = &reinterpret_cast<biped_object *>(root_obj)->biped;
                        if ((biped->flags & 1) != 0) {
                            local_player_globals->mode = 3;
                            root = best_root;
                        }
                    }
                } else {
                    root_obj = halo::objects::object_try_and_get(root, _object_mask_vehicle);
                    if (root_obj != (object *)0) {
                        vehicle = &reinterpret_cast<vehicle_object *>(root_obj)->vehicle;
                        if (vehicle->airborne_ticks != 0) {
                            local_player_globals->mode = 3;
                            root = best_root;
                        } else {
                            root = unit_handle;
                        }
                    } else {
                        root = unit_handle;
                    }
                }
            }
            plr = (player *)halo::memory::data_iterator_next(&player_iter);
            best_root = root;
        } while (plr != (player *)0);

        success = 0;
        if (best_root != (datum_index)-1) {
            success = 1;
            player_iter.data = player_data;
            player_iter.next_index = 0;
            player_iter.index = (datum_index)-1;
            player_iter.signature = (uint32_t)(uintptr_t)player_iter.data ^ k_data_iterator_signature;
            plr = (player *)halo::memory::data_iterator_next(&player_iter);
            while (plr != (player *)0) {
                if (plr->unit == (datum_index)-1) {
                    player_handle = player_iter.index;
                    halo::game::player_respawn(player_handle);
                    if (plr->unit == (datum_index)-1) {
                        success = 0;
                    } else {
                        root_obj = halo::game::object_at(best_root);
                        success = halo::game::player_attach_unit_to_parent(player_handle, best_root, &root_obj->bounding_center);
                    }
                }
                plr = (player *)halo::memory::data_iterator_next(&player_iter);
            }
        }
    }

    if (local_player_globals->teleported == 0 || success != 0) {
        local_player_globals->teleported = 0;
    } else {
        local_player_globals->teleported = 1;
    }
    if (success != 0) {
        local_player_globals->mode = 0;
    }
    return success;
}

/**
 * Server only: starts the end-of-game sequence when none has started, setting the ending state with a 7 second
 * timer, queueing the end sound, closing the widgets and sending the end-game notification.
 *
 * @address 0x45fd90
 */
void Lifecycle::begin_end_game_sequence(void)
{
    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host && game_engine_state_value == _game_engine_state_not_started) {
        network_server->game_over = 1;
        game_engine_state_value = _game_engine_state_ending;
        game_engine_end_game_timer = 7.0f;
        halo::game::game_engine_queue_multiplayer_sound(1, halo::k_dword_none, 0);
        halo::interface::widget_close_all();
        halo::game::game_engine_send_end_game_notification(1);
    }
}

/**
 * Moves the engine to the ending state with a 7 second timer, queues the end-of-game sound and closes the
 * widgets.
 *
 * @address 0x4670c0
 */
void Lifecycle::end_game_sequence_stage1(void)
{
    game_engine_state_value = _game_engine_state_ending;
    game_engine_end_game_timer = 7.0f;
    halo::game::game_engine_queue_multiplayer_sound(1, halo::k_dword_none, 0);
    halo::interface::widget_close_all();
}

/**
 * Moves the engine to the ended state with a 5 second timer and sets bit 0x20 of the vitality flags of every
 * player unit.
 *
 * @address 0x4670f0
 */
void Lifecycle::end_game_sequence_stage2(void)
{
    data_iterator iterator;
    uint32_t unused_checksum;
    player *p;

    game_engine_post_game_fade = 0.0f;
    game_engine_state_value = _game_engine_state_ended;
    game_engine_end_game_timer = 5.0f;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)halo::k_dword_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    unused_checksum = (uint32_t)player_data ^ halo::game::k_iterator_signature_key;

    p = (player *)halo::memory::data_iterator_next(&iterator);
    while (p != (player *)0) {
        if (p->unit != (datum_index)halo::k_dword_none) {
            object *unit_obj = halo::game::object_at(p->unit);
            unit_obj->vitality_flags = unit_obj->vitality_flags | 0x0020;
        }
        p = (player *)halo::memory::data_iterator_next(&iterator);
    }

    (void)unused_checksum;
}

/**
 * Moves the engine to the post-game state and arms the dedicated server idle timer when it is configured.
 *
 * @address 0x467180
 */
void Lifecycle::end_game_sequence_stage3(void)
{
    game_engine_state_value = _game_engine_state_post_game;

    if (halo::game::fields::server_end_game_requested == 1) {
        game_engine_dedicated_idle = 0;
        game_engine_dedicated_idle_timer = 0.0f;
        return;
    }

    if (halo::game::fields::mapcycle_timeout > 0) {
        game_engine_dedicated_idle_timer = (float)halo::game::fields::mapcycle_timeout;
        game_engine_dedicated_idle = 1;
    }
}

/**
 * Returns whether the active game engine has team play enabled.
 *
 * @address 0x462bf0
 */
uint8_t Lifecycle::get_teams_enabled(void)
{
    if (current_game_engine != 0) {
        return game_engine_variant.teams;
    }
    return 0;
}

/**
 * Prepares the game engine for a new game: looks up the map table entry, clears the multiplayer sound queue
 * and runs the engine initialize callback.
 *
 * @address 0x45c370
 */
void Lifecycle::initialize_for_new_game(void)
{
    int32_t map_index;
    uint8_t initialize_result;

    if (current_game_engine != (game_engine_definition *)0) {
        map_index = halo::interface::map_list_find_known_map_index(network_build_string);
        game_engine_map_table_value = 0;
        if (map_index < 0x13) {
            game_engine_map_table_value = map_per_map_table[map_index].game_engine_value;
        }
        halo::game::game_engine_validate_scenario_placements_noop();

        memset(multiplayer_sound_queue, 0, sizeof(multiplayer_sound_queue));
        multiplayer_sound_queue[0].player = (datum_index)halo::k_dword_none;
        multiplayer_sound_queue[0].sound_index = -1;

        memset(custom_waypoints, 0, sizeof(custom_waypoints));

        multiplayer_sound_queue[0].remaining_ticks = 0x3c;
        multiplayer_sound_queue[0].broadcast = 0;
        memset(multiplayer_sound_queue[0].pad_0d, 0, sizeof(multiplayer_sound_queue[0].pad_0d));
        game_engine_auto_team_counter = 0;
        multiplayer_sound_queue_count = 1;
        game_engine_ctf_reset_ticks = 0;

        if (current_game_engine->initialize_for_new_game != nullptr) {
            initialize_result =
                ((uint8_t (*)(void))current_game_engine->initialize_for_new_game)();
            if (initialize_result == 0) {
                halo::game::game_engine_unload();
            }
        }
        halo::game::game_engine_touch_multiplayer_predicted_resources();
        game_engine_dedicated_idle = 0;
        game_engine_dedicated_idle_timer = 0.0f;
        server_end_game_requested = 0;
        if (network_session_host_state != 2) {
            network_session_host_state = 1;
        }
    }
}

/**
 * Reports whether the game engine is currently inactive (no game variant loaded, or not yet started).
 *
 * @address 0x461610
 */
uint8_t Lifecycle::is_inactive(void)
{
    return current_game_engine == 0 || game_engine_state_value == _game_engine_state_not_started;
}

/**
 * Invokes the postgame carnage-report rendering only while the game engine is in one of its end-of-game
 * display states.
 *
 * @address 0x461a80
 */
void Lifecycle::maybe_render_post_game(void)
{
    if (current_game_engine != 0 && 1 < game_engine_state_value) {
        halo::game::game_engine_post_rasterize_post_game();
        Rectangle2D viewport = {0, 0, 480, 640};
        halo::interface::widget_draw_split_screen_region(&viewport, 0);
    }
}

/**
 * Maps the state of the network server or client session to the multiplayer UI state id; returns 8 without a
 * session.
 *
 * @address 0x4655d0
 */
int32_t Lifecycle::multiplayer_ui_state_id(void)
{
    network_game_session *record;

    if (network_server != (network_server_globals *)0) {
        record = &network_server->session;
    } else if (network_client != nullptr) {
        record = &((network_client_globals *)network_client)->session;
    } else {
        return 8;
    }

    switch (record->variant.game_engine_index) {
    case 1:
        if (record->variant.engine.ctf.assault == 1) {
            return 0x1d - (record->variant.engine.ctf.single_flag_time != 0);
        }
        return (-(int32_t)(record->variant.engine.ctf.single_flag_time != 0) & 0x1b) + 3;
    case 2:
        return 4;
    case 3:
        if (record->variant.engine.oddball.ball_type == 1) {
            return 0x1f;
        }
        if (record->variant.engine.oddball.ball_type != 2) {
            return 5;
        }
        return 0x20;
    case 4:
        return 6;
    case 5:
        if (record->variant.engine.race.race_type != 2) {
            return 7;
        }
        return 0x21;
    default:
        return 8;
    }
}

}
