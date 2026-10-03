/**
 * @file standalone/data/link/effects_vars.hpp
 * Link names of the engine variables owned by the effects module (halo::effects::vars()). The data image defines them under these C
 * names; only src/effects/vars.cpp includes this header, so the names are declared as untyped storage.
 */
#pragma once

extern "C" {
extern char ambient_noise[];
extern char camera_forward_x[];
extern char camera_position_z[];
extern char contrail_data[];
extern char contrail_point_data[];
extern char decal_clip_buffers[];
extern char decal_data[];
extern char decal_grid_block[];
extern char decals_enabled[];
extern char decals_for_all_responses[];
extern char effect_data[];
extern char effect_location_data[];
extern char effect_marker_callback_context[];
extern char effect_random_seed[];
extern char first_person_effects_enabled[];
extern char global_white_color[];
extern char k_decal_type_parameters[];
extern char k_render_identity_matrix_ptr[];
extern char light_count_enabled[];
extern char particle_creation_physics_table[];
extern char particle_data[];
extern char particle_impact_vector_names[];
extern char particle_spawn_debug_mode[];
extern char particle_system_data[];
extern char particle_system_particle_data[];
extern char particle_system_update_physics_table[];
extern char particle_systems_enabled[];
extern char particle_update_physics_table[];
extern char player_effect_globals_pointer[];
extern char player_effect_reentry_count[];
extern char rasterizer_decal_vertex_cache[];
extern char rasterizer_decal_vertex_cache_handle[];
extern char screen_flash_pass[];
extern char weather_enabled[];
extern char weather_frame_counter[];
extern char weather_instance_count[];
extern char weather_instances[];
extern char weather_particle_system_count[];
extern char weather_wind_states[];
}
