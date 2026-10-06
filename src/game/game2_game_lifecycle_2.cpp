#include "halo/game/lockstep.hpp"
#include "halo/math/constants.hpp"
#include "halo/game/game2_game_lifecycle.hpp"
#include "halo/networking/game_mode.hpp"
#include "halo/core/datum.hpp"
#include "halo/game/legacy_globals.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/structures/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/cutscene/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/main/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/hs/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/cutscene/vars.hpp"
#include "halo/effects/vars.hpp"
#include "halo/game/vars.hpp"
#include "halo/objects/vars.hpp"
#include "halo/physics/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/physics/api.hpp"
#include "halo/units/api.hpp"
#include "halo/platform/cpu.hpp"

typedef struct ai_update_stagger_state { int16_t threshold; int16_t highest; uint8_t claimed; } ai_update_stagger_state;

static auto &ai_update_stagger = halo::link::ref<ai_update_stagger_state *>(halo::units::vars().ai_update_stagger);
static auto &network_scenario_round_counter_a = halo::link::ref<int32_t>(halo::game::vars().network_scenario_round_counter_a);
static auto &network_scenario_round_counter_b = halo::link::ref<int32_t>(halo::game::vars().network_scenario_round_counter_b);
static auto &current_game_engine = halo::link::ref<game_engine_definition *>(halo::game::vars().current_game_engine);
static auto &player_profile_cache_initialized = halo::link::ref<uint8_t>(halo::game::vars().player_profile_cache_initialized);
static auto &player_profile_cache = halo::link::ref<uint32_t [0xc0]>(halo::game::vars().player_profile_cache);
static auto &game_engine_active_variant = halo::link::ref<game_variant>(halo::game::vars().game_engine_active_variant);
static auto &game_time = halo::link::ref<game_time_globals *>(halo::ai::vars().game_time);
static auto &unknown_00746280_block = halo::link::ref<uint32_t [0x343]>(halo::game::vars().unknown_00746280_block);
static auto &k_default_sound_environment = halo::link::ref<uint32_t [0x12]>(halo::game::vars().k_default_sound_environment);
static auto &object_render_state_cache = halo::link::ref<data_array *>(halo::game::vars().object_render_state_cache);
static auto &decal_grid_block = halo::link::ref<void *>(halo::effects::vars().decal_grid_block);
static auto &particle_system_data = halo::link::ref<void *>(halo::effects::vars().particle_system_data);
static auto &sound_class_gains = halo::link::ref<void *>(halo::game::vars().sound_class_gains);
static auto &weather_instances = halo::link::ref<int32_t>(halo::effects::vars().weather_instances);
static auto &weather_instance_count = halo::link::ref<int32_t>(halo::effects::vars().weather_instance_count);
static auto &weather_particle_data = halo::link::ref<data_array *>(halo::game::vars().weather_particle_data);
static auto &k_air_density = halo::link::ref<real>(halo::game::vars().k_air_density);
static auto &k_water_density = halo::link::ref<real>(halo::game::vars().k_water_density);
static auto &game_engine_attribute_enabled = halo::link::ref<uint32_t>(halo::game::vars().game_engine_attribute_enabled);
static auto &player_effect_globals_pointer = halo::link::ref<uint32_t *>(halo::effects::vars().player_effect_globals_pointer);
static auto &recorded_animations = halo::link::ref<void *>(halo::game::vars().recorded_animations);
static auto &cinematic_saved_music_gain = halo::link::ref<uint32_t>(halo::cutscene::vars().cinematic_saved_music_gain);
static auto &cinematic_globals_ptr = halo::link::ref<uint32_t *>(halo::game::vars().cinematic_globals_ptr);
static auto &object_globals_pointer = halo::link::ref<uint8_t *>(halo::objects::vars().object_globals_pointer);
static auto &rasterizer_globals_data = halo::link::ref<uint32_t>(halo::game::vars().rasterizer_globals_data);
static auto &player_data = halo::link::ref<data_array *>(halo::game::vars().player_data);
static auto &team_data = halo::link::ref<data_array *>(halo::game::vars().team_data);
static auto &text_localization_strings = halo::link::ref<uint32_t>(halo::game::vars().text_localization_strings);
static auto &global_scenario_index = halo::link::ref<uint32_t>(halo::game::vars().global_scenario_index);
static auto &global_structure_bsp_index = halo::link::ref<uint16_t>(halo::game::vars().global_structure_bsp_index);
static auto &global_structure_bsp = halo::link::ref<void *>(halo::ai::vars().global_structure_bsp);
static auto &global_structure_collision_bsp = halo::link::ref<void *>(halo::physics::vars().global_structure_collision_bsp);
static auto &global_collision_bsp = halo::link::ref<void *>(halo::physics::vars().global_collision_bsp);
static auto &global_globals = halo::link::ref<Globals *>(halo::game::vars().global_globals);

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
    fields::simulation_tick_in_progress = 1;
    halo::game::lockstep::tick_begin();
    halo::platform::fpu_control(0x9001f, 0xfffff);
    halo::game::game_engine_flag_local_player_units();
    halo::game::team_pair_overrides_tick();

    ai_update_stagger->threshold = ai_update_stagger->highest;
    ai_update_stagger->highest = 0;
    ai_update_stagger->claimed = 0;

    halo::ai::ai_tick_dispatcher();

    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_local ||
        halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
        halo::game::game_engine_players_update_server();
    } else if (halo::networking::globals().game_mode == halo::networking::k_game_mode_client) {
        halo::game::game_engine_players_update_client();
    }

    {
        float seconds_per_tick = (halo::main::globals().game_globals->players_are_double_speed == 0) ? halo::math::k_seconds_per_tick : 0.016666668f;
        halo::effects::effects_update_all(seconds_per_tick);
    }

    halo::effects::globals().player_effect_reentry_count = halo::effects::globals().player_effect_reentry_count + 1;
    halo::interface::first_person_weapon_interface_tick();
    halo::effects::globals().player_effect_reentry_count = halo::effects::globals().player_effect_reentry_count - 1;

    halo::game::game_engine_tick();
    halo::hs::hs_runtime_update();
    halo::cutscene::recorded_animations_update();
    halo::objects::objects_update();
    halo::game::main_switch_structure_bsp();
    halo::interface::hud_update_dispatch();
    halo::effects::player_effect_clear_dead_players();
    halo::game::lockstep::tick_end();
    halo::game::lockstep::probe_tick_end();

    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_host) {
        if (predict_pass == 0) {
            halo::game::players_server_catchup_on_client_updates();
        }
        halo::game::game_engine_server_update_player_positions();
        halo::networking::network_client_send_local_player_updates();
        halo::game::network_server_broadcast_object_type_changes();
        if (0 < network_scenario_round_counter_a) {
            halo::networking::network_event_feed_flush((int32_t *)(fields::network_event_feed_a));
        }
        if (0 < network_scenario_round_counter_b) {
            halo::networking::network_event_feed_flush((int32_t *)(fields::network_event_feed_b));
        }
    }
    if (halo::networking::globals().game_mode == halo::networking::k_game_mode_client) {
        halo::game::players_client_catchup_on_server_updates();
    }

    fields::simulation_tick_in_progress = 0;
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

    halo::math::globals().random_seed_global = halo::main::globals().game_globals->random_seed;
    halo::game::lockstep::on_new_map(halo::main::globals().game_globals->random_seed);

    if (current_game_engine != (game_engine_definition *)0) {
        if (current_game_engine->dispose != (void *)0) {
            ((void (*)(void))current_game_engine->dispose)();
        }
        current_game_engine = (game_engine_definition *)0;
    }
    if (player_profile_cache_initialized == 1) {
        memset(player_profile_cache, 0, sizeof(player_profile_cache));
        player_profile_cache_initialized = 0;
    }

    halo::game::game_engine_load_from_variant(&game_engine_active_variant);
    halo::platform::fpu_control(0x9001f, 0xfffff);
    halo::rasterizer::decal_and_font_system_reset();
    halo::saved_games::game_state_build_header();

    static_assert(sizeof(game_time_globals) == 8 * sizeof(uint32_t), "game time reset clears eight dwords");
    memset(game_time, 0, sizeof(*game_time));
    game_time->initialized = 1;

    halo::interface::interface_local_player_state_reset();
    halo::game::team_pair_table_init_defaults();
    halo::game::players_dispose();

    cursor = unknown_00746280_block;
    for (i = 0x343; i != 0; i = i - 1) {
        *cursor = 0;
        cursor = cursor + 1;
    }
    *(uint8_t *)unknown_00746280_block = 1;
    halo::effects::ambient_color_randomize();

    tag_cache_bytes = (uint8_t *)halo::scenario::globals().game_globals;
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

    halo::objects::objects_reset();
    object_render_state_cache->valid = 1;
    halo::memory::data_delete_all(object_render_state_cache);

    dst = (uint32_t *)halo::structures::globals().detail_objects;
    for (i = 0x290c; i != 0; i = i - 1) {
        *dst = 0;
        dst = dst + 1;
    }
    *((uint8_t *)halo::structures::globals().detail_objects + 0x520e) = 0;
    *(uint32_t *)halo::structures::globals().runtime_decals_suppressed = 0;
    halo::objects::breakable_surfaces_reset();

    dst = (uint32_t *)decal_grid_block;
    for (i = 0xa00; i != 0; i = i - 1) {
        *dst = 0xffffffff;
        dst = dst + 1;
    }
    dst[0] = halo::k_dword_none;
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
    ((data_array *)particle_system_data)->valid = 1;
    halo::memory::data_delete_all((data_array *)particle_system_data);
    halo::effects::globals().particle_system_particle_data->valid = 1;
    halo::memory::data_delete_all(halo::effects::globals().particle_system_particle_data);

    if (halo::sound::globals().disabled == 0) {
        ((data_array *)halo::sound::globals().sound_data)->valid = 1;
        halo::memory::data_delete_all((data_array *)halo::sound::globals().sound_data);
        ((data_array *)halo::sound::globals().looping_sound_data)->valid = 1;
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
        ((uint32_t *)halo::sound::globals().game_sound_state)[1] = halo::k_dword_none;
        ((uint32_t *)halo::sound::globals().game_sound_state)[0] = 0;
        ((uint32_t *)halo::sound::globals().game_sound_state)[2] = 0;
    }

    weather_instances = -1;
    weather_instance_count = 0;
    halo::effects::globals().weather_particle_data->valid = 1;
    halo::memory::data_delete_all(halo::effects::globals().weather_particle_data);

    k_air_density = fields::air_density_base * 118613.34f;
    k_water_density = fields::water_density_base * 118613.34f;

    halo::game::game_engine_initialize_for_new_game();
    game_engine_attribute_enabled = 1;
    halo::game::update_server_new();
    halo::game::game_engine_reset_player_look_state();

    dst = player_effect_globals_pointer;
    for (i = 0x4a; i != 0; i = i - 1) {
        *dst = 0;
        dst = dst + 1;
    }
    *(uint16_t *)((uint8_t *)player_effect_globals_pointer + 0x3f * 4) = 0xffff;
    player_effect_globals_pointer[0x49] = ((uint32_t *)game_time)[3];
    halo::ai::ai_reset_for_new_map();

    dst = cinematic_globals_ptr;
    dst[0] = 0;
    dst[1] = 0;
    dst[2] = 0;
    dst[3] = halo::k_dword_none;
    dst[4] = halo::k_dword_none;
    dst[5] = halo::k_dword_none;
    dst[6] = halo::k_dword_none;

    cinematic_saved_music_gain = 0xbf800000;
    halo::hs::hs_scripts_reload();
    *((uint8_t *)recorded_animations + 0x24) = 1;
    halo::memory::data_delete_all((data_array *)recorded_animations);

    halo::main::globals().game_globals->active = 1;
    *object_globals_pointer = 1;
    halo::objects::scenario_objects_place((uint8_t *)(halo::scenario::globals().scenario));
    *object_globals_pointer = 0;
    halo::ai::encounters_spawn_initial();
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

    halo::rasterizer::font_glyph_cache_clear_all();
    rasterizer_globals_data = 0;
    ((data_array *)recorded_animations)->valid = 0;
    halo::hs::hs_scripts_free();

    ((uint8_t *)cinematic_globals_ptr)[8] = 0;
    ((uint8_t *)cinematic_globals_ptr)[9] = 0;
    halo::ai::globals().conversation_data->valid = 0;
    halo::ai::globals().encounter_data->valid = 0;
    halo::ai::globals().pursuit_data->valid = 0;
    halo::ai::globals().prop_data->valid = 0;
    halo::ai::globals().actor_data->valid = 0;
    halo::ai::globals().swarm_data->valid = 0;
    halo::ai::globals().swarm_component_data->valid = 0;
    halo::ai::globals().state->actors_valid = 0;
    halo::effects::particle_systems_delete_all();

    if (halo::effects::globals().weather_particle_data->valid != 0) {
        halo::effects::globals().weather_particle_data->valid = 0;
    }
    if (halo::rasterizer::globals().decal_vertex_cache_handle != 0) {
        halo::effects::decal_clear_flags(1);
        halo::memory::cache_flush((::cache *)halo::rasterizer::globals().decal_vertex_cache_handle);
    }
    halo::effects::globals().decal_data->valid = 0;
    if (object_render_state_cache != (data_array *)0 && object_render_state_cache->valid != 0) {
        object_render_state_cache->valid = 0;
    }
    halo::objects::objects_flush_dirty_state();

    had_network_predicted_globals = halo::sound::globals().game_looping_sound_data != (data_array *)0;
    halo::camera::globals().directors[0].pov_proc = 0;
    halo::camera::globals().directors[0].look_scale = 1.0f;
    halo::camera::globals().directors[0].unknown_c0 = 0;
    *halo::camera::globals().hs_camera_control_pointer = 0;
    text_localization_strings = halo::k_dword_none;
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
    halo::game::update_queues_dispose();

    if (current_game_engine != (game_engine_definition *)0 && current_game_engine->dispose_from_old_game != (void *)0) {
        ((void (*)(void))current_game_engine->dispose_from_old_game)();
    }

    *(uint8_t *)unknown_00746280_block = 0;
    if (game_time != (game_time_globals *)0) {
        ((uint8_t *)game_time)[0] = 0;
        ((uint8_t *)game_time)[1] = 0;
    }

    halo::interface::widget_close_all();
    halo::main::globals().game_globals->active = 0;
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
        halo::main::globals().game_globals->map_loading_in_progress = 1;
        do {
            status = halo::cache::cache_file_download_status_get(&halo::main::globals().game_globals->map_load_progress, 0);
            halo::main::render_pregame_view_initialize();
            halo::main::movie_capture_frame_export();
        } while (status == 0);
        halo::interface::widget_close_all();
        if (status == 2) {
            halo::interface::interface_handle_quit_request();
        }
        halo::cache::cache_file_download_finish();
    }
    if (halo::main::globals().game_globals->map_loaded != 0) {
        halo::cache::cache_file_unload();
        halo::scenario::globals().game_globals->structure_bsp_index = -1;
        global_scenario_index = halo::k_dword_none;
        global_structure_bsp_index = halo::k_word_none;
        halo::scenario::globals().scenario = (Scenario *)0;
        global_structure_bsp = (void *)0;
        global_structure_collision_bsp = (void *)0;
        global_collision_bsp = (void *)0;
        global_globals = (::Globals *)0;
        halo::main::globals().game_globals->map_loaded = 0;
    }
}

}
