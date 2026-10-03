/**
 * @file include/halo/effects/vars.hpp
 * Addresses of the engine variables the effects module owns, by name. Other files bind a typed reference to the entry they use
 * (halo::link::ref<T>); the link names themselves appear only in standalone/data/link/effects_vars.hpp.
 */
#pragma once

namespace halo::effects {

/** Address table of the engine variables owned by the effects module. */
struct Vars {
    void *ambient_noise;
    void *camera_forward_x;
    void *camera_position_z;
    void *contrail_data;
    void *contrail_point_data;
    void *decal_clip_buffers;
    void *decal_data;
    void *decal_grid_block;
    void *decals_enabled;
    void *decals_for_all_responses;
    void *effect_data;
    void *effect_location_data;
    void *effect_marker_callback_context;
    void *effect_random_seed;
    void *first_person_effects_enabled;
    void *global_white_color;
    void *k_decal_type_parameters;
    void *k_render_identity_matrix_ptr;
    void *light_count_enabled;
    void *particle_creation_physics_table;
    void *particle_data;
    void *particle_impact_vector_names;
    void *particle_spawn_debug_mode;
    void *particle_system_data;
    void *particle_system_particle_data;
    void *particle_system_update_physics_table;
    void *particle_systems_enabled;
    void *particle_update_physics_table;
    void *player_effect_globals_pointer;
    void *player_effect_reentry_count;
    void *rasterizer_decal_vertex_cache;
    void *rasterizer_decal_vertex_cache_handle;
    void *screen_flash_pass;
    void *weather_enabled;
    void *weather_frame_counter;
    void *weather_instance_count;
    void *weather_instances;
    void *weather_particle_system_count;
    void *weather_wind_states;
};

/** The singleton address table; its storage is constant-initialised. */
const Vars &vars();

}  // namespace halo::effects
