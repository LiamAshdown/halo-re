/**
 * @file include/halo/effects/api.hpp
 * Functions of the effects module that other modules and the data tables call (namespace halo::effects). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdint.h>

struct data_array;
struct player_effect_globals;

struct ColorARGB;
struct ColorRGB;
struct EffectPart;
struct ParticleSystemType;
struct ParticleSystemTypeParticleState;
struct bsp_leaf_reference;
struct collision_result;
struct contrail;
struct damage_data;
struct decal_flood_accumulator;
struct decal_projection;
struct effect;
struct effect_location_marker;
struct effect_tint_source;
struct object_marker;
struct particle;
struct particle_creation_data;
struct particle_state_values;
struct particle_system;
struct particle_system_particle;
struct particle_system_type_state;
struct player_camera_shake;
struct player_effect;
struct player_screen_flash;
struct real_matrix4x3;
struct real_point3d;
struct real_vector3d;
typedef uint32_t datum_index;
typedef float real;
typedef uint32_t tag_group;
typedef int32_t (*effect_marker_resolver)(uint32_t object_index, const char *location, object_marker *out, uint32_t max_count);
typedef uint32_t random_seed;
typedef int16_t EffectDistributionFunction_t;

namespace halo::effects {

/**
 * The engine globals the effects module owns (their storage is defined by standalone/data under the original link names);
 * other modules reach them through globals().
 */
struct Globals {
    random_seed &effect_random_seed;
    data_array *&contrail_data;
    data_array *&contrail_point_data;
    data_array *&decal_data;
    data_array *&effect_data;
    data_array *&effect_location_data;
    data_array *&particle_data;
    data_array *&particle_system_particle_data;
    data_array *&weather_particle_data;
    player_effect_globals *&player_effect_state;
    uint8_t &decals_for_all_responses;
    int32_t &player_effect_reentry_count;
    uint8_t &particle_spawn_debug_mode;
};

Globals &globals();

void ambient_color_for_marker(int16_t weather_row, real_point3d *position, uint8_t flags, real_vector3d *out);
uint8_t ambient_color_marker_visible(bsp_leaf_reference *location, real_point3d *position, real_vector3d *out, uint32_t filter_flags);
void ambient_color_randomize();
void ambient_color_sample(ColorRGB *out, real_point3d *position, real hash_scale, real intensity);
void contrail_advance(datum_index contrail_handle, uint8_t detach, real delta_time);
void contrail_age_points(datum_index contrail_handle, real delta_time);
void contrail_delete(datum_index contrail_index);
void contrail_generate_points(datum_index contrail_handle, int16_t point_count, uint8_t force);
datum_index contrail_new(int16_t attachment_index, datum_index object_index, datum_index definition_index);
void contrail_next_sequence(contrail *self);
int16_t contrail_points_due(datum_index contrail_handle, real elapsed_time);
void contrail_refresh_lightmap();
void contrail_update(real delta_time);
void contrails_initialize();
void decal_build_projection(real_matrix4x3 *placement, real *box, decal_projection *out);
void decal_clear_flags(uint8_t clear_object_attached);
void decal_delete(datum_index decal_index);
void decal_evict_object_decals(int16_t cluster_index);
void decal_flood_surfaces(decal_projection *projection, decal_flood_accumulator *accumulator, int32_t surface_index, uint8_t is_first_surface, real radius, int16_t decal_type, int32_t *surface_queue, uint16_t *surface_queue_count, int32_t *fallback_queue, uint16_t *fallback_queue_count);
void decal_link(int16_t cluster_index, datum_index decal_index, int16_t layer);
datum_index decal_new(datum_index requested_handle, int16_t cluster_index, int16_t layer, datum_index insert_before, uint8_t object_attached);
void decal_place(datum_index decal_tag_index, collision_result *placement, real_vector3d *direction, real radius_scale, uint8_t object_attached, int16_t requested_sequence_index);
void decal_rehash_object_decals();
void decal_spawn_for_response(datum_index response_tag_index, uint8_t deterministic, real_point3d *origin, real_vector3d *direction, real radius, int32_t marker_index);
void decal_update_fade(datum_index decal_index);
void decals_detach_from_structure_bsp();
void decals_initialize();
void decals_update_fade();
uint32_t effect_check_object_collisions();
void effect_delete(datum_index effect_index);
float effect_distribution_function_evaluate(EffectDistributionFunction_t type, float fraction);
void effect_event_apply(effect *self, EffectPart *part, effect_location_marker *marker, real_vector3d *up, real_vector3d *forward, real_point3d *position, real scale);
uint8_t effect_first_person_screen_timer_active(datum_index object_index);
void effect_marker_environment_probe(uint32_t definition_index, int16_t location_index, real_point3d *marker_position, uint32_t sound_param);
void effect_marker_from_node_table(int16_t entry_index, uint8_t *context, object_marker *out);
datum_index effect_marker_new(effect *self, int16_t location_index, object_marker *resolved_marker, uint8_t first_person);
effect_location_marker * effect_marker_next(effect *self, datum_index *marker, int32_t mode);
int32_t effect_marker_node_table_resolver(uint32_t object_index, const char *location, object_marker *out, uint32_t max_count);
datum_index effect_new(datum_index definition_index, datum_index creator_object_index, uint8_t force_create);
datum_index effect_new_at_texture_coordinate(datum_index definition_index, datum_index object_index, int16_t change_color_index, int16_t u, int16_t v);
datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index, datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale, const ColorRGB *color, const effect_tint_source *tint_source);
datum_index effect_new_on_object_with_node_table(datum_index creator_object_index, datum_index definition_index, datum_index object_index, uint16_t node_index, uint16_t ctx_08, uint32_t ctx_0c, uint32_t ctx_10, uint32_t ctx_14, real a_scale, real b_scale, const ColorRGB *color, const effect_tint_source *tint_source);
datum_index effect_new_with_color(datum_index definition_index, datum_index creator_object_index, const real_vector3d *velocity, uint16_t ctx_08, uint32_t ctx_0c, real_point3d *position, uint32_t ctx_14, real a_scale, real b_scale, const ColorRGB *color, const effect_tint_source *tint_source, uint8_t force_create);
real effect_property_random_value(uint8_t bit_index, effect *self, uint32_t a_bitset, uint32_t b_bitset, random_seed *seed, real base_min, real base_max);
void effect_random_direction_from_table(real_point3d *out);
void effect_random_direction_vector(random_seed *seed, real_point3d *out, real min, real max, effect *self, uint32_t a_bitset, uint32_t b_bitset);
real effect_random_fraction();
int16_t effect_random_int_between(int16_t minimum, int16_t maximum);
real effect_random_scaled_range(uint32_t flags, real scale, real base_min, real base_max, uint8_t bit_index);
uint32_t effect_random_uint16();
void effect_random_velocity_vector(effect *self, random_seed *seed, real_vector3d *direction, real_vector3d *out_direction, real_vector3d *out_velocity, real min, real max, real angle_max, uint32_t a_bitset, uint8_t b_bitset);
void effect_reattach_markers_for_object(int16_t first_person_weapon_index, datum_index object_index);
void effect_rebuild_markers(effect *self, effect_marker_resolver resolve_marker);
void effect_release_first_person_markers(int16_t first_person_weapon_index);
real_matrix4x3 * effect_resolve_marker_transform(effect *self, int16_t marker);
void effect_set_placement(effect *self, const ColorRGB *color, const effect_tint_source *tint_source, real a_scale, real b_scale);
void effect_spawn_particles(effect *self);
void effect_start_event(datum_index effect_handle, int16_t event_index);
void effect_stop(datum_index effect_handle, uint8_t stop_immediately);
effect * effect_try_and_get(datum_index effect_index);
void effect_update(datum_index effect_index, real dt);
void effects_refresh_structure_locations();
void effects_update_all(real delta_time);
void material_effects_play_at_marker(uint32_t material_effects_tag, int16_t material_type, int16_t sub_effect_index, uint32_t *location_bundle, uint32_t sound_param, real_point3d *position, real_vector3d *offset);
void object_change_color_evaluate(effect *self);
uint8_t particle_advance_animation(datum_index particle_handle, real delta_time);
uint8_t particle_advance_frame(datum_index particle_handle);
void particle_creation_physics_default(particle_system *system, int32_t type_index, particle_system_particle *particle, object_marker *marker);
void particle_creation_physics_explosion(particle_system *system, int32_t type_index, particle_system_particle *particle, object_marker *marker);
void particle_creation_physics_jet(particle_system *system, int32_t type_index, particle_system_particle *particle, object_marker *marker);
real particle_current_radius(datum_index particle_handle);
void particle_impact(datum_index particle_handle);
void particle_impact_response_dispatch(particle *self, tag_group fourcc, datum_index definition_index, real intensity);
void particle_new(particle_creation_data *creation_data);
uint8_t particle_next_sequence(datum_index particle_handle);
void particle_system_advance_particle_state(particle_system_particle *particle, ParticleSystemType *type);
void particle_system_advance_type_state(particle_system_type_state *state, ParticleSystemType *type, particle_system *system);
void particle_system_delete(datum_index handle);
datum_index particle_system_new_at_point(uint32_t definition_index, real_point3d *position, real_vector3d *velocity, ColorARGB *color, float scale);
datum_index particle_system_new_on_marker(uint32_t definition_index, uint32_t object_index, int16_t attachment_index);
uint8_t particle_system_new_type_states(datum_index handle);
void particle_system_render(datum_index particle_system_handle);
void particle_system_resolve_local_players();
void particle_system_roll_particle_state(int16_t index, ParticleSystemTypeParticleState *states, particle_state_values *out);
void particle_system_spawn(particle_system *system_record, int32_t type_index, float dt);
void particle_system_update(float delta_time, datum_index handle);
void particle_system_update_physics_default(particle_system *system, real dt);
void particle_system_update_physics_explosion(particle_system *system, real dt);
void particle_systems_delete_all();
void particle_systems_render();
void particle_systems_update(float delta_time);
uint8_t particle_update_motion(datum_index particle_handle, real delta_time);
void particle_update_physics_default(particle_system *system, int16_t type_index, real dt, particle_system_particle *particle);
void particles_delete_by_first_person_weapon(uint8_t first_person_weapon_index);
void particles_refresh_structure_locations();
void particles_update(real delta_time);
void player_effect_apply_at_object(uint32_t tag_reference, int16_t local_player_index, real_point3d *origin);
void player_effect_apply_continuous_damage(uint32_t tag_reference, int16_t local_player_index, float distance);
void player_effect_apply_generic_damage_feedback(datum_index player_index, float fraction);
void player_effect_build_camera_shake_matrix(real_matrix4x3 *out, int16_t local_player_index);
void player_effect_build_screen_flash(uint32_t *out, int16_t local_player_index);
void player_effect_clear_dead_players();
void player_effect_fade_damage_indicators(int16_t local_player_index, uint32_t *out_previous_indicators);
void player_effect_mark_damage_direction(datum_index player_index, const damage_data *dd, const real_vector3d *direction, float random_blend, float damage_amount);
void player_effect_mark_damage_direction_dispatch(void **context);
void player_effect_random_shake_offset(real_matrix4x3 *out, real magnitude, real angle);
void player_effect_send_network_update(datum_index player_handle, const real_vector3d *direction, const damage_data *dd, float random_blend, float damage_amount);
void player_effect_set_camera_impulse(player_effect *self, int16_t local_player_index, real *descriptor, real *direction, real intensity_falloff, real duration_scale);
void player_effect_set_camera_shake(player_effect *self, player_camera_shake *descriptor, float intensity_falloff, float duration_scale);
void player_effect_set_screen_flash(player_effect *self, player_screen_flash *descriptor, float intensity_falloff, float duration_scale);
void player_effect_set_screen_flash_for_player(datum_index player_index, player_screen_flash *descriptor, float intensity_falloff);
int32_t player_weapon_locality_for_object(datum_index weapon_object_index);
void weather_instance_activate(datum_index definition_index, int16_t instance_index, real intensity);
void weather_instance_adjust_count(int16_t instance_index, int16_t type_index, real target_value);
void weather_instance_build_render_geometry(int16_t instance_index);
void weather_instance_deactivate(int16_t instance_index);
void weather_instance_update(int16_t instance_index);
datum_index weather_particle_new(int16_t instance_index, int16_t type_index);
void weather_particle_update(datum_index weather_particle_handle, int16_t type_index, int16_t instance_index);
void weather_update();
void weather_update_local_player();

}
