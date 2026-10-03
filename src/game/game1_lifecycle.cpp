/**
 * Game engine lifetime: new game set-up, end-game sequence stages and per-frame effects.
 */

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include <stdint.h>

#include "halo/game/game1_lifecycle.hpp"
#include "halo/memory/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"

extern "C" {
extern game_engine_definition *current_game_engine;
extern uint8_t player_profile_cache_initialized;
extern player_profile player_profile_cache[16];
extern void *weather_particle_data;
extern uint32_t effect_data;
extern uint32_t effect_location_data;
extern uint32_t particle_data;
extern data_array *player_data;
extern data_array *team_data;
extern player_globals *local_player_globals;
extern uint8_t *widget_memory_pool;
extern uint32_t ui_root_widget[13];
extern uint32_t sound_class_gains;
extern uint32_t rasterizer_device;
extern void **rasterizer_decal_vertex_cache;
extern uint32_t object_render_state_cache;
extern uint8_t console_win32_attached;
extern uint32_t *terminal_messages;
extern uint8_t terminal_initialized;
extern void *game_state_write_buffer;
extern uint32_t input_event_queue_active[0x43];
extern uint32_t input_globals[0x97c];
extern uint32_t profile_globals_block[0x1829];
extern uint8_t game_state_write_buffer_allocated;
extern void *game_state_persistent_storage;
extern uint8_t game_state_persistent_storage_created;
extern void hs_dispose_dynamic_globals(void);
extern void widget_close_all(void);
extern void saved_game_files_dispose(void);
extern void network_shutdown(void);
extern data_array *object_data;
extern uint8_t ai_scan_for_recent_combat_activity(uint32_t param);
extern void player_respawn(datum_index player_handle);
extern uint8_t player_attach_unit_to_parent(datum_index player_handle, datum_index parent_object,
                             void *local_offset);
extern int16_t network_game_mode;
extern game_engine_state game_engine_state_value;
extern uint8_t *network_server;
extern float game_engine_end_game_timer;
extern void game_engine_send_end_game_notification(uint32_t reason);
extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast);
extern float game_engine_post_game_fade;
extern uint8_t game_engine_dedicated_idle;
extern float game_engine_dedicated_idle_timer;
extern uint8_t g_006f1d25;
extern int32_t g_006f1d28;
extern game_variant game_engine_variant;
extern int32_t game_engine_map_table_value;
extern uint8_t map_per_map_table[];
extern uint8_t network_session_host_state;
extern multiplayer_sound_request multiplayer_sound_queue[5];
extern int32_t multiplayer_sound_queue_count;
extern custom_waypoint custom_waypoints[k_maximum_custom_waypoints];
extern int32_t game_engine_auto_team_counter;
extern int32_t game_engine_ctf_reset_ticks;
extern void game_engine_unload(void);
extern void game_engine_validate_scenario_placements_noop(void);
extern void game_engine_touch_multiplayer_predicted_resources(void);
extern int32_t map_list_find_known_map_index(void);
extern void game_engine_post_rasterize_post_game(void);
extern void widget_draw_split_screen_region(void);
extern uint8_t *network_client;
}

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
    uint32_t *cursor;

    hs_dispose_dynamic_globals();
    widget_close_all();
    if (*(void **)(widget_memory_pool + 4) != (void *)0) {
        GlobalFree(*(void **)(widget_memory_pool + 4));
    }
    *(uint32_t *)(widget_memory_pool + 4) = 0;
    *(uint32_t *)(widget_memory_pool + 8) = 0;

    for (i = 0; i < 13; i = i + 1) {
        ui_root_widget[i] = 0;
    }
    halo::sound::globals().game_looping_sound_data = (data_array *)0;
    sound_class_gains = 0;

    if (current_game_engine != (game_engine_definition *)0) {
        if (current_game_engine->dispose != (void *)0) {
            ((void (*)(void))current_game_engine->dispose)();
        }
        current_game_engine = (game_engine_definition *)0;
    }

    if (player_profile_cache_initialized == 1) {
        cursor = (uint32_t *)player_profile_cache;
        for (i = 0; i < 0xc0; i = i + 1) {
            cursor[i] = 0;
        }
        player_profile_cache_initialized = 0;
    }

    if (weather_particle_data != (void *)0) {
        cursor = (uint32_t *)weather_particle_data;
        for (i = 0; i < 14; i = i + 1) {
            cursor[i] = 0;
        }
        GlobalFree(weather_particle_data);
        weather_particle_data = (void *)0;
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

    if (rasterizer_device != 0 && rasterizer_decal_vertex_cache != (void **)0) {
        ((void (__stdcall *)(void **))(*(void ***)((uint8_t *)*rasterizer_decal_vertex_cache + 8)))(rasterizer_decal_vertex_cache);
        rasterizer_decal_vertex_cache = (void **)0;
    }
    object_render_state_cache = 0;
    halo::objects::objects_dispose();

    if (console_win32_attached != 0) {
        console_win32_attached = 0;
    }
    if (terminal_messages != (uint32_t *)0) {
        if (*((uint8_t *)terminal_messages + 9 * 4) != 0) {
            *((uint8_t *)terminal_messages + 9 * 4) = 0;
        }
        for (i = 0; i < 14; i = i + 1) {
            terminal_messages[i] = 0;
        }
        GlobalFree(terminal_messages);
    }
    terminal_initialized = 0;
    saved_game_files_dispose();

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

    network_shutdown();
}

/**
 * After a structure-BSP switch, attempts to give every player without a unit a valid unit and attach it to the
 * new BSP's parent object, retrying as needed.
 *
 * Original register convention: stack -> param; UNSURE purpose.
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
        if (local_player_globals->teleported == 0 && ai_scan_for_recent_combat_activity(1) != 0) {
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
                    next = ((object_header *)object_data->data)[walk & 0xffff].data->parent_object;
                    walk = next;
                } while (next != (datum_index)-1);

                if (root == unit_handle) {
                    root_obj = halo::objects::object_try_and_get(unit_handle, _object_mask_biped);
                    if (root_obj != (object *)0) {
                        biped = (biped_data *)((uint8_t *)root_obj + 0x4cc);
                        if ((biped->flags & 1) != 0) {
                            local_player_globals->mode = 3;
                            root = best_root;
                        }
                    }
                } else {
                    root_obj = halo::objects::object_try_and_get(root, _object_mask_vehicle);
                    if (root_obj != (object *)0) {
                        vehicle = (vehicle_data *)((uint8_t *)root_obj + 0x4cc);
                        if (vehicle->airborne_ticks != 0) {
                            local_player_globals->mode = 3;
                            root = best_root;
                        }
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
                    player_respawn(player_handle);
                    if (plr->unit == (datum_index)-1) {
                        success = 0;
                    } else {
                        root_obj = ((object_header *)object_data->data)[best_root & 0xffff].data;
                        success = player_attach_unit_to_parent(player_handle, best_root, (uint8_t *)root_obj + 0xa0);
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
    if (network_game_mode == 2 && game_engine_state_value == _game_engine_state_not_started) {
        *((uint8_t *)network_server + 0xa0f) = 1;
        game_engine_state_value = _game_engine_state_ending;
        game_engine_end_game_timer = 7.0f;
        game_engine_queue_multiplayer_sound(1, 0xffffffff, 0);
        widget_close_all();
        game_engine_send_end_game_notification(1);
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
    game_engine_queue_multiplayer_sound(1, 0xffffffff, 0);
    widget_close_all();
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
    iterator.index = (datum_index)0xffffffff;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    unused_checksum = (uint32_t)player_data ^ 0x69746572;

    p = (player *)halo::memory::data_iterator_next(&iterator);
    while (p != (player *)0) {
        if (p->unit != (datum_index)0xffffffff) {
            object *unit_obj = ((object_header *)object_data->data)[p->unit & 0xffff].data;
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

    if (g_006f1d25 == 1) {
        game_engine_dedicated_idle = 0;
        game_engine_dedicated_idle_timer = 0.0f;
        return;
    }

    if (g_006f1d28 > 0) {
        game_engine_dedicated_idle_timer = (float)g_006f1d28;
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
    uint32_t *dst;
    int32_t i;
    uint8_t initialize_result;

    if (current_game_engine != (game_engine_definition *)0) {
        map_index = map_list_find_known_map_index();
        game_engine_map_table_value = 0;
        if (map_index < 0x13) {
            game_engine_map_table_value = *(int32_t *)(map_per_map_table + map_index * 0x30);
        }
        game_engine_validate_scenario_placements_noop();

        dst = (uint32_t *)multiplayer_sound_queue;
        for (i = 0x14; i != 0; i = i - 1) {
            *dst = 0;
            dst = dst + 1;
        }
        multiplayer_sound_queue[0].player = (datum_index)0xffffffff;
        multiplayer_sound_queue[0].sound_index = -1;

        dst = (uint32_t *)custom_waypoints;
        for (i = 0x100; i != 0; i = i - 1) {
            *dst = 0;
            dst = dst + 1;
        }

        multiplayer_sound_queue[0].remaining_ticks = 0x3c;
        *(uint32_t *)&multiplayer_sound_queue[0].broadcast = 0;
        game_engine_auto_team_counter = 0;
        multiplayer_sound_queue_count = 1;
        game_engine_ctf_reset_ticks = 0;

        if (current_game_engine->initialize_for_new_game != (void *)0) {
            initialize_result =
                ((uint8_t (*)(void))current_game_engine->initialize_for_new_game)();
            if (initialize_result == 0) {
                game_engine_unload();
            }
        }
        game_engine_touch_multiplayer_predicted_resources();
        game_engine_dedicated_idle = 0;
        game_engine_dedicated_idle_timer = 0.0f;
        ((uint8_t *)&game_engine_map_table_value)[1] = 0;
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
        game_engine_post_rasterize_post_game();
        widget_draw_split_screen_region();
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
    uint8_t *record;

    if (network_server != (uint8_t *)0) {
        record = network_server + 8;
    } else if (network_client != (uint8_t *)0) {
        record = network_client + 0xb14;
    } else {
        return 8;
    }

    if (record == (uint8_t *)0) {
        return 8;
    }

    switch (*(int32_t *)(record + 0x134)) {
    case 1:
        if (*(uint8_t *)(record + 0x180) == 1) {
            return 0x1d - (*(int32_t *)(record + 0x184) != 0);
        }
        return (-(int32_t)(*(int32_t *)(record + 0x184) != 0) & 0x1b) + 3;
    case 2:
        return 4;
    case 3:
        if (*(int32_t *)(record + 400) == 1) {
            return 0x1f;
        }
        if (*(int32_t *)(record + 400) != 2) {
            return 5;
        }
        return 0x20;
    case 4:
        return 6;
    case 5:
        if (*(int32_t *)(record + 0x180) != 2) {
            return 7;
        }
        return 0x21;
    default:
        return 8;
    }
}

}
