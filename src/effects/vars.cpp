/**
 * @file src/effects/vars.cpp
 * Binds halo::effects::Vars to the engine variables the data image defines under their original link names.
 */

#include "halo/effects/vars.hpp"
#include "link/effects_vars.hpp"
#include "halo/effects/api.hpp"

namespace halo::effects {

const Vars &vars()
{
    static const Vars table{
        ambient_noise,
        camera_forward_x,
        camera_position_z,
        contrail_data,
        contrail_point_data,
        decal_clip_buffers,
        decal_data,
        decal_grid_block,
        decals_enabled,
        decals_for_all_responses,
        effect_data,
        effect_location_data,
        effect_marker_callback_context,
        effect_random_seed,
        first_person_effects_enabled,
        global_white_color,
        k_decal_type_parameters,
        k_render_identity_matrix_ptr,
        light_count_enabled,
        particle_creation_physics_table,
        particle_data,
        particle_impact_vector_names,
        particle_spawn_debug_mode,
        particle_system_data,
        particle_system_particle_data,
        particle_system_update_physics_table,
        particle_systems_enabled,
        particle_update_physics_table,
        player_effect_globals_pointer,
        player_effect_reentry_count,
        rasterizer_decal_vertex_cache,
        rasterizer_decal_vertex_cache_handle,
        screen_flash_pass,
        weather_enabled,
        weather_frame_counter,
        weather_instance_count,
        weather_instances,
        weather_particle_system_count,
        weather_wind_states,
    };
    return table;
}

}  // namespace halo::effects
