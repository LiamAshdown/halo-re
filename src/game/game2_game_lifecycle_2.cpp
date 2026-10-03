#include "halo/game/game2_game_lifecycle.hpp"
#include "halo/game/legacy_globals.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/structures/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/cutscene/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/saved_games/api.hpp"

typedef struct ai_update_stagger_state { int16_t threshold; int16_t highest; uint8_t claimed; } ai_update_stagger_state;

extern "C" {
extern ai_update_stagger_state *ai_update_stagger;
extern int16_t network_game_mode;
extern game_main_globals *main_game_globals;
extern int32_t network_scenario_round_counter_a;
extern int32_t network_scenario_round_counter_b;
extern void game_engine_flag_local_player_units(void);
extern void team_pair_overrides_tick(void);
extern void game_engine_tick(void);
extern void hs_runtime_update(void);
extern void ai_tick_dispatcher(void);
extern void game_engine_players_update_server(void);
extern void game_engine_players_update_client(void);
extern void main_switch_structure_bsp(void);
extern void game_engine_server_update_player_positions(void);
extern void players_server_catchup_on_client_updates(void);
extern void players_client_catchup_on_server_updates(void);
extern void first_person_weapon_interface_tick(void);
extern void hud_update_dispatch(void);
extern void network_client_send_local_player_updates(void);
extern void network_event_feed_flush(void *queue);
extern void objects_update(void);
extern void network_server_broadcast_object_type_changes(void);
extern game_engine_definition *current_game_engine;
extern uint8_t player_profile_cache_initialized;
extern uint32_t player_profile_cache[0xc0];
extern game_variant game_engine_active_variant;
extern game_time_globals *game_time;
extern uint32_t unknown_00746280_block[0x343];
extern scenario_game_globals *global_scenario_game_globals;
extern uint32_t k_default_sound_environment[0x12];
extern data_array *object_render_state_cache;
extern void *decal_grid_block;
extern void *particle_system_data;
extern void *sound_class_gains;
extern int32_t weather_instances;
extern int32_t weather_instance_count;
extern data_array *weather_particle_data;
extern real k_air_density;
extern real k_water_density;
extern uint32_t game_engine_attribute_enabled;
extern uint32_t *player_effect_globals_pointer;
extern void *recorded_animations;
extern uint32_t cinematic_saved_music_gain;
extern uint32_t *cinematic_globals_ptr;
extern Scenario *global_scenario;
extern uint8_t *object_globals_pointer;
extern void ai_reset_for_new_map(void);
extern void encounters_spawn_initial(void);
extern void team_pair_table_init_defaults(void);
extern void game_engine_load_from_variant(const game_variant *variant);
extern void game_engine_initialize_for_new_game(void);
extern void game_engine_reset_player_look_state(void);
extern uint8_t update_server_new(void);
extern void players_dispose(void);
extern void hs_scripts_reload(void);
extern void interface_local_player_state_reset(void);
extern void scenario_objects_place(Scenario *scenario);
extern void objects_reset(void);
extern void breakable_surfaces_reset(void);
extern void decal_and_font_system_reset(void);
extern uint32_t rasterizer_globals_data;
extern data_array *ai_conversation_data;
extern data_array *encounter_data;
extern data_array *ai_pursuit_data;
extern data_array *prop_data;
extern data_array *actor_data;
extern data_array *swarm_data;
extern data_array *swarm_component_data;
extern ai_globals *ai_globals_ptr;
extern uint32_t rasterizer_decal_vertex_cache_handle;
extern data_array *player_data;
extern data_array *team_data;
extern uint32_t text_localization_strings;
extern void update_queues_dispose(void);
extern void hs_scripts_free(void);
extern void objects_flush_dirty_state(void);
extern void font_glyph_cache_clear_all(void);
extern void widget_close_all(void);
extern uint32_t global_scenario_index;
extern uint16_t global_structure_bsp_index;
extern void *global_structure_bsp;
extern void *global_structure_collision_bsp;
extern void *global_collision_bsp;
extern Globals *global_globals;
extern void render_pregame_view_initialize(void);
extern void movie_capture_frame_export(void);
extern void interface_handle_quit_request(void);
}

namespace halo::game {

/**
 * The per-frame simulation driver: resets the AI update-stagger record, sets the FPU control word, ticks team-
 * pair overrides and local-player flags, advances the network/host update path for the current connection
 * role, sets a tick-length constant (halved in slow motion), then runs the main tick sequence (game engine, hs
 * scripts, devices, objects, structure-BSP switch) and, while hosting, the outbound network catch-up ...
 *
 * @address 0x45b780
 */
void GameLifecycle::simulate_tick(uint32_t predict_pass)
{
    globals::simulation_tick_in_progress = 1;
    _control87(0x9001f, 0xfffff);
    game_engine_flag_local_player_units();
    team_pair_overrides_tick();

    ai_update_stagger->threshold = ai_update_stagger->highest;
    ai_update_stagger->highest = 0;
    ai_update_stagger->claimed = 0;

    ai_tick_dispatcher();

    if (network_game_mode != 0) {
        if (network_game_mode == 1) {
            game_engine_players_update_client();
            goto after_role_update;
        }
        if (network_game_mode != 2) {
            goto after_role_update;
        }
    }
    game_engine_players_update_server();

after_role_update:
    {
        float seconds_per_tick = (main_game_globals->players_are_double_speed == 0) ? 0.033333335f : 0.016666668f;
        halo::effects::effects_update_all(seconds_per_tick);
    }

    halo::effects::globals().player_effect_reentry_count = halo::effects::globals().player_effect_reentry_count + 1;
    first_person_weapon_interface_tick();
    halo::effects::globals().player_effect_reentry_count = halo::effects::globals().player_effect_reentry_count - 1;

    game_engine_tick();
    hs_runtime_update();
    halo::cutscene::recorded_animations_update();
    objects_update();
    main_switch_structure_bsp();
    hud_update_dispatch();
    halo::effects::player_effect_clear_dead_players();

    if (network_game_mode == 2) {
        if (predict_pass == 0) {
            players_server_catchup_on_client_updates();
        }
        game_engine_server_update_player_positions();
        network_client_send_local_player_updates();
        network_server_broadcast_object_type_changes();
        if (0 < network_scenario_round_counter_a) {
            network_event_feed_flush(globals::network_event_feed_a);
        }
        if (0 < network_scenario_round_counter_b) {
            network_event_feed_flush(globals::network_event_feed_b);
        }
    }
    if (network_game_mode == 1) {
        players_client_catchup_on_server_updates();
    }

    globals::simulation_tick_in_progress = 0;
}

/**
 * Resets game state (objects, scripts, particle/effect pools, network server) to begin a new game on the
 * currently loaded map.
 *
 * @address 0x45b050
 */
void GameLifecycle::start_new_map(void)
{
    uint32_t i;
    uint8_t *tag_cache_bytes;
    uint32_t *cursor;
    uint32_t *dst;
    uint8_t *record;

    halo::math::globals().random_seed_global = main_game_globals->random_seed;

    if (current_game_engine != (game_engine_definition *)0) {
        if (current_game_engine->dispose != (void *)0) {
            ((void (*)(void))current_game_engine->dispose)();
        }
        current_game_engine = (game_engine_definition *)0;
    }
    if (player_profile_cache_initialized == 1) {
        for (i = 0; i < 0xc0; i = i + 1) {
            player_profile_cache[i] = 0;
        }
        player_profile_cache_initialized = 0;
    }

    game_engine_load_from_variant(&game_engine_active_variant);
    _control87(0x9001f, 0xfffff);
    decal_and_font_system_reset();
    halo::saved_games::game_state_build_header();

    {
        uint32_t *game_time_dwords = (uint32_t *)game_time;
        for (i = 0; i < 8; i = i + 1) {
            game_time_dwords[i] = 0;
        }
    }
    ((uint8_t *)game_time)[0] = 1;

    interface_local_player_state_reset();
    team_pair_table_init_defaults();
    players_dispose();

    cursor = unknown_00746280_block;
    for (i = 0x343; i != 0; i = i - 1) {
        *cursor = 0;
        cursor = cursor + 1;
    }
    *(uint8_t *)unknown_00746280_block = 1;
    halo::effects::ambient_color_randomize();

    tag_cache_bytes = (uint8_t *)global_scenario_game_globals;
    cursor = (uint32_t *)tag_cache_bytes;
    for (i = 1; i <= 0xb; i = i + 1) {
        cursor[i] = 0;
    }
    dst = (uint32_t *)tag_cache_bytes + 0xd;
    for (i = 0x12; i != 0; i = i - 1) {
        *dst = k_default_sound_environment[0x12 - i];
        dst = dst + 1;
    }
    *((uint8_t *)((uint32_t *)tag_cache_bytes + 0xc)) = 0;

    objects_reset();
    object_render_state_cache->valid = 1;
    halo::memory::data_delete_all(object_render_state_cache);

    dst = (uint32_t *)halo::structures::globals().detail_objects;
    for (i = 0x290c; i != 0; i = i - 1) {
        *dst = 0;
        dst = dst + 1;
    }
    *((uint8_t *)halo::structures::globals().detail_objects + 0x520e) = 0;
    *(uint32_t *)halo::structures::globals().runtime_decals_suppressed = 0;
    breakable_surfaces_reset();

    dst = (uint32_t *)decal_grid_block;
    for (i = 0xa00; i != 0; i = i - 1) {
        *dst = 0xffffffff;
        dst = dst + 1;
    }
    dst[0] = 0xffffffff;
    dst[1] = 0;
    dst[2] = 0;
    halo::effects::globals().decal_data->valid = 1;
    halo::memory::data_delete_all(halo::effects::globals().decal_data);

    halo::camera::camera_initialize();
    halo::camera::observer_new(&halo::camera::globals().observers[0]);

    halo::effects::globals().contrail_data->valid = 1;
    halo::memory::data_delete_all(halo::effects::globals().contrail_data);
    halo::effects::globals().contrail_point_data->valid = 1;
    halo::memory::data_delete_all(halo::effects::globals().contrail_point_data);
    halo::effects::globals().particle_data->valid = 1;
    halo::memory::data_delete_all(halo::effects::globals().particle_data);
    halo::effects::globals().effect_data->valid = 1;
    halo::memory::data_delete_all(halo::effects::globals().effect_data);
    halo::effects::globals().effect_location_data->valid = 1;
    halo::memory::data_delete_all(halo::effects::globals().effect_location_data);
    *((uint8_t *)particle_system_data + 0x24) = 1;
    halo::memory::data_delete_all((data_array *)particle_system_data);
    halo::effects::globals().particle_system_particle_data->valid = 1;
    halo::memory::data_delete_all(halo::effects::globals().particle_system_particle_data);

    if (halo::sound::globals().disabled == 0) {
        *((uint8_t *)halo::sound::globals().sound_data + 0x24) = 1;
        halo::memory::data_delete_all((data_array *)halo::sound::globals().sound_data);
        *((uint8_t *)halo::sound::globals().looping_sound_data + 0x24) = 1;
        halo::memory::data_delete_all((data_array *)halo::sound::globals().looping_sound_data);
    }

    record = (uint8_t *)sound_class_gains + 8;
    i = 0x33;
    do {
        *(uint32_t *)(record - 4) = 0x3f800000;
        *(uint32_t *)(record - 8) = 0x3f800000;
        *(uint16_t *)record = 0;
        record = record + 0xc;
        i = i - 1;
    } while (i != 0);

    if (halo::sound::globals().game_looping_sound_data != (data_array *)0) {
        halo::sound::globals().game_looping_sound_data->valid = 1;
        halo::memory::data_delete_all(halo::sound::globals().game_looping_sound_data);
        ((uint32_t *)halo::sound::globals().game_sound_state)[1] = 0xffffffff;
        ((uint32_t *)halo::sound::globals().game_sound_state)[0] = 0;
        ((uint32_t *)halo::sound::globals().game_sound_state)[2] = 0;
    }

    weather_instances = -1;
    weather_instance_count = 0;
    halo::effects::globals().weather_particle_data->valid = 1;
    halo::memory::data_delete_all(halo::effects::globals().weather_particle_data);

    k_air_density = globals::air_density_base * 118613.34f;
    k_water_density = globals::water_density_base * 118613.34f;

    game_engine_initialize_for_new_game();
    game_engine_attribute_enabled = 1;
    update_server_new();
    game_engine_reset_player_look_state();

    dst = player_effect_globals_pointer;
    for (i = 0x4a; i != 0; i = i - 1) {
        *dst = 0;
        dst = dst + 1;
    }
    *(uint16_t *)((uint8_t *)player_effect_globals_pointer + 0x3f * 4) = 0xffff;
    player_effect_globals_pointer[0x49] = ((uint32_t *)game_time)[3];
    ai_reset_for_new_map();

    dst = cinematic_globals_ptr;
    dst[0] = 0;
    dst[1] = 0;
    dst[2] = 0;
    dst[3] = 0xffffffff;
    dst[4] = 0xffffffff;
    dst[5] = 0xffffffff;
    dst[6] = 0xffffffff;

    cinematic_saved_music_gain = 0xbf800000;
    hs_scripts_reload();
    *((uint8_t *)recorded_animations + 0x24) = 1;
    halo::memory::data_delete_all((data_array *)recorded_animations);

    main_game_globals->active = 1;
    *object_globals_pointer = 1;
    scenario_objects_place(global_scenario);
    *object_globals_pointer = 0;
    encounters_spawn_initial();
}

/**
 * Shuts down the currently running game (frees scripts, flushes caches, reverts modded state) as the
 * counterpart to game_start_new_map.
 *
 * @address 0x45b370
 */
void GameLifecycle::stop_current_map(void)
{
    uint8_t had_network_predicted_globals;

    font_glyph_cache_clear_all();
    rasterizer_globals_data = 0;
    ((data_array *)recorded_animations)->valid = 0;
    hs_scripts_free();

    ((uint8_t *)cinematic_globals_ptr)[8] = 0;
    ((uint8_t *)cinematic_globals_ptr)[9] = 0;
    ai_conversation_data->valid = 0;
    encounter_data->valid = 0;
    ai_pursuit_data->valid = 0;
    prop_data->valid = 0;
    actor_data->valid = 0;
    swarm_data->valid = 0;
    swarm_component_data->valid = 0;
    ai_globals_ptr->actors_valid = 0;
    halo::effects::particle_systems_delete_all();

    if (halo::effects::globals().weather_particle_data->valid != 0) {
        halo::effects::globals().weather_particle_data->valid = 0;
    }
    if (rasterizer_decal_vertex_cache_handle != 0) {
        halo::effects::decal_clear_flags(1);
        halo::memory::cache_flush((::cache *)rasterizer_decal_vertex_cache_handle);
    }
    halo::effects::globals().decal_data->valid = 0;
    if (object_render_state_cache != (data_array *)0 && object_render_state_cache->valid != 0) {
        object_render_state_cache->valid = 0;
    }
    objects_flush_dirty_state();

    had_network_predicted_globals = halo::sound::globals().game_looping_sound_data != (data_array *)0;
    halo::camera::globals().directors[0].pov_proc = 0;
    halo::camera::globals().directors[0].look_scale = 1.0f;
    halo::camera::globals().directors[0].unknown_c0 = 0;
    *halo::camera::globals().hs_camera_control_pointer = 0;
    text_localization_strings = 0xffffffff;
    player_data->valid = 0;
    team_data->valid = 0;
    halo::effects::globals().contrail_point_data->valid = 0;
    halo::effects::globals().contrail_data->valid = 0;
    halo::effects::globals().particle_data->valid = 0;
    halo::effects::globals().effect_data->valid = 0;
    halo::effects::globals().effect_location_data->valid = 0;

    if (had_network_predicted_globals && halo::sound::globals().game_looping_sound_data->valid != 0) {
        halo::sound::game_sound_revert_scripting_sounds();
        halo::sound::globals().game_looping_sound_data->valid = 0;
    }

    halo::sound::sound_fade_out_and_stop_all();
    update_queues_dispose();

    if (current_game_engine != (game_engine_definition *)0 && current_game_engine->dispose_from_old_game != (void *)0) {
        ((void (*)(void))current_game_engine->dispose_from_old_game)();
    }

    *(uint8_t *)unknown_00746280_block = 0;
    if (game_time != (game_time_globals *)0) {
        ((uint8_t *)game_time)[0] = 0;
        ((uint8_t *)game_time)[1] = 0;
    }

    widget_close_all();
    main_game_globals->active = 0;
}

/**
 * Spins pumping the download/movie-export loop until any pending cache download finishes, then unloads the
 * current map cache file and resets the tag-index globals to their unloaded state.
 *
 * @address 0x45afb0
 */
void GameLifecycle::unload_map(void)
{
    int16_t status;

    if (halo::cache::globals().map_download_in_progress != 0) {
        main_game_globals->map_loading_in_progress = 1;
        do {
            status = halo::cache::cache_file_download_status_get(&main_game_globals->map_load_progress, 0);
            render_pregame_view_initialize();
            movie_capture_frame_export();
        } while (status == 0);
        widget_close_all();
        if (status == 2) {
            interface_handle_quit_request();
        }
        halo::cache::cache_file_download_finish();
    }
    if (main_game_globals->map_loaded != 0) {
        halo::cache::cache_file_unload();
        global_scenario_game_globals->structure_bsp_index = -1;
        global_scenario_index = 0xffffffff;
        global_structure_bsp_index = 0xffff;
        global_scenario = (Scenario *)0;
        global_structure_bsp = (void *)0;
        global_structure_collision_bsp = (void *)0;
        global_collision_bsp = (void *)0;
        global_globals = (Globals *)0;
        main_game_globals->map_loaded = 0;
    }
}

}
