#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"

#define CHECK(name, cond) typedef char check_##name[(cond) ? 1 : -1]
#define OFF(t, f) __builtin_offsetof(t, f)

// struct sizes
CHECK(render_view, sizeof(render_view) == 0xac);
CHECK(render_model_effect, sizeof(render_model_effect) == 0x28);
CHECK(object_render_data, sizeof(object_render_data) == 0x48);
CHECK(cached_object_render_state, sizeof(cached_object_render_state) == 0x100);
CHECK(rendered_particle_datum, sizeof(rendered_particle_datum) == 0x08);
CHECK(build_sprite_group, sizeof(build_sprite_group) == 0x10);
CHECK(build_sprite_data, sizeof(build_sprite_data) == 0xa4);
CHECK(cinematic_screen_effect_globals, sizeof(cinematic_screen_effect_globals) == 0x78);
CHECK(rasterizer_frame_statistics, sizeof(rasterizer_frame_statistics) == 0x18);
CHECK(frame_graph, sizeof(frame_graph) == 0x32b0);

// the rasterizer.h records this module relies on
CHECK(render_camera_size, sizeof(render_camera) == 0x54);
CHECK(render_frustum_size, sizeof(render_frustum) == 0x18c);
CHECK(render_fog_size, sizeof(render_fog) == 0x50);
CHECK(render_lighting_size, sizeof(render_lighting) == 0x74);
CHECK(window_parameters_size, sizeof(rasterizer_window_parameters) == 0x258);
CHECK(screen_vertex_size, sizeof(rasterizer_dynamic_screen_vertex) == 0x18);
CHECK(matrix4x3_size, sizeof(real_matrix4x3) == 0x34);

// render_view (render_frame_all_views 0x4c9260, render_player_frame 0x50ba80)
CHECK(rv_nonplayer, OFF(render_view, nonplayer) == 0x02);
CHECK(rv_source, OFF(render_view, source_camera) == 0x04);
CHECK(rv_source_z_near, OFF(render_view, source_camera.z_near) == 0x40);
CHECK(rv_source_z_far, OFF(render_view, source_camera.z_far) == 0x44);
CHECK(rv_rast, OFF(render_view, rasterizer_camera) == 0x58);
CHECK(rv_rast_forward, OFF(render_view, rasterizer_camera.forward) == 0x64);
CHECK(rv_rast_up, OFF(render_view, rasterizer_camera.up) == 0x70);
CHECK(rv_rast_mirrored, OFF(render_view, rasterizer_camera.mirrored) == 0x7c);
CHECK(rv_rast_fov, OFF(render_view, rasterizer_camera.vertical_field_of_view) == 0x80);
CHECK(rv_rast_viewport, OFF(render_view, rasterizer_camera.viewport_bounds) == 0x84);
CHECK(rv_rast_z_near, OFF(render_view, rasterizer_camera.z_near) == 0x94);
CHECK(rv_rast_z_far, OFF(render_view, rasterizer_camera.z_far) == 0x98);

// render_model_effect (render_object_list 0x50ee20 stack block local_28)
CHECK(rme_37c, OFF(render_model_effect, unit_37c) == 0x04);
CHECK(rme_object, OFF(render_model_effect, object_index) == 0x0c);
CHECK(rme_centroid, OFF(render_model_effect, centroid) == 0x10);
CHECK(rme_shader, OFF(render_model_effect, modifier_shader) == 0x1c);
CHECK(rme_colors, OFF(render_model_effect, change_colors) == 0x20);
CHECK(rme_functions, OFF(render_model_effect, function_values) == 0x24);

// object_render_data (0x50eba0 EDI, 0x50f830 EAX, 0x50f980 ECX)
CHECK(ord_lighting, OFF(object_render_data, lighting) == 0x04);
CHECK(ord_shadow, OFF(object_render_data, shadow_pass) == 0x08);
CHECK(ord_fog, OFF(object_render_data, outside_fog_plane) == 0x09);
CHECK(ord_matrix, OFF(object_render_data, shadow_matrix) == 0x0c);
CHECK(ord_forward, OFF(object_render_data, shadow_matrix.forward) == 0x10);
CHECK(ord_left, OFF(object_render_data, shadow_matrix.left) == 0x1c);
CHECK(ord_up, OFF(object_render_data, shadow_matrix.up) == 0x28);
CHECK(ord_position, OFF(object_render_data, shadow_matrix.position) == 0x34);
CHECK(ord_radius, OFF(object_render_data, shadow_radius) == 0x40);
CHECK(ord_44, OFF(object_render_data, unknown_44) == 0x44);

// cached_object_render_state (0x50f150, 0x50f270, 0x50ea00)
CHECK(cors_object, OFF(cached_object_render_state, object_index) == 0x04);
CHECK(cors_sample, OFF(cached_object_render_state, last_sample_frame) == 0x08);
CHECK(cors_window, OFF(cached_object_render_state, last_update_window) == 0x0c);
CHECK(cors_frame, OFF(cached_object_render_state, last_update_frame) == 0x10);
CHECK(cors_lighting, OFF(cached_object_render_state, lighting) == 0x14);
CHECK(cors_point_count, OFF(cached_object_render_state, lighting.point_light_count) == 0x54);
CHECK(cors_point_indices, OFF(cached_object_render_state, lighting.point_light_indices) == 0x58);
CHECK(cors_desired, OFF(cached_object_render_state, desired_lighting) == 0x88);
CHECK(cors_desired_count, OFF(cached_object_render_state, desired_lighting.point_light_count) == 0xc8);
CHECK(cors_lod, OFF(cached_object_render_state, level_of_detail_pixels) == 0xfc);
// the members 0x50f270 steps (offsets inside render_lighting)
CHECK(rl_reflection, OFF(render_lighting, reflection_tint) == 0x4c);
CHECK(rl_distant1_color, OFF(render_lighting, distant_lights[1].color) == 0x28);
CHECK(rl_distant1_dir, OFF(render_lighting, distant_lights[1].direction) == 0x34);
CHECK(rl_shadow_vector, OFF(render_lighting, shadow_vector) == 0x5c);
CHECK(rl_shadow_color, OFF(render_lighting, shadow_color) == 0x68);

// rendered_particle_datum (0x50fd90 and the sort)
CHECK(rpd_definition, OFF(rendered_particle_datum, definition_index) == 0x02);
CHECK(rpd_cluster, OFF(rendered_particle_datum, cluster_index) == 0x04);
CHECK(rpd_first_person, OFF(rendered_particle_datum, first_person) == 0x06);

// build_sprite_data (0x511520, 0x511620, 0x511700; 0x50fd90 locals 0x24ac..0x248c)
CHECK(bsd_max, OFF(build_sprite_data, maximum_sprite_count) == 0x04);
CHECK(bsd_shader, OFF(build_sprite_data, shader) == 0x08);
CHECK(bsd_count, OFF(build_sprite_data, sprite_count) == 0x0c);
CHECK(bsd_flags, OFF(build_sprite_data, flags) == 0x10);
CHECK(bsd_centroid, OFF(build_sprite_data, centroid) == 0x14);
CHECK(bsd_group_count, OFF(build_sprite_data, group_count) == 0x20);
CHECK(bsd_groups, OFF(build_sprite_data, groups) == 0x24);
CHECK(bsd_group0_vertices, OFF(build_sprite_data, groups[0].vertices) == 0x28);
CHECK(bsd_group0_count, OFF(build_sprite_data, groups[0].quad_count) == 0x2c);
CHECK(bsd_group0_bitmap, OFF(build_sprite_data, groups[0].bitmap) == 0x30);
CHECK(bsd_span, 0x24 + k_maximum_build_sprite_groups * 0x10 == 0xa4);

// cinematic_screen_effect_globals (0x5121d0, 0x512230, 0x5122a0, 0x512360, hs 0x4810f0..)
CHECK(cse_radius, OFF(cinematic_screen_effect_globals, convolution_radius) == 0x04);
CHECK(cse_08, OFF(cinematic_screen_effect_globals, unknown_08) == 0x08);
CHECK(cse_light, OFF(cinematic_screen_effect_globals, filter_light_enhancement_intensity) == 0x0c);
CHECK(cse_desat, OFF(cinematic_screen_effect_globals, filter_desaturation_intensity) == 0x10);
CHECK(cse_tint, OFF(cinematic_screen_effect_globals, filter_desaturation_tint) == 0x14);
CHECK(cse_additive, OFF(cinematic_screen_effect_globals, filter_desaturation_is_additive) == 0x20);
CHECK(cse_video, OFF(cinematic_screen_effect_globals, video_enabled) == 0x23);
CHECK(cse_overbright, OFF(cinematic_screen_effect_globals, video_overbright_mode) == 0x24);
CHECK(cse_scanline, OFF(cinematic_screen_effect_globals, video_scanline_map) == 0x28);
CHECK(cse_noise, OFF(cinematic_screen_effect_globals, video_noise_intensity) == 0x2c);
CHECK(cse_30, OFF(cinematic_screen_effect_globals, unknown_30) == 0x30);
CHECK(cse_noise_map, OFF(cinematic_screen_effect_globals, video_noise_map) == 0x34);
CHECK(cse_active, OFF(cinematic_screen_effect_globals, active) == 0x38);
CHECK(cse_initialized, OFF(cinematic_screen_effect_globals, initialized) == 0x39);
CHECK(cse_conv_lower, OFF(cinematic_screen_effect_globals, convolution_radius_lower_bound) == 0x3c);
CHECK(cse_conv_start, OFF(cinematic_screen_effect_globals, convolution_start_time) == 0x44);
CHECK(cse_conv_end, OFF(cinematic_screen_effect_globals, convolution_end_time) == 0x48);
CHECK(cse_light_lower, OFF(cinematic_screen_effect_globals, filter_light_enhancement_intensity_lower_bound) == 0x4c);
CHECK(cse_desat_lower, OFF(cinematic_screen_effect_globals, filter_desaturation_intensity_lower_bound) == 0x54);
CHECK(cse_filter_start, OFF(cinematic_screen_effect_globals, filter_start_time) == 0x5c);
CHECK(cse_filter_end, OFF(cinematic_screen_effect_globals, filter_end_time) == 0x60);
CHECK(cse_values, OFF(cinematic_screen_effect_globals, script_values) == 0x64);
CHECK(cse_near_clip, OFF(cinematic_screen_effect_globals, near_clip_distance) == 0x74);

// rasterizer_frame_statistics (0x512530 EBX = 0x007c30a0; 0x512e80 reads 0x7c30a0..0x7c30b4)
CHECK(rfs_count, OFF(rasterizer_frame_statistics, sample_count) == 0x04);
CHECK(rfs_average, OFF(rasterizer_frame_statistics, average_framerate) == 0x08);
CHECK(rfs_minimum, OFF(rasterizer_frame_statistics, minimum_framerate) == 0x0c);
CHECK(rfs_maximum, OFF(rasterizer_frame_statistics, maximum_framerate) == 0x10);
CHECK(rfs_dropped, OFF(rasterizer_frame_statistics, dropped_percentage) == 0x14);

// frame_graph (base 0x006b9260)
CHECK(fg_name_bounds, OFF(frame_graph, name_bounds) == 0x6b9268 - 0x6b9260);
CHECK(fg_max_bounds, OFF(frame_graph, maximum_bounds) == 0x6b9270 - 0x6b9260);
CHECK(fg_avg_bounds, OFF(frame_graph, average_bounds) == 0x6b9278 - 0x6b9260);
CHECK(fg_vertices, OFF(frame_graph, vertices) == 0x6b9280 - 0x6b9260);
CHECK(fg_vertex1_y, OFF(frame_graph, vertices[1].y) == 0x6b929c - 0x6b9260);
CHECK(fg_last_y, OFF(frame_graph, vertices[0x1ff].y) == 0x6bc26c - 0x6b9260);
CHECK(fg_frame, OFF(frame_graph, frame_vertices) == 0x6bc280 - 0x6b9260);
CHECK(fg_frame4_color, OFF(frame_graph, frame_vertices[4].color) == 0x6bc2ec - 0x6b9260);
CHECK(fg_maximum, OFF(frame_graph, maximum) == 0x6bc2f8 - 0x6b9260);
CHECK(fg_average, OFF(frame_graph, average) == 0x6bc2fc - 0x6b9260);
CHECK(fg_recent, OFF(frame_graph, recent_samples) == 0x6bc300 - 0x6b9260);
CHECK(fg_name, OFF(frame_graph, name) == 0x6bc310 - 0x6b9260);
CHECK(fg_end, 0x6b9260 + sizeof(frame_graph) == 0x6bc510);

// the global block layout of 0x007c3100..0x007c3344
CHECK(render_camera_block, 0x7c3114 + sizeof(render_camera) == 0x7c3168);
CHECK(render_frustum_block, 0x7c3168 + sizeof(render_frustum) == 0x7c32f4);
CHECK(render_fog_block, 0x7c32f4 + sizeof(render_fog) == 0x7c3344);
CHECK(camera_mirrored_global, 0x7c3114 + OFF(render_camera, mirrored) == 0x7c3138);
CHECK(frustum_world_to_view, 0x7c3168 + OFF(render_frustum, world_to_view) == 0x7c3178);
CHECK(frustum_view_to_world, 0x7c3168 + OFF(render_frustum, view_to_world) == 0x7c31ac);
CHECK(frustum_projection_scale, 0x7c3168 + OFF(render_frustum, projection_world_to_screen) == 0x7c32ec);
CHECK(fog_planar_mode, 0x7c32f4 + OFF(render_fog, planar_mode) == 0x7c3310);
CHECK(fog_plane, 0x7c32f4 + OFF(render_fog, plane) == 0x7c3314);
CHECK(fog_planar_distance, 0x7c32f4 + OFF(render_fog, planar_maximum_distance) == 0x7c3334);
CHECK(rendered_objects_span, 0x6b8dc4 + k_maximum_rendered_objects * 4 == 0x6b91c4);
CHECK(uncached_lighting_span, 0x6b91c8 + sizeof(render_lighting) == 0x6b923c);
CHECK(frame_statistics_span, 0x71cfe8 + k_frame_statistics_history * 4 == 0x71d0d8);
CHECK(frame_dropped_span, 0x71d0d8 + k_frame_statistics_history == 0x71d114);
CHECK(render_views_span, 0x719b70 + k_maximum_render_views * sizeof(render_view) == 0x719cc8);

int main(void) { return 0; }
