#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

// struct sizes
typedef char check_decal_type_parameters[(sizeof(decal_type_parameters) == 0x10) ? 1 : -1];
typedef char check_contrail[(sizeof(contrail) == 0x44) ? 1 : -1];
typedef char check_contrail_point[(sizeof(contrail_point) == 0x38) ? 1 : -1];
typedef char check_decal[(sizeof(decal) == 0x38) ? 1 : -1];
typedef char check_decal_grid[(sizeof(decal_grid) == 0x280c) ? 1 : -1];
typedef char check_decal_projected_corner[(sizeof(decal_projected_corner) == 0x08) ? 1 : -1];
typedef char check_decal_projection[(sizeof(decal_projection) == 0x8c) ? 1 : -1];
typedef char check_effect_tint_source[(sizeof(effect_tint_source) == 0x0c) ? 1 : -1];
typedef char check_effect[(sizeof(effect) == 0xfc) ? 1 : -1];
typedef char check_effect_location_marker[(sizeof(effect_location_marker) == 0x3c) ? 1 : -1];
typedef char check_particle_state_values[(sizeof(particle_state_values) == 0x1c) ? 1 : -1];
typedef char check_particle_system_type_state[(sizeof(particle_system_type_state) == 0x40) ? 1 : -1];
typedef char check_particle_system[(sizeof(particle_system) == 0x158) ? 1 : -1];
typedef char check_particle_system_particle[(sizeof(particle_system_particle) == 0x80) ? 1 : -1];
typedef char check_particle_creation_data[(sizeof(particle_creation_data) == 0x5c) ? 1 : -1];
typedef char check_particle[(sizeof(particle) == 0x70) ? 1 : -1];
typedef char check_weather_state[(sizeof(weather_particle_system_state) == 0x20) ? 1 : -1];
typedef char check_weather_instance_type[(sizeof(weather_instance_type) == 0x10) ? 1 : -1];
typedef char check_weather_instance[(sizeof(weather_instance) == 0x9c) ? 1 : -1];
typedef char check_weather_particle[(sizeof(weather_particle) == 0x54) ? 1 : -1];
typedef char check_ambient_noise_grid[(sizeof(ambient_noise_grid) == 0x900) ? 1 : -1];
typedef char check_player_screen_flash[(sizeof(player_screen_flash) == 0x38) ? 1 : -1];
typedef char check_player_camera_impulse[(sizeof(player_camera_impulse) == 0x34) ? 1 : -1];
typedef char check_player_camera_shake[(sizeof(player_camera_shake) == 0x48) ? 1 : -1];
typedef char check_player_effect[(sizeof(player_effect) == 0xec) ? 1 : -1];
typedef char check_player_effect_globals[(sizeof(player_effect_globals) == 0x124) ? 1 : -1];

// contrail offsets, from contrail_new 0x44c910 and contrail_update 0x44cb50
typedef char check_c_flags[(__builtin_offsetof(contrail, flags) == 0x02) ? 1 : -1];
typedef char check_c_def[(__builtin_offsetof(contrail, definition_index) == 0x04) ? 1 : -1];
typedef char check_c_object[(__builtin_offsetof(contrail, object_index) == 0x08) ? 1 : -1];
typedef char check_c_attach[(__builtin_offsetof(contrail, attachment_index) == 0x0c) ? 1 : -1];
typedef char check_c_scalefn[(__builtin_offsetof(contrail, scale_function_index) == 0x0e) ? 1 : -1];
typedef char check_c_scale[(__builtin_offsetof(contrail, scale) == 0x10) ? 1 : -1];
typedef char check_c_seq[(__builtin_offsetof(contrail, sequence_index) == 0x14) ? 1 : -1];
typedef char check_c_frame[(__builtin_offsetof(contrail, frame_index) == 0x16) ? 1 : -1];
typedef char check_c_u[(__builtin_offsetof(contrail, texture_offset_u) == 0x18) ? 1 : -1];
typedef char check_c_v[(__builtin_offsetof(contrail, texture_offset_v) == 0x1c) ? 1 : -1];
typedef char check_c_gen[(__builtin_offsetof(contrail, generation_timer) == 0x20) ? 1 : -1];
typedef char check_c_anim[(__builtin_offsetof(contrail, animation_timer) == 0x24) ? 1 : -1];
typedef char check_c_acc[(__builtin_offsetof(contrail, accumulated_delta_time) == 0x28) ? 1 : -1];
typedef char check_c_counts[(__builtin_offsetof(contrail, point_count) == 0x2c) ? 1 : -1];
typedef char check_c_first[(__builtin_offsetof(contrail, first_point) == 0x34) ? 1 : -1];

// contrail_point offsets, from contrail_generate_points 0x44d020 and contrail_age_points 0x44d470
typedef char check_cp_state[(__builtin_offsetof(contrail_point, state_index) == 0x03) ? 1 : -1];
typedef char check_cp_age[(__builtin_offsetof(contrail_point, age) == 0x04) ? 1 : -1];
typedef char check_cp_invdur[(__builtin_offsetof(contrail_point, inverse_duration) == 0x08) ? 1 : -1];
typedef char check_cp_scale[(__builtin_offsetof(contrail_point, scale) == 0x0c) ? 1 : -1];
typedef char check_cp_loc[(__builtin_offsetof(contrail_point, location) == 0x14) ? 1 : -1];
typedef char check_cp_pos[(__builtin_offsetof(contrail_point, position) == 0x1c) ? 1 : -1];
typedef char check_cp_vel[(__builtin_offsetof(contrail_point, velocity) == 0x28) ? 1 : -1];
typedef char check_cp_next[(__builtin_offsetof(contrail_point, next_point) == 0x34) ? 1 : -1];

// decal offsets, from decal_place 0x44edc0, decal_link 0x44dd30 and decal_update_fade 0x44dc30
typedef char check_d_cluster[(__builtin_offsetof(decal, cluster_index) == 0x04) ? 1 : -1];
typedef char check_d_layer[(__builtin_offsetof(decal, layer) == 0x06) ? 1 : -1];
typedef char check_d_pos[(__builtin_offsetof(decal, position) == 0x08) ? 1 : -1];
typedef char check_d_time[(__builtin_offsetof(decal, creation_game_time) == 0x14) ? 1 : -1];
typedef char check_d_seq[(__builtin_offsetof(decal, sequence_index) == 0x18) ? 1 : -1];
typedef char check_d_life[(__builtin_offsetof(decal, lifetime) == 0x1c) ? 1 : -1];
typedef char check_d_decay[(__builtin_offsetof(decal, decay_time) == 0x20) ? 1 : -1];
typedef char check_d_color[(__builtin_offsetof(decal, color) == 0x24) ? 1 : -1];
typedef char check_d_alpha[(__builtin_offsetof(decal, alpha) == 0x28) ? 1 : -1];
typedef char check_d_tris[(__builtin_offsetof(decal, triangle_count) == 0x2a) ? 1 : -1];
typedef char check_d_def[(__builtin_offsetof(decal, definition_index) == 0x2c) ? 1 : -1];
typedef char check_d_prev[(__builtin_offsetof(decal, previous_decal) == 0x30) ? 1 : -1];
typedef char check_d_next[(__builtin_offsetof(decal, next_decal) == 0x34) ? 1 : -1];
typedef char check_dg_object[(__builtin_offsetof(decal_grid, first_object_decal) == 0x2800) ? 1 : -1];
typedef char check_dg_temp[(__builtin_offsetof(decal_grid, temporary_count) == 0x2804) ? 1 : -1];
typedef char check_dg_objcount[(__builtin_offsetof(decal_grid, object_count) == 0x2808) ? 1 : -1];

// decal_projection offsets, from decal_build_projection 0x44e460 and decal_flood_surfaces 0x44e730
typedef char check_dp_axis[(__builtin_offsetof(decal_projection, major_axis) == 0x54) ? 1 : -1];
typedef char check_dp_sign[(__builtin_offsetof(decal_projection, normal_positive) == 0x56) ? 1 : -1];
typedef char check_dp_corners[(__builtin_offsetof(decal_projection, corners) == 0x58) ? 1 : -1];
typedef char check_dp_du0[(__builtin_offsetof(decal_projection, du_edge0) == 0x78) ? 1 : -1];
typedef char check_dp_invdet[(__builtin_offsetof(decal_projection, inverse_determinant) == 0x88) ? 1 : -1];

// effect offsets, from effect_new 0x451500, effect_set_placement 0x451600,
// effect_start_event 0x451660, effect_update 0x451a30 and effect_spawn_particles 0x451f90
typedef char check_e_def[(__builtin_offsetof(effect, definition_index) == 0x04) ? 1 : -1];
typedef char check_e_change[(__builtin_offsetof(effect, change_color_index) == 0x0c) ? 1 : -1];
typedef char check_e_loc[(__builtin_offsetof(effect, location) == 0x10) ? 1 : -1];
typedef char check_e_color[(__builtin_offsetof(effect, color) == 0x18) ? 1 : -1];
typedef char check_e_vel[(__builtin_offsetof(effect, velocity) == 0x24) ? 1 : -1];
typedef char check_e_tint[(__builtin_offsetof(effect, tint_source) == 0x30) ? 1 : -1];
typedef char check_e_tintproc[(0x30 + __builtin_offsetof(effect_tint_source, proc) == 0x34) ? 1 : -1];
typedef char check_e_object[(__builtin_offsetof(effect, object_index) == 0x3c) ? 1 : -1];
typedef char check_e_creator[(__builtin_offsetof(effect, creator_object_index) == 0x40) ? 1 : -1];
typedef char check_e_a[(__builtin_offsetof(effect, a_scale) == 0x44) ? 1 : -1];
typedef char check_e_b[(__builtin_offsetof(effect, b_scale) == 0x48) ? 1 : -1];
typedef char check_e_fp[(__builtin_offsetof(effect, first_person_weapon_index) == 0x4c) ? 1 : -1];
typedef char check_e_event[(__builtin_offsetof(effect, event_index) == 0x4e) ? 1 : -1];
typedef char check_e_etime[(__builtin_offsetof(effect, event_time) == 0x50) ? 1 : -1];
typedef char check_e_edur[(__builtin_offsetof(effect, event_duration) == 0x54) ? 1 : -1];
typedef char check_e_prev[(__builtin_offsetof(effect, previous_event_fraction) == 0x58) ? 1 : -1];
typedef char check_e_markers[(__builtin_offsetof(effect, location_markers) == 0x5c) ? 1 : -1];
typedef char check_e_counts[(__builtin_offsetof(effect, particle_counts) == 0xdc) ? 1 : -1];
typedef char check_em_marker[(__builtin_offsetof(effect_location_marker, marker_index) == 0x02) ? 1 : -1];
typedef char check_em_next[(__builtin_offsetof(effect_location_marker, next_marker) == 0x04) ? 1 : -1];
typedef char check_em_xform[(__builtin_offsetof(effect_location_marker, transform) == 0x08) ? 1 : -1];
// the marker transform copy starts at object_marker.transform, so the two must line up
typedef char check_em_from_object_marker[(__builtin_offsetof(object_marker, transform) == 0x04) ? 1 : -1];

// particle_system offsets, from particle_system_new_at_point 0x453600,
// particle_system_new_on_marker 0x4536f0, particle_system_new_type_states 0x4538b0 and
// particle_system_update 0x4544f0
typedef char check_ps_flags[(__builtin_offsetof(particle_system, flags) == 0x04) ? 1 : -1];
typedef char check_ps_def[(__builtin_offsetof(particle_system, definition_index) == 0x08) ? 1 : -1];
typedef char check_ps_object[(__builtin_offsetof(particle_system, object_index) == 0x0c) ? 1 : -1];
typedef char check_ps_attach[(__builtin_offsetof(particle_system, attachment_index) == 0x10) ? 1 : -1];
typedef char check_ps_scalefn[(__builtin_offsetof(particle_system, scale_function_index) == 0x12) ? 1 : -1];
typedef char check_ps_scale[(__builtin_offsetof(particle_system, scale) == 0x14) ? 1 : -1];
typedef char check_ps_loc[(__builtin_offsetof(particle_system, location) == 0x18) ? 1 : -1];
typedef char check_ps_pos[(__builtin_offsetof(particle_system, position) == 0x20) ? 1 : -1];
typedef char check_ps_vel[(__builtin_offsetof(particle_system, velocity) == 0x2c) ? 1 : -1];
typedef char check_ps_color[(__builtin_offsetof(particle_system, color) == 0x38) ? 1 : -1];
typedef char check_ps_ambient[(__builtin_offsetof(particle_system, ambient_color) == 0x48) ? 1 : -1];
typedef char check_ps_states[(__builtin_offsetof(particle_system, type_states) == 0x58) ? 1 : -1];
// the per type block that particle_system_spawn 0x453b10 reaches as 0x94 + i*0x40
typedef char check_ps_first_particle[(0x58 + __builtin_offsetof(particle_system_type_state, first_particle) == 0x94) ? 1 : -1];
typedef char check_pst_next[(__builtin_offsetof(particle_system_type_state, next_state_index) == 0x02) ? 1 : -1];
typedef char check_pst_remain[(__builtin_offsetof(particle_system_type_state, state_time_remaining) == 0x04) ? 1 : -1];
typedef char check_pst_dur[(__builtin_offsetof(particle_system_type_state, state_duration) == 0x08) ? 1 : -1];
typedef char check_pst_scale[(__builtin_offsetof(particle_system_type_state, scale) == 0x0c) ? 1 : -1];
typedef char check_pst_color[(__builtin_offsetof(particle_system_type_state, color) == 0x18) ? 1 : -1];
typedef char check_pst_mincount[(__builtin_offsetof(particle_system_type_state, minimum_particle_count) == 0x2c) ? 1 : -1];
typedef char check_pst_rate[(__builtin_offsetof(particle_system_type_state, particle_creation_rate) == 0x30) ? 1 : -1];
typedef char check_pst_frac[(__builtin_offsetof(particle_system_type_state, creation_fraction) == 0x34) ? 1 : -1];
typedef char check_pst_pp[(__builtin_offsetof(particle_system_type_state, ping_pong_forward) == 0x38) ? 1 : -1];
typedef char check_pst_count[(__builtin_offsetof(particle_system_type_state, particle_count) == 0x3a) ? 1 : -1];

// particle_system_particle offsets, from particle_system_spawn 0x453b10,
// particle_system_update 0x4544f0 and particle_system_render 0x454bf0
typedef char check_psp_next[(__builtin_offsetof(particle_system_particle, next_particle) == 0x04) ? 1 : -1];
typedef char check_psp_state[(__builtin_offsetof(particle_system_particle, state_index) == 0x08) ? 1 : -1];
typedef char check_psp_nstate[(__builtin_offsetof(particle_system_particle, next_state_index) == 0x0a) ? 1 : -1];
typedef char check_psp_remain[(__builtin_offsetof(particle_system_particle, state_time_remaining) == 0x0c) ? 1 : -1];
typedef char check_psp_dur[(__builtin_offsetof(particle_system_particle, state_duration) == 0x10) ? 1 : -1];
typedef char check_psp_loc[(__builtin_offsetof(particle_system_particle, location) == 0x14) ? 1 : -1];
typedef char check_psp_pos[(__builtin_offsetof(particle_system_particle, position) == 0x1c) ? 1 : -1];
typedef char check_psp_dir[(__builtin_offsetof(particle_system_particle, direction) == 0x34) ? 1 : -1];
typedef char check_psp_rot[(__builtin_offsetof(particle_system_particle, rotation) == 0x40) ? 1 : -1];
typedef char check_psp_frame[(__builtin_offsetof(particle_system_particle, frame) == 0x44) ? 1 : -1];
typedef char check_psp_values[(__builtin_offsetof(particle_system_particle, values) == 0x48) ? 1 : -1];
typedef char check_psp_next_values[(__builtin_offsetof(particle_system_particle, next_values) == 0x64) ? 1 : -1];
typedef char check_psv_color[(0x48 + __builtin_offsetof(particle_state_values, color) == 0x54) ? 1 : -1];

// particle offsets, from particle_new 0x455740, particle_update_motion 0x4561a0 and
// particle_current_radius 0x4566f0
typedef char check_p_def[(__builtin_offsetof(particle, definition_index) == 0x04) ? 1 : -1];
typedef char check_p_object[(__builtin_offsetof(particle, object_index) == 0x08) ? 1 : -1];
typedef char check_p_marker[(__builtin_offsetof(particle, marker_index) == 0x0c) ? 1 : -1];
typedef char check_p_seqstate[(__builtin_offsetof(particle, sequence_state) == 0x0e) ? 1 : -1];
typedef char check_p_fp[(__builtin_offsetof(particle, first_person_weapon_index) == 0x0f) ? 1 : -1];
typedef char check_p_tick[(__builtin_offsetof(particle, last_update_tick) == 0x10) ? 1 : -1];
typedef char check_p_age[(__builtin_offsetof(particle, age) == 0x14) ? 1 : -1];
typedef char check_p_life[(__builtin_offsetof(particle, lifespan) == 0x18) ? 1 : -1];
typedef char check_p_anim[(__builtin_offsetof(particle, animation_timer) == 0x1c) ? 1 : -1];
typedef char check_p_invanim[(__builtin_offsetof(particle, inverse_animation_period) == 0x20) ? 1 : -1];
typedef char check_p_seq[(__builtin_offsetof(particle, sequence_index) == 0x24) ? 1 : -1];
typedef char check_p_frame[(__builtin_offsetof(particle, frame_index) == 0x26) ? 1 : -1];
typedef char check_p_loc[(__builtin_offsetof(particle, location) == 0x28) ? 1 : -1];
typedef char check_p_pos[(__builtin_offsetof(particle, position) == 0x30) ? 1 : -1];
typedef char check_p_vel[(__builtin_offsetof(particle, velocity) == 0x48) ? 1 : -1];
typedef char check_p_scale[(__builtin_offsetof(particle, scale) == 0x5c) ? 1 : -1];
typedef char check_p_color[(__builtin_offsetof(particle, color) == 0x60) ? 1 : -1];
typedef char check_pcd_pos[(__builtin_offsetof(particle_creation_data, position) == 0x10) ? 1 : -1];
typedef char check_pcd_vel[(__builtin_offsetof(particle_creation_data, velocity) == 0x28) ? 1 : -1];
typedef char check_pcd_scale[(__builtin_offsetof(particle_creation_data, scale) == 0x48) ? 1 : -1];
typedef char check_pcd_color[(__builtin_offsetof(particle_creation_data, color) == 0x4c) ? 1 : -1];

// weather offsets, from weather_update 0x53f5c0, weather_instance_activate 0x457e20,
// weather_particle_new 0x458070 and weather_particle_update 0x458630
typedef char check_ws_mag[(__builtin_offsetof(weather_particle_system_state, magnitude_walk) == 0x04) ? 1 : -1];
typedef char check_ws_pitch[(__builtin_offsetof(weather_particle_system_state, pitch_walk) == 0x08) ? 1 : -1];
typedef char check_ws_yaw[(__builtin_offsetof(weather_particle_system_state, yaw_walk) == 0x0c) ? 1 : -1];
typedef char check_ws_magnitude[(__builtin_offsetof(weather_particle_system_state, magnitude) == 0x10) ? 1 : -1];
typedef char check_ws_i[(__builtin_offsetof(weather_particle_system_state, direction_i) == 0x14) ? 1 : -1];
typedef char check_ws_k[(__builtin_offsetof(weather_particle_system_state, direction_k) == 0x1c) ? 1 : -1];
typedef char check_wi_elapsed[(__builtin_offsetof(weather_instance, elapsed_time) == 0x04) ? 1 : -1];
typedef char check_wi_delta[(__builtin_offsetof(weather_instance, delta_time) == 0x08) ? 1 : -1];
typedef char check_wi_intensity[(__builtin_offsetof(weather_instance, intensity) == 0x0c) ? 1 : -1];
typedef char check_wi_cluster[(__builtin_offsetof(weather_instance, cluster_index) == 0x18) ? 1 : -1];
typedef char check_wi_sky[(__builtin_offsetof(weather_instance, in_sky) == 0x1a) ? 1 : -1];
typedef char check_wi_types[(__builtin_offsetof(weather_instance, types) == 0x1c) ? 1 : -1];
// the per type block that weather_instance_adjust_count 0x457fc0 reaches as 0x006b0b00 + i*0x10,
// which is 0x006b0ae4 + 0x1c + i*0x10, with the count at +8 and the head at +0xc
typedef char check_wit_count[(0x1c + __builtin_offsetof(weather_instance_type, particle_count) == 0x24) ? 1 : -1];
typedef char check_wit_first[(0x1c + __builtin_offsetof(weather_instance_type, first_particle) == 0x28) ? 1 : -1];
typedef char check_wp_pos[(__builtin_offsetof(weather_particle, position) == 0x04) ? 1 : -1];
typedef char check_wp_vel[(__builtin_offsetof(weather_particle, velocity) == 0x10) ? 1 : -1];
typedef char check_wp_accel[(__builtin_offsetof(weather_particle, acceleration) == 0x1c) ? 1 : -1];
typedef char check_wp_seq[(__builtin_offsetof(weather_particle, sequence_index) == 0x28) ? 1 : -1];
typedef char check_wp_frame[(__builtin_offsetof(weather_particle, frame) == 0x2c) ? 1 : -1];
typedef char check_wp_rot[(__builtin_offsetof(weather_particle, rotation) == 0x30) ? 1 : -1];
typedef char check_wp_alpha[(__builtin_offsetof(weather_particle, alpha) == 0x34) ? 1 : -1];
typedef char check_wp_color[(__builtin_offsetof(weather_particle, color) == 0x38) ? 1 : -1];
typedef char check_wp_radius[(__builtin_offsetof(weather_particle, radius) == 0x44) ? 1 : -1];
typedef char check_wp_rotrate[(__builtin_offsetof(weather_particle, rotation_rate) == 0x48) ? 1 : -1];
typedef char check_wp_animrate[(__builtin_offsetof(weather_particle, animation_rate) == 0x4c) ? 1 : -1];
typedef char check_wp_next[(__builtin_offsetof(weather_particle, next_particle) == 0x50) ? 1 : -1];

// player_effect offsets, from the three set helpers and
// player_effect_apply_continuous_damage 0x4567c0
typedef char check_pe_flash[(__builtin_offsetof(player_effect, flash) == 0x18) ? 1 : -1];
typedef char check_pe_impulse[(__builtin_offsetof(player_effect, impulse) == 0x50) ? 1 : -1];
typedef char check_pe_shake[(__builtin_offsetof(player_effect, shake) == 0x84) ? 1 : -1];
typedef char check_pe_lf[(__builtin_offsetof(player_effect, low_frequency_vibrate) == 0xcc) ? 1 : -1];
typedef char check_pe_hf[(__builtin_offsetof(player_effect, high_frequency_vibrate) == 0xd0) ? 1 : -1];
typedef char check_pe_st[(__builtin_offsetof(player_effect, shake_translation) == 0xd4) ? 1 : -1];
typedef char check_pe_sr[(__builtin_offsetof(player_effect, shake_rotation) == 0xd8) ? 1 : -1];
typedef char check_pe_vt[(__builtin_offsetof(player_effect, vibrate_ticks) == 0xdc) ? 1 : -1];
typedef char check_pe_ft[(__builtin_offsetof(player_effect, flash_ticks) == 0xde) ? 1 : -1];
typedef char check_pe_it[(__builtin_offsetof(player_effect, impulse_ticks) == 0xe0) ? 1 : -1];
typedef char check_pe_ht[(__builtin_offsetof(player_effect, shake_ticks) == 0xe2) ? 1 : -1];
typedef char check_pe_ind[(__builtin_offsetof(player_effect, damage_indicator_alpha) == 0xe4) ? 1 : -1];
typedef char check_pe_flags[(__builtin_offsetof(player_effect, flags) == 0xe8) ? 1 : -1];
// the sub block fields the helpers touch through the record base
typedef char check_pe_flash_dur[(0x18 + __builtin_offsetof(player_screen_flash, duration) == 0x28) ? 1 : -1];
typedef char check_pe_flash_int[(0x18 + __builtin_offsetof(player_screen_flash, intensity) == 0x3c) ? 1 : -1];
typedef char check_pe_flash_col[(0x18 + __builtin_offsetof(player_screen_flash, color) == 0x40) ? 1 : -1];
typedef char check_pe_imp_min[(0x50 + __builtin_offsetof(player_camera_impulse, magnitude_minimum) == 0x60) ? 1 : -1];
typedef char check_pe_imp_int[(0x50 + __builtin_offsetof(player_camera_impulse, intensity) == 0x68) ? 1 : -1];
typedef char check_pe_shake_int[(0x84 + __builtin_offsetof(player_camera_shake, intensity) == 0xac) ? 1 : -1];
typedef char check_pe_shake_20[(0x84 + __builtin_offsetof(player_camera_shake, unknown_20) == 0xa4) ? 1 : -1];
// the scripted block player_effect_build_screen_flash 0x457000 reads at base + 0xec and 0xfc
typedef char check_peg_color[(__builtin_offsetof(player_effect_globals, scripted_flash_color) == 0x0ec) ? 1 : -1];
typedef char check_peg_start[(__builtin_offsetof(player_effect_globals, scripted_flash_start_tick) == 0x0f8) ? 1 : -1];
typedef char check_peg_ticks[(__builtin_offsetof(player_effect_globals, scripted_flash_ticks) == 0x0fc) ? 1 : -1];
typedef char check_peg_fade[(__builtin_offsetof(player_effect_globals, scripted_flash_fade_in) == 0x0fe) ? 1 : -1];
typedef char check_peg_rot[(__builtin_offsetof(player_effect_globals, scripted_shake_rotation) == 0x100) ? 1 : -1];
typedef char check_peg_trans[(__builtin_offsetof(player_effect_globals, scripted_shake_translation) == 0x10c) ? 1 : -1];
typedef char check_peg_int[(__builtin_offsetof(player_effect_globals, scripted_shake_intensity) == 0x118) ? 1 : -1];
typedef char check_peg_sticks[(__builtin_offsetof(player_effect_globals, scripted_shake_ticks) == 0x11c) ? 1 : -1];
typedef char check_peg_sflags[(__builtin_offsetof(player_effect_globals, scripted_shake_flags) == 0x120) ? 1 : -1];

// the tag offsets every attribution above rests on
typedef char check_tag_contrail[(sizeof(Contrail) == 0x144) ? 1 : -1];
typedef char check_tag_contrail_rate[(__builtin_offsetof(Contrail, point_generation_rate) == 0x04) ? 1 : -1];
typedef char check_tag_contrail_vel[(__builtin_offsetof(Contrail, point_velocity) == 0x08) ? 1 : -1];
typedef char check_tag_contrail_cone[(__builtin_offsetof(Contrail, point_velocity_cone_angle) == 0x10) ? 1 : -1];
typedef char check_tag_contrail_inherit[(__builtin_offsetof(Contrail, inherited_velocity_fraction) == 0x14) ? 1 : -1];
typedef char check_tag_contrail_anim_u[(__builtin_offsetof(Contrail, texture_animation_u) == 0x24) ? 1 : -1];
typedef char check_tag_contrail_anim_v[(__builtin_offsetof(Contrail, texture_animation_v) == 0x28) ? 1 : -1];
typedef char check_tag_contrail_rate2[(__builtin_offsetof(Contrail, animation_rate) == 0x2c) ? 1 : -1];
typedef char check_tag_contrail_seq[(__builtin_offsetof(Contrail, first_sequence_index) == 0x40) ? 1 : -1];
typedef char check_tag_contrail_states[(__builtin_offsetof(Contrail, point_states) == 0x138) ? 1 : -1];
typedef char check_tag_cps[(sizeof(ContrailPointState) == 0x68) ? 1 : -1];
typedef char check_tag_cps_flags[(__builtin_offsetof(ContrailPointState, scale_flags) == 0x64) ? 1 : -1];
typedef char check_tag_cps_width[(__builtin_offsetof(ContrailPointState, width) == 0x40) ? 1 : -1];
typedef char check_tag_cps_trans[(__builtin_offsetof(ContrailPointState, transition_duration) == 0x08) ? 1 : -1];
typedef char check_tag_decal[(sizeof(Decal) == 0x10c) ? 1 : -1];
typedef char check_tag_decal_layer[(__builtin_offsetof(Decal, layer) == 0x04) ? 1 : -1];
typedef char check_tag_decal_int[(__builtin_offsetof(Decal, intensity) == 0x2c) ? 1 : -1];
typedef char check_tag_decal_life[(__builtin_offsetof(Decal, lifetime) == 0x78) ? 1 : -1];
typedef char check_tag_decal_decay[(__builtin_offsetof(Decal, decay_time) == 0x80) ? 1 : -1];
typedef char check_tag_decal_map[(__builtin_offsetof(Decal, map) == 0xd8) ? 1 : -1];
typedef char check_tag_effect[(sizeof(Effect) == 0x40) ? 1 : -1];
typedef char check_tag_effect_loop[(__builtin_offsetof(Effect, loop_start_event) == 0x04) ? 1 : -1];
typedef char check_tag_effect_radius[(__builtin_offsetof(Effect, maximum_damage_radius) == 0x08) ? 1 : -1];
typedef char check_tag_effect_locs[(__builtin_offsetof(Effect, locations) == 0x28) ? 1 : -1];
typedef char check_tag_effect_events[(__builtin_offsetof(Effect, events) == 0x34) ? 1 : -1];
typedef char check_tag_event[(sizeof(EffectEvent) == 0x44) ? 1 : -1];
typedef char check_tag_event_skip[(__builtin_offsetof(EffectEvent, skip_fraction) == 0x04) ? 1 : -1];
typedef char check_tag_event_dur[(__builtin_offsetof(EffectEvent, duration_bounds) == 0x10) ? 1 : -1];
typedef char check_tag_event_parts[(__builtin_offsetof(EffectEvent, parts) == 0x2c) ? 1 : -1];
typedef char check_tag_event_particles[(__builtin_offsetof(EffectEvent, particles) == 0x38) ? 1 : -1];
typedef char check_tag_part[(sizeof(EffectPart) == 0x68) ? 1 : -1];
typedef char check_tag_part_class[(__builtin_offsetof(EffectPart, type_class) == 0x14) ? 1 : -1];
typedef char check_tag_part_type[(__builtin_offsetof(EffectPart, type) == 0x18) ? 1 : -1];
typedef char check_tag_eparticle[(sizeof(EffectParticle) == 0xe8) ? 1 : -1];
typedef char check_tag_eparticle_flags[(__builtin_offsetof(EffectParticle, flags) == 0x64) ? 1 : -1];
typedef char check_tag_eparticle_count[(__builtin_offsetof(EffectParticle, count) == 0x6c) ? 1 : -1];
typedef char check_tag_eparticle_a[(__builtin_offsetof(EffectParticle, a_scales_values) == 0xe0) ? 1 : -1];
typedef char check_tag_location[(sizeof(EffectLocation) == 0x20) ? 1 : -1];
typedef char check_tag_pctl[(sizeof(ParticleSystem) == 0x68) ? 1 : -1];
typedef char check_tag_pctl_update[(__builtin_offsetof(ParticleSystem, system_update_physics) == 0x48) ? 1 : -1];
typedef char check_tag_pctl_types[(__builtin_offsetof(ParticleSystem, particle_types) == 0x5c) ? 1 : -1];
typedef char check_tag_pst[(sizeof(ParticleSystemType) == 0x80) ? 1 : -1];
typedef char check_tag_pst_flags[(__builtin_offsetof(ParticleSystemType, flags) == 0x20) ? 1 : -1];
typedef char check_tag_pst_mode[(__builtin_offsetof(ParticleSystemType, complex_sprite_render_mode) == 0x28) ? 1 : -1];
typedef char check_tag_pst_states[(__builtin_offsetof(ParticleSystemType, states) == 0x68) ? 1 : -1];
typedef char check_tag_pst_pstates[(__builtin_offsetof(ParticleSystemType, particle_states) == 0x74) ? 1 : -1];
typedef char check_tag_states[(sizeof(ParticleSystemTypeStates) == 0xc0) ? 1 : -1];
typedef char check_tag_states_dur[(__builtin_offsetof(ParticleSystemTypeStates, duration_bounds) == 0x20) ? 1 : -1];
typedef char check_tag_states_scale[(__builtin_offsetof(ParticleSystemTypeStates, scale_multiplier) == 0x34) ? 1 : -1];
typedef char check_tag_states_rate[(__builtin_offsetof(ParticleSystemTypeStates, particle_creation_rate) == 0x58) ? 1 : -1];
typedef char check_tag_states_phys[(__builtin_offsetof(ParticleSystemTypeStates, particle_creation_physics) == 0xb0) ? 1 : -1];
typedef char check_tag_states_upd[(__builtin_offsetof(ParticleSystemTypeStates, particle_update_physics) == 0xb2) ? 1 : -1];
typedef char check_tag_pps[(sizeof(ParticleSystemTypeParticleState) == 0x178) ? 1 : -1];
typedef char check_tag_pps_seq[(__builtin_offsetof(ParticleSystemTypeParticleState, sequence_index) == 0x40) ? 1 : -1];
typedef char check_tag_pps_scale[(__builtin_offsetof(ParticleSystemTypeParticleState, scale) == 0x48) ? 1 : -1];
typedef char check_tag_pps_anim[(__builtin_offsetof(ParticleSystemTypeParticleState, animation_rate) == 0x50) ? 1 : -1];
typedef char check_tag_pps_rot[(__builtin_offsetof(ParticleSystemTypeParticleState, rotation_rate) == 0x58) ? 1 : -1];
typedef char check_tag_pps_c1[(__builtin_offsetof(ParticleSystemTypeParticleState, color_1) == 0x60) ? 1 : -1];
typedef char check_tag_pps_c2[(__builtin_offsetof(ParticleSystemTypeParticleState, color_2) == 0x70) ? 1 : -1];
typedef char check_tag_particle[(sizeof(Particle) == 0x164) ? 1 : -1];
typedef char check_tag_particle_bitmap[(__builtin_offsetof(Particle, bitmap) == 0x04) ? 1 : -1];
typedef char check_tag_particle_phys[(__builtin_offsetof(Particle, physics) == 0x14) ? 1 : -1];
typedef char check_tag_particle_life[(__builtin_offsetof(Particle, lifespan) == 0x38) ? 1 : -1];
typedef char check_tag_particle_coll[(__builtin_offsetof(Particle, collision_effect) == 0x48) ? 1 : -1];
typedef char check_tag_particle_death[(__builtin_offsetof(Particle, death_effect) == 0x58) ? 1 : -1];
typedef char check_tag_particle_radius[(__builtin_offsetof(Particle, radius_animation) == 0x74) ? 1 : -1];
typedef char check_tag_particle_rate[(__builtin_offsetof(Particle, animation_rate) == 0x80) ? 1 : -1];
typedef char check_tag_particle_det[(__builtin_offsetof(Particle, contact_deterioration) == 0x88) ? 1 : -1];
typedef char check_tag_particle_seq[(__builtin_offsetof(Particle, first_sequence_index) == 0x98) ? 1 : -1];
typedef char check_tag_particle_final[(__builtin_offsetof(Particle, final_sequence_count) == 0x9e) ? 1 : -1];
typedef char check_tag_weather[(sizeof(WeatherParticleSystem) == 0x30) ? 1 : -1];
typedef char check_tag_weather_types[(__builtin_offsetof(WeatherParticleSystem, particle_types) == 0x24) ? 1 : -1];
typedef char check_tag_wtype[(sizeof(WeatherParticleSystemParticleType) == 0x25c) ? 1 : -1];
typedef char check_tag_wtype_flags[(__builtin_offsetof(WeatherParticleSystemParticleType, flags) == 0x20) ? 1 : -1];
typedef char check_tag_wtype_fadeout[(__builtin_offsetof(WeatherParticleSystemParticleType, fade_out_end_distance) == 0x30) ? 1 : -1];
typedef char check_tag_wtype_count[(__builtin_offsetof(WeatherParticleSystemParticleType, particle_count) == 0xa4) ? 1 : -1];
typedef char check_tag_wtype_phys[(__builtin_offsetof(WeatherParticleSystemParticleType, physics) == 0xac) ? 1 : -1];
typedef char check_tag_wtype_accel[(__builtin_offsetof(WeatherParticleSystemParticleType, acceleration_magnitude) == 0xcc) ? 1 : -1];
typedef char check_tag_wtype_radius[(__builtin_offsetof(WeatherParticleSystemParticleType, particle_radius) == 0xfc) ? 1 : -1];
typedef char check_tag_wtype_anim[(__builtin_offsetof(WeatherParticleSystemParticleType, animation_rate) == 0x104) ? 1 : -1];
typedef char check_tag_wtype_rot[(__builtin_offsetof(WeatherParticleSystemParticleType, rotation_rate) == 0x10c) ? 1 : -1];
typedef char check_tag_wtype_lower[(__builtin_offsetof(WeatherParticleSystemParticleType, color_lower_bound) == 0x134) ? 1 : -1];
typedef char check_tag_wtype_upper[(__builtin_offsetof(WeatherParticleSystemParticleType, color_upper_bound) == 0x144) ? 1 : -1];
typedef char check_tag_wtype_bitmap[(__builtin_offsetof(WeatherParticleSystemParticleType, sprite_bitmap) == 0x194) ? 1 : -1];
typedef char check_tag_attach[(sizeof(ObjectAttachment) == 0x48) ? 1 : -1];
typedef char check_tag_attach_marker[(__builtin_offsetof(ObjectAttachment, marker) == 0x10) ? 1 : -1];
typedef char check_tag_attach_primary[(__builtin_offsetof(ObjectAttachment, primary_scale) == 0x30) ? 1 : -1];
typedef char check_tag_attach_change[(__builtin_offsetof(ObjectAttachment, change_color) == 0x34) ? 1 : -1];
typedef char check_tag_cde_radius[(__builtin_offsetof(ContinuousDamageEffect, radius) == 0x00) ? 1 : -1];
typedef char check_tag_cde_lf[(__builtin_offsetof(ContinuousDamageEffect, low_frequency_vibrate_frequency) == 0x24) ? 1 : -1];
typedef char check_tag_cde_hf[(__builtin_offsetof(ContinuousDamageEffect, high_frequency_vibrate_frequency) == 0x28) ? 1 : -1];
typedef char check_tag_cde_trans[(__builtin_offsetof(ContinuousDamageEffect, camera_shaking_random_translation) == 0x44) ? 1 : -1];
typedef char check_tag_cde_rot[(__builtin_offsetof(ContinuousDamageEffect, camera_shaking_random_rotation) == 0x48) ? 1 : -1];
typedef char check_tag_cde_period[(__builtin_offsetof(ContinuousDamageEffect, camera_shaking_wobble_period) == 0x5c) ? 1 : -1];
typedef char check_tag_cde_weight[(__builtin_offsetof(ContinuousDamageEffect, camera_shaking_wobble_weight) == 0x60) ? 1 : -1];

// the object fields this module reaches through
typedef char check_obj_velocity[(__builtin_offsetof(object, velocity) == 0x68) ? 1 : -1];
typedef char check_obj_leaf[(__builtin_offsetof(object, location_leaf_index) == 0x98) ? 1 : -1];
typedef char check_obj_parent[(__builtin_offsetof(object, parent_object) == 0x11c) ? 1 : -1];
typedef char check_obj_valid[(__builtin_offsetof(object, function_valid_flags) == 0x123) ? 1 : -1];
typedef char check_obj_out[(__builtin_offsetof(object, function_out_values) == 0x134) ? 1 : -1];
typedef char check_obj_handles[(__builtin_offsetof(object, attachment_handles) == 0x14c) ? 1 : -1];
typedef char check_obj_change[(__builtin_offsetof(object, change_colors) == 0x1b8) ? 1 : -1];
typedef char check_obj_nodes[(__builtin_offsetof(object, nodes) == 0x1f0) ? 1 : -1];

// enum values the module dispatches on
typedef char check_transition[(effectdistributionfunction_start == 0
                               && effectdistributionfunction_end == 1
                               && effectdistributionfunction_constant == 2
                               && effectdistributionfunction_buildup == 3
                               && effectdistributionfunction_falloff == 4
                               && effectdistributionfunction_buildup_and_falloff == 5) ? 1 : -1];
typedef char check_decal_layers_enum[(decallayer_primary == 0 && decallayer_water == 4) ? 1 : -1];
typedef char check_creation_physics[(particlesystemparticlecreationphysics_default == 0
                                     && particlesystemparticlecreationphysics_explosion == 1
                                     && particlesystemparticlecreationphysics_jet == 2) ? 1 : -1];
typedef char check_system_physics[(particlesystemsystemupdatephysics_default == 0
                                   && particlesystemsystemupdatephysics_explosion == 1) ? 1 : -1];
typedef char check_sequence_states[(_particle_sequence_state_new == 0
                                    && _particle_sequence_state_finished == 4) ? 1 : -1];
typedef char check_grid_span[(k_decal_layers * k_decal_grid_clusters * 4 == 0x2800) ? 1 : -1];
typedef char check_weather_span[(0x1c + k_maximum_weather_particle_types * 0x10 == 0x9c) ? 1 : -1];
typedef char check_pctl_span[(0x58 + k_maximum_particle_system_types * 0x40 == 0x158) ? 1 : -1];

// ---------------------------------------------------------------------------
// Types folded into types/effects.h by the phase-4 integration pass, out of the local copies
// that used to live in src/effects/decal_flood_surfaces.c, src/effects/decal_place.c,
// src/effects/effect_new_on_object_with_node_table.c and src/effects/effect_new_with_color.c.
// ---------------------------------------------------------------------------
typedef char check_flood_vertex[(sizeof(decal_flood_vertex_record) == 0x14) ? 1 : -1];
typedef char check_flood_vertex_u[(__builtin_offsetof(decal_flood_vertex_record, u) == 0x0c) ? 1 : -1];
typedef char check_flood_vertex_v[(__builtin_offsetof(decal_flood_vertex_record, v) == 0x10) ? 1 : -1];
typedef char check_flood_acc[(sizeof(decal_flood_accumulator) == 0x5804) ? 1 : -1];
typedef char check_flood_acc_count[(__builtin_offsetof(decal_flood_accumulator, vertex_count) == 0x5000) ? 1 : -1];
typedef char check_flood_acc_list[(__builtin_offsetof(decal_flood_accumulator, visited_surfaces) == 0x5002) ? 1 : -1];
typedef char check_flood_acc_n[(__builtin_offsetof(decal_flood_accumulator, visited_surface_count) == 0x5802) ? 1 : -1];
typedef char check_marker_ctx[(sizeof(effect_marker_node_context) == 0x18) ? 1 : -1];
typedef char check_marker_ctx_entry[(__builtin_offsetof(effect_marker_node_context, node_table_entry) == 0x04) ? 1 : -1];
typedef char check_marker_ctx_08[(__builtin_offsetof(effect_marker_node_context, unknown_08) == 0x08) ? 1 : -1];
typedef char check_marker_ctx_0c[(__builtin_offsetof(effect_marker_node_context, unknown_0c) == 0x0c) ? 1 : -1];
typedef char check_marker_ctx_10[(__builtin_offsetof(effect_marker_node_context, unknown_10) == 0x10) ? 1 : -1];
typedef char check_marker_ctx_14[(__builtin_offsetof(effect_marker_node_context, unknown_14) == 0x14) ? 1 : -1];
// The BSP tables decal_flood_surfaces walks come from types/tags.h, not from this header: the
// strides this module reads (0xc / 0x18 / 0x10) and the reflexive pointer offsets it indexes
// (+0x40 / +0x4c / +0x58 of structure_collision_bsp) have to agree with those tag structs.
typedef char check_bsp_surface_stride[(sizeof(ModelCollisionGeometryBSPSurface) == 0x0c) ? 1 : -1];
typedef char check_bsp_edge_stride[(sizeof(ModelCollisionGeometryBSPEdge) == 0x18) ? 1 : -1];
typedef char check_bsp_vertex_stride[(sizeof(ModelCollisionGeometryBSPVertex) == 0x10) ? 1 : -1];
typedef char check_bsp_surfaces_ptr[(__builtin_offsetof(ModelCollisionGeometryBSP, surfaces) + 4 == 0x40) ? 1 : -1];
typedef char check_bsp_edges_ptr[(__builtin_offsetof(ModelCollisionGeometryBSP, edges) + 4 == 0x4c) ? 1 : -1];
typedef char check_bsp_vertices_ptr[(__builtin_offsetof(ModelCollisionGeometryBSP, vertices) + 4 == 0x58) ? 1 : -1];
