#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"
#include "projectiles.h"

// struct sizes
// projectile_data is fixed by the object_type_definition row at 0x0069b9a0 (object_size 0x2b0,
// chain { object, projectile }), so 0x2b0 - 0x1f4 = 0xbc.
typedef char check_projectile_data[(sizeof(projectile_data) == 0xbc) ? 1 : -1];
typedef char check_object_plus_projectile[(0x1f4 + sizeof(projectile_data) == 0x2b0) ? 1 : -1];
typedef char check_projectile_network_state[(sizeof(projectile_network_state) == 0x18) ? 1 : -1];
typedef char check_collision_result[(sizeof(collision_result) == 0x50) ? 1 : -1];
typedef char check_creation_message[(sizeof(projectile_creation_message) == 0x54) ? 1 : -1];
typedef char check_detonation_message[(sizeof(projectile_detonation_message) == 0x10) ? 1 : -1];
typedef char check_attach_message[(sizeof(projectile_attach_message) == 0x0a) ? 1 : -1];
typedef char check_update_header[(sizeof(projectile_network_update_header) == 0x07) ? 1 : -1];

// projectile_data offsets, written as OBJECT offsets the way the module reads them
#define PROJ_OFF(field) (0x1f4 + __builtin_offsetof(projectile_data, field))
typedef char check_proj_flags[(PROJ_OFF(flags) == 0x22c) ? 1 : -1];
typedef char check_proj_state[(PROJ_OFF(state) == 0x230) ? 1 : -1];
typedef char check_proj_material[(PROJ_OFF(material_response_index) == 0x232) ? 1 : -1];
typedef char check_proj_ignore[(PROJ_OFF(ignore_object_index) == 0x234) ? 1 : -1];
typedef char check_proj_tracked[(PROJ_OFF(tracked_object_index) == 0x238) ? 1 : -1];
typedef char check_proj_contrail[(PROJ_OFF(contrail_attachment_index) == 0x23c) ? 1 : -1];
typedef char check_proj_det_timer[(PROJ_OFF(detonation_timer) == 0x240) ? 1 : -1];
typedef char check_proj_det_rate[(PROJ_OFF(detonation_timer_rate) == 0x244) ? 1 : -1];
typedef char check_proj_arm_timer[(PROJ_OFF(arming_timer) == 0x248) ? 1 : -1];
typedef char check_proj_arm_rate[(PROJ_OFF(arming_timer_rate) == 0x24c) ? 1 : -1];
typedef char check_proj_distance[(PROJ_OFF(distance_travelled) == 0x250) ? 1 : -1];
typedef char check_proj_dec_delay[(PROJ_OFF(deceleration_delay) == 0x254) ? 1 : -1];
typedef char check_proj_dec_delay_rate[(PROJ_OFF(deceleration_delay_rate) == 0x258) ? 1 : -1];
typedef char check_proj_dec[(PROJ_OFF(deceleration) == 0x25c) ? 1 : -1];
typedef char check_proj_dec_end[(PROJ_OFF(deceleration_end_range) == 0x260) ? 1 : -1];
typedef char check_proj_rot_axis[(PROJ_OFF(rotation_axis) == 0x264) ? 1 : -1];
typedef char check_proj_rot_sine[(PROJ_OFF(rotation_sine) == 0x270) ? 1 : -1];
typedef char check_proj_rot_cosine[(PROJ_OFF(rotation_cosine) == 0x274) ? 1 : -1];
typedef char check_proj_thrown[(PROJ_OFF(thrown_grenade) == 0x278) ? 1 : -1];
typedef char check_proj_net_valid[(PROJ_OFF(network_state_valid) == 0x279) ? 1 : -1];
typedef char check_proj_net_baseline[(PROJ_OFF(network_baseline_index) == 0x27a) ? 1 : -1];
typedef char check_proj_net_seq[(PROJ_OFF(network_sequence) == 0x27b) ? 1 : -1];
typedef char check_proj_net_state[(PROJ_OFF(network_state) == 0x27c) ? 1 : -1];
typedef char check_proj_last_valid[(PROJ_OFF(last_update_valid) == 0x294) ? 1 : -1];
typedef char check_proj_last_state[(PROJ_OFF(last_update_state) == 0x298) ? 1 : -1];

// collision_result offsets, as read by projectile_response through its ebp base
typedef char check_cr_type[(__builtin_offsetof(collision_result, type) == 0x00) ? 1 : -1];
typedef char check_cr_leaf[(__builtin_offsetof(collision_result, leaf) == 0x0c) ? 1 : -1];
typedef char check_cr_t[(__builtin_offsetof(collision_result, t) == 0x14) ? 1 : -1];
typedef char check_cr_point[(__builtin_offsetof(collision_result, point) == 0x18) ? 1 : -1];
typedef char check_cr_normal[(__builtin_offsetof(collision_result, normal) == 0x24) ? 1 : -1];
typedef char check_cr_material[(__builtin_offsetof(collision_result, material_type) == 0x34) ? 1 : -1];
typedef char check_cr_object[(__builtin_offsetof(collision_result, object_index) == 0x38) ? 1 : -1];
typedef char check_cr_marker[(__builtin_offsetof(collision_result, marker_index) == 0x3e) ? 1 : -1];
typedef char check_cr_surface[(__builtin_offsetof(collision_result, surface_index) == 0x44) ? 1 : -1];
typedef char check_cr_flags[(__builtin_offsetof(collision_result, surface_flags) == 0x4c) ? 1 : -1];
typedef char check_cr_4e[(__builtin_offsetof(collision_result, unknown_4e) == 0x4e) ? 1 : -1];

// message record offsets
typedef char check_cm_position[(__builtin_offsetof(projectile_creation_message, position) == 0x14) ? 1 : -1];
typedef char check_cm_forward[(__builtin_offsetof(projectile_creation_message, forward) == 0x20) ? 1 : -1];
typedef char check_cm_up[(__builtin_offsetof(projectile_creation_message, up) == 0x2c) ? 1 : -1];
typedef char check_cm_velocity[(__builtin_offsetof(projectile_creation_message, velocity) == 0x38) ? 1 : -1];
typedef char check_cm_angular[(__builtin_offsetof(projectile_creation_message, angular_velocity) == 0x44) ? 1 : -1];
typedef char check_cm_baseline[(__builtin_offsetof(projectile_creation_message, baseline_index) == 0x50) ? 1 : -1];
typedef char check_dm_position[(__builtin_offsetof(projectile_detonation_message, position) == 0x04) ? 1 : -1];
typedef char check_am_marker[(__builtin_offsetof(projectile_attach_message, parent_marker_index) == 0x08) ? 1 : -1];

// the Projectile tag offsets every attribution above rests on
typedef char check_tag_flags[(__builtin_offsetof(Projectile, projectile_flags) == 0x17c) ? 1 : -1];
typedef char check_tag_timer_starts[(__builtin_offsetof(Projectile, detonation_timer_starts) == 0x180) ? 1 : -1];
typedef char check_tag_impact_noise[(__builtin_offsetof(Projectile, impact_noise) == 0x182) ? 1 : -1];
typedef char check_tag_a_in[(__builtin_offsetof(Projectile, projectile_a_in) == 0x184) ? 1 : -1];
typedef char check_tag_collision_radius[(__builtin_offsetof(Projectile, collision_radius) == 0x1a0) ? 1 : -1];
typedef char check_tag_arming_time[(__builtin_offsetof(Projectile, arming_time) == 0x1a4) ? 1 : -1];
typedef char check_tag_timer[(__builtin_offsetof(Projectile, timer) == 0x1bc) ? 1 : -1];
typedef char check_tag_min_velocity[(__builtin_offsetof(Projectile, minimum_velocity) == 0x1c4) ? 1 : -1];
typedef char check_tag_max_range[(__builtin_offsetof(Projectile, maximum_range) == 0x1c8) ? 1 : -1];
typedef char check_tag_air_gravity[(__builtin_offsetof(Projectile, air_gravity_scale) == 0x1cc) ? 1 : -1];
typedef char check_tag_air_range[(__builtin_offsetof(Projectile, air_damage_range) == 0x1d0) ? 1 : -1];
typedef char check_tag_water_gravity[(__builtin_offsetof(Projectile, water_gravity_scale) == 0x1d8) ? 1 : -1];
typedef char check_tag_water_range[(__builtin_offsetof(Projectile, water_damage_range) == 0x1dc) ? 1 : -1];
typedef char check_tag_initial_velocity[(__builtin_offsetof(Projectile, initial_velocity) == 0x1e4) ? 1 : -1];
typedef char check_tag_final_velocity[(__builtin_offsetof(Projectile, final_velocity) == 0x1e8) ? 1 : -1];
typedef char check_tag_guided[(__builtin_offsetof(Projectile, guided_angular_velocity) == 0x1ec) ? 1 : -1];
typedef char check_tag_responses[(__builtin_offsetof(Projectile, projectile_material_response) == 0x240) ? 1 : -1];
typedef char check_tag_size[(sizeof(Projectile) == 0x24c) ? 1 : -1];

// the ProjectileMaterialResponse offsets the static record at 0x00695e20 confirms
typedef char check_pmr_size[(sizeof(ProjectileMaterialResponse) == 0xa0) ? 1 : -1];
typedef char check_pmr_default_effect[(__builtin_offsetof(ProjectileMaterialResponse, default_effect) == 0x04) ? 1 : -1];
typedef char check_pmr_potential_response[(__builtin_offsetof(ProjectileMaterialResponse, potential_response) == 0x24) ? 1 : -1];
typedef char check_pmr_skip[(__builtin_offsetof(ProjectileMaterialResponse, potential_skip_fraction) == 0x28) ? 1 : -1];
typedef char check_pmr_between[(__builtin_offsetof(ProjectileMaterialResponse, potential_between) == 0x2c) ? 1 : -1];
typedef char check_pmr_and[(__builtin_offsetof(ProjectileMaterialResponse, potential_and) == 0x34) ? 1 : -1];
typedef char check_pmr_potential_effect[(__builtin_offsetof(ProjectileMaterialResponse, potential_effect) == 0x3c) ? 1 : -1];
typedef char check_pmr_scale[(__builtin_offsetof(ProjectileMaterialResponse, scale_effects_by) == 0x5c) ? 1 : -1];
typedef char check_pmr_angular_noise[(__builtin_offsetof(ProjectileMaterialResponse, angular_noise) == 0x60) ? 1 : -1];
typedef char check_pmr_velocity_noise[(__builtin_offsetof(ProjectileMaterialResponse, velocity_noise) == 0x64) ? 1 : -1];
typedef char check_pmr_det_effect[(__builtin_offsetof(ProjectileMaterialResponse, detonation_effect) == 0x68) ? 1 : -1];
typedef char check_pmr_initial_friction[(__builtin_offsetof(ProjectileMaterialResponse, initial_friction) == 0x90) ? 1 : -1];
typedef char check_pmr_parallel[(__builtin_offsetof(ProjectileMaterialResponse, parallel_friction) == 0x98) ? 1 : -1];
typedef char check_pmr_perp[(__builtin_offsetof(ProjectileMaterialResponse, perpendicular_friction) == 0x9c) ? 1 : -1];

// enum values the module dispatches on
typedef char check_state_values[(_projectile_state_flying == 0 && _projectile_state_detonating == 1
                                 && _projectile_state_disappearing == 2) ? 1 : -1];
typedef char check_flag_initial[(_projectile_tracer_bit == 2) ? 1 : -1];
typedef char check_response_values[(projectileresponse_disappear == 0 && projectileresponse_detonate == 1
                                    && projectileresponse_reflect == 2 && projectileresponse_overpenetrate == 3
                                    && projectileresponse_attach == 4) ? 1 : -1];
typedef char check_function_in[(projectilefunctionin_range_remaining == 1
                                && projectilefunctionin_time_remaining == 2
                                && projectilefunctionin_tracer == 3) ? 1 : -1];
typedef char check_sizes[(k_projectile_object_size - k_projectile_data_offset == 0xbc) ? 1 : -1];
