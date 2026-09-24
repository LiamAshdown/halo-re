#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#define CHECK(name, cond) typedef char check_##name[(cond) ? 1 : -1]
#define OFF(t, f) __builtin_offsetof(t, f)

// struct sizes
CHECK(vertex_buffer, sizeof(rasterizer_vertex_buffer) == 0x14);
CHECK(index_buffer, sizeof(rasterizer_index_buffer) == 0x10);
CHECK(vertex_declaration, sizeof(rasterizer_vertex_declaration) == 0x0c);
CHECK(vertex_buffer_slot, sizeof(rasterizer_vertex_buffer_slot) == 0x14);
CHECK(dynamic_vertex_cache, sizeof(rasterizer_dynamic_vertex_cache) == 0x0c);
CHECK(dynamic_vertex_slot, sizeof(rasterizer_dynamic_vertex_slot) == 0x10);
CHECK(dynamic_index_slot, sizeof(rasterizer_dynamic_index_slot) == 0x0c);
CHECK(effect_slot, sizeof(rasterizer_effect_slot) == 0x20);
CHECK(vertex_shader, sizeof(rasterizer_vertex_shader) == 0x08);
CHECK(render_target, sizeof(rasterizer_render_target) == 0x14);
CHECK(present_parameters, sizeof(d3d_present_parameters) == 0x38);
CHECK(caps9, sizeof(d3d_caps9) == 0x130);
CHECK(gamma_ramp, sizeof(d3d_gamma_ramp) == 0x600);
CHECK(display_mode, sizeof(rasterizer_display_mode) == 0x10);
CHECK(render_camera, sizeof(render_camera) == 0x54);
CHECK(render_frustum, sizeof(render_frustum) == 0x18c);
CHECK(render_fog, sizeof(render_fog) == 0x50);
CHECK(render_screen_flash, sizeof(render_screen_flash) == 0x18);
CHECK(window_parameters, sizeof(rasterizer_window_parameters) == 0x258);
CHECK(frame_time, sizeof(rasterizer_frame_time) == 0x10);
CHECK(light, sizeof(rasterizer_light) == 0x38);
CHECK(point_light_constants, sizeof(rasterizer_point_light_constants) == 0x30);
CHECK(projected_light_constants, sizeof(rasterizer_projected_light_constants) == 0x50);
CHECK(distant_light, sizeof(render_distant_light) == 0x18);
CHECK(render_lighting, sizeof(render_lighting) == 0x74);
CHECK(skinning_matrix, sizeof(rasterizer_skinning_matrix) == 0x30);
CHECK(group_parameters, sizeof(rasterizer_geometry_group_parameters) == 0x28);
CHECK(model_draw_context, sizeof(rasterizer_model_draw_context) == 0xcc);
CHECK(group, sizeof(transparent_geometry_group) == 0xa8);
CHECK(lens_flare_instance, sizeof(lens_flare_instance) == 0x28);
CHECK(lens_flare_object_visibility, sizeof(lens_flare_object_visibility) == 0x0a);
CHECK(lens_flare_vertex, sizeof(lens_flare_vertex) == 0x20);
CHECK(lens_flare_batch_key, sizeof(lens_flare_batch_key) == 0x10);
CHECK(lens_flare_batch, sizeof(lens_flare_batch) == 0x18018);
CHECK(glyph_entry, sizeof(font_glyph_cache_entry) == 0x08);
CHECK(glyph_cache, sizeof(font_glyph_cache) == 0x1010);

// pool and table spans against the allocation sizes and neighbouring globals
CHECK(group_pool, k_rasterizer_maximum_transparent_groups * sizeof(transparent_geometry_group) == 0xfc00);
CHECK(group_pool2, k_rasterizer_maximum_secondary_groups * sizeof(transparent_geometry_group) == 0x1500);
CHECK(declarations, k_rasterizer_vertex_type_count * sizeof(rasterizer_vertex_declaration) == 0x3c * 4);
CHECK(vertex_caches, 0x006d98e8 + k_rasterizer_vertex_type_count * sizeof(rasterizer_dynamic_vertex_cache) == 0x006d99d8);
CHECK(vertex_slots, 0x006d99d8 + k_rasterizer_dynamic_vertex_slots * sizeof(rasterizer_dynamic_vertex_slot) == 0x006dd9d8);
CHECK(index_slots, 0x006dd9e0 + k_rasterizer_dynamic_vertex_slots * sizeof(rasterizer_dynamic_index_slot) == 0x006e09e0);
CHECK(vb_slots, 0x007bf060 + k_rasterizer_vertex_buffer_slots * sizeof(rasterizer_vertex_buffer_slot) == 0x007c0460);
CHECK(effects, 0x0069d410 + k_rasterizer_pixel_shader_effects * sizeof(rasterizer_effect_slot) == 0x0069e350);
CHECK(vshaders, 0x0069e350 + k_rasterizer_vertex_shaders * sizeof(rasterizer_vertex_shader) == 0x0069e550);
CHECK(render_targets, 0x0069d358 + k_rasterizer_render_targets * sizeof(rasterizer_render_target) == 0x0069d40c);
CHECK(caps_end, 0x007c10c0 + sizeof(d3d_caps9) == 0x007c11f0);
CHECK(caps_ps, 0x007c10c0 + OFF(d3d_caps9, pixel_shader_version) == 0x007c118c);
CHECK(caps_streams, 0x007c10c0 + OFF(d3d_caps9, max_streams) == 0x007c117c);
CHECK(caps_textures, 0x007c10c0 + OFF(d3d_caps9, max_simultaneous_textures) == 0x007c1158);
CHECK(caps_lights, 0x007c10c0 + OFF(d3d_caps9, max_active_lights) == 0x007c1160);
CHECK(caps_raster, 0x007c10c0 + OFF(d3d_caps9, raster_caps) == 0x007c10e4);
CHECK(caps_texture, 0x007c10c0 + OFF(d3d_caps9, texture_caps) == 0x007c10fc);
CHECK(caps_address, 0x007c10c0 + OFF(d3d_caps9, texture_address_caps) == 0x007c110c);
CHECK(caps_aniso, 0x007c10c0 + OFF(d3d_caps9, max_anisotropy) == 0x007c112c);
CHECK(pp_interval, 0x007c04a0 + OFF(d3d_present_parameters, presentation_interval) == 0x007c04d4);
CHECK(pp_refresh, 0x007c04a0 + OFF(d3d_present_parameters, fullscreen_refresh_rate) == 0x007c04d0);
CHECK(palette, 0x007c04e0 + k_rasterizer_maximum_skinning_nodes * sizeof(rasterizer_skinning_matrix) == 0x007c10b0);
CHECK(lights, 0x007c1484 + k_rasterizer_maximum_lights * sizeof(rasterizer_light) == 0x007c3084);
CHECK(flare_instances, 0x006ce818 + k_lens_flare_maximum_instances * sizeof(lens_flare_instance) <= 0x006d8828);
CHECK(flare_objects, 0x006bc510 + k_lens_flare_object_visibility_slots * sizeof(lens_flare_object_visibility) == 0x006be810);
CHECK(flare_markers, 0x006be810 + k_lens_flare_marker_visibility_size == 0x006ce818);
CHECK(flare_batches, 0x00746fc0 + k_lens_flare_batch_slots * sizeof(lens_flare_batch) == 0x007bf038);
CHECK(flare_queries, 0x006e1dc8 + k_lens_flare_occlusion_queries * 4 == 0x006e2dc8);
CHECK(glyph_cache_end, 0x006d8828 + sizeof(font_glyph_cache) == 0x006d9838);
CHECK(immediate_group, 0x006e0a70 + OFF(transparent_geometry_group, sorted_index) == 0x006e0b08);
CHECK(immediate_group2, 0x006e1828 + OFF(transparent_geometry_group, sorted_index) == 0x006e18c0);
CHECK(gamma, 0x006e0b18 + 2 * sizeof(d3d_gamma_ramp) == 0x006e1718);
CHECK(immediate_to_gamma, 0x006e0a70 + sizeof(transparent_geometry_group) == 0x006e0b18);

// window parameters, against the absolute globals the module reads
#define W(f) (0x007c1220 + OFF(rasterizer_window_parameters, f))
CHECK(w_window, W(window_index) == 0x007c1222);
CHECK(w_flag4, W(unknown_04) == 0x007c1224);
CHECK(w_position, W(camera.position) == 0x007c1228);
CHECK(w_forward, W(camera.forward) == 0x007c1234);
CHECK(w_viewport, W(camera.viewport_bounds) == 0x007c1254);
CHECK(w_zfar, W(camera.z_far) == 0x007c1268);
CHECK(w_world_to_view, W(frustum.world_to_view) == 0x007c128c);
CHECK(w_view_forward, W(frustum.view_to_world.forward) == 0x007c12c4);
CHECK(w_view_left, W(frustum.view_to_world.left) == 0x007c12d0);
CHECK(w_projection, W(frustum.projection) == 0x007c13c0);
CHECK(w_fog, W(fog) == 0x007c1408);
CHECK(w_fog_color, W(fog.atmospheric_color) == 0x007c140c);
CHECK(w_fog_density, W(fog.atmospheric_maximum_density) == 0x007c1418);
CHECK(w_fog_min, W(fog.atmospheric_minimum_distance) == 0x007c141c);
CHECK(w_fog_max, W(fog.atmospheric_maximum_distance) == 0x007c1420);
CHECK(w_fog_mode, W(fog.planar_mode) == 0x007c1424);
CHECK(w_fog_plane, W(fog.plane) == 0x007c1428);
CHECK(w_fog_pcolor, W(fog.planar_color) == 0x007c1438);
CHECK(w_fog_pdensity, W(fog.planar_maximum_density) == 0x007c1444);
CHECK(w_fog_pdist, W(fog.planar_maximum_distance) == 0x007c1448);
CHECK(w_fog_pdepth, W(fog.planar_maximum_depth) == 0x007c144c);
CHECK(w_flash, W(screen_flash.type) == 0x007c1458);
CHECK(w_flash_intensity, W(screen_flash.intensity) == 0x007c145c);
CHECK(w_flash_color, W(screen_flash.color) == 0x007c1460);
CHECK(w_end, 0x007c1220 + sizeof(rasterizer_window_parameters) == 0x007c1478);
CHECK(w_camera_dwords, sizeof(render_camera) == 0x15 * 4);
CHECK(w_fog_dwords, sizeof(render_fog) == 0x14 * 4);

// lights, against the table addresses read at stride 0x38
#define L(f) (0x007c1484 + OFF(rasterizer_light, f))
CHECK(l_position, L(position) == 0x007c1488);
CHECK(l_forward, L(forward) == 0x007c1494);
CHECK(l_up, L(up) == 0x007c14a0);
CHECK(l_color, L(color) == 0x007c14ac);
CHECK(l_radius, L(radius) == 0x007c14b8);

// transparent geometry group
CHECK(g_sort_key, OFF(transparent_geometry_group, sort_key) == 0x08);
CHECK(g_shader, OFF(transparent_geometry_group, shader) == 0x0c);
CHECK(g_perm, OFF(transparent_geometry_group, shader_permutation) == 0x10);
CHECK(g_mode, OFF(transparent_geometry_group, parameters.mode) == 0x14);
CHECK(g_blend, OFF(transparent_geometry_group, parameters.blend_factor) == 0x18);
CHECK(g_distort, OFF(transparent_geometry_group, parameters.distortion_factor) == 0x1c);
CHECK(g_3c, OFF(transparent_geometry_group, unknown_3c) == 0x3c);
CHECK(g_index_slot, OFF(transparent_geometry_group, dynamic_index_slot) == 0x44);
CHECK(g_index_buffer, OFF(transparent_geometry_group, index_buffer) == 0x48);
CHECK(g_first, OFF(transparent_geometry_group, first_index) == 0x4c);
CHECK(g_count, OFF(transparent_geometry_group, primitive_count) == 0x50);
CHECK(g_vertex_slot, OFF(transparent_geometry_group, dynamic_vertex_slot) == 0x54);
CHECK(g_vertex_buffer, OFF(transparent_geometry_group, vertex_buffer) == 0x58);
CHECK(g_lightmap, OFF(transparent_geometry_group, lightmap_bitmap) == 0x5c);
CHECK(g_nodes, OFF(transparent_geometry_group, node_matrices) == 0x60);
CHECK(g_node_count, OFF(transparent_geometry_group, node_count) == 0x64);
CHECK(g_parts, OFF(transparent_geometry_group, node_part_indices) == 0x68);
CHECK(g_lighting, OFF(transparent_geometry_group, lighting) == 0x70);
CHECK(g_extra, OFF(transparent_geometry_group, lighting_extra) == 0x74);
CHECK(g_depth, OFF(transparent_geometry_group, depth) == 0x78);
CHECK(g_position, OFF(transparent_geometry_group, position) == 0x7c);
CHECK(g_tint, OFF(transparent_geometry_group, tint) == 0x88);
CHECK(g_sorted, OFF(transparent_geometry_group, sorted_index) == 0x98);
CHECK(g_prev, OFF(transparent_geometry_group, previous_group_index) == 0x9c);
CHECK(g_next, OFF(transparent_geometry_group, next_group_index) == 0x9e);
CHECK(g_a0, OFF(transparent_geometry_group, unknown_a0) == 0xa0);
CHECK(g_fp, OFF(transparent_geometry_group, first_person) == 0xa5);

// model draw context
CHECK(c_nodes, OFF(rasterizer_model_draw_context, node_matrices) == 0x08);
CHECK(c_node_count, OFF(rasterizer_model_draw_context, node_count) == 0x0c);
CHECK(c_lighting, OFF(rasterizer_model_draw_context, lighting) == 0x10);
CHECK(c_extra, OFF(rasterizer_model_draw_context, unknown_84) == 0x84);
CHECK(c_params, OFF(rasterizer_model_draw_context, group_parameters) == 0x8c);
CHECK(c_float90, OFF(rasterizer_model_draw_context, group_parameters.blend_factor) == 0x90);
CHECK(c_sort, OFF(rasterizer_model_draw_context, group_parameters.sort_key) == 0x98);
CHECK(c_position, OFF(rasterizer_model_draw_context, group_parameters.position) == 0x9c);
CHECK(c_shader, OFF(rasterizer_model_draw_context, group_parameters.shader) == 0xa8);
CHECK(c_functions, OFF(rasterizer_model_draw_context, group_parameters.function_values) == 0xb0);
CHECK(c_center, OFF(rasterizer_model_draw_context, center) == 0xb4);
CHECK(c_c4, OFF(rasterizer_model_draw_context, unknown_c4) == 0xc4);

// render lighting
CHECK(rl_count, OFF(render_lighting, distant_light_count) == 0x0c);
CHECK(rl_distant, OFF(render_lighting, distant_lights) == 0x10);
CHECK(rl_points, OFF(render_lighting, point_light_count) == 0x40);

// lens flare instance, against the column addresses of the instance array
#define F(f) (0x006ce818 + OFF(lens_flare_instance, f))
CHECK(f_position, F(position) == 0x006ce81c);
CHECK(f_color_alpha, F(color) + 3 == 0x006ce833);
CHECK(f_object, F(object_index) == 0x006ce834);
CHECK(f_high, F(visibility_high) == 0x006ce836);
CHECK(f_low, F(visibility_low) == 0x006ce838);
CHECK(f_window, F(window_flags) == 0x006ce83a);
CHECK(f_intensity, F(intensity) == 0x006ce83b);
CHECK(f_samples, F(sample_count) == 0x006ce83c);

// lens flare batch, against the column addresses of slot 0
CHECK(b_count, 0x00746fc0 + OFF(lens_flare_batch, vertex_count) == 0x0075efc0);
CHECK(b_key, 0x00746fc0 + OFF(lens_flare_batch, key) == 0x0075efc4);
CHECK(b_used, 0x00746fc0 + OFF(lens_flare_batch, last_used) == 0x0075efd4);
CHECK(b_stride_dwords, sizeof(lens_flare_batch) == 0x6006 * 4);

// font glyph cache
CHECK(fg_oldest, 0x006d8828 + OFF(font_glyph_cache, oldest_slot) == 0x006d882a);
CHECK(fg_next, 0x006d8828 + OFF(font_glyph_cache, next_slot) == 0x006d882c);
CHECK(fg_cursor, 0x006d8828 + OFF(font_glyph_cache, cursor_x) == 0x006d882e);
CHECK(fg_row, 0x006d8828 + OFF(font_glyph_cache, cursor_y) == 0x006d8830);
CHECK(fg_atlas, 0x006d8828 + OFF(font_glyph_cache, atlas) == 0x006d8834);
CHECK(fg_entries, 0x006d8828 + OFF(font_glyph_cache, entries) == 0x006d8838);
CHECK(fg_entry_y, 0x006d8838 + OFF(font_glyph_cache_entry, y) == 0x006d883e);

// dynamic caches, effects, render targets
CHECK(dv_first, 0x006d99d8 + OFF(rasterizer_dynamic_vertex_slot, first_vertex) == 0x006d99dc);
CHECK(dv_lock, 0x006d99d8 + OFF(rasterizer_dynamic_vertex_slot, locked_vertices) == 0x006d99e4);
CHECK(dc_handle, 0x006d98e8 + OFF(rasterizer_dynamic_vertex_cache, buffer_handle) == 0x006d98f0);
CHECK(vbs_type, 0x007bf060 + OFF(rasterizer_vertex_buffer_slot, vertex_type) == 0x007bf064);
CHECK(vbs_managed, 0x007bf060 + OFF(rasterizer_vertex_buffer_slot, managed) == 0x007bf070);
CHECK(vd_usage, 0x006e1a90 + OFF(rasterizer_vertex_declaration, usage) == 0x006e1a98);
CHECK(es_vs, 0x0069d410 + OFF(rasterizer_effect_slot, vertex_shader_index) == 0x0069d414);
CHECK(es_tex, 0x0069d410 + OFF(rasterizer_effect_slot, texture_handles) == 0x0069d418);
CHECK(es_consts, 0x0069d410 + OFF(rasterizer_effect_slot, constant_handles) == 0x0069d428);
CHECK(rt_surface, 0x0069d358 + OFF(rasterizer_render_target, surface) == 0x0069d364);
CHECK(rt_texture, 0x0069d358 + OFF(rasterizer_render_target, texture) == 0x0069d368);
CHECK(rt1_format, 0x0069d358 + sizeof(rasterizer_render_target) + OFF(rasterizer_render_target, format) == 0x0069d374);
CHECK(vb_hw, OFF(rasterizer_vertex_buffer, hardware_buffer) == 0x10);
CHECK(ib_hw, OFF(rasterizer_index_buffer, hardware_buffer) == 0x0c);
CHECK(pl_constants, 0x006e0a10 + sizeof(rasterizer_projected_light_constants) == 0x006e0a60);

// the tag layouts this module leans on
CHECK(t_bitmap_data, sizeof(BitmapData) == 0x30);
CHECK(t_bitmap_flags, OFF(BitmapData, flags) == 0x0e);
CHECK(t_lens_flare_radius, OFF(LensFlare, occlusion_radius) == 0x10);
CHECK(t_lens_flare_dir, OFF(LensFlare, occlusion_offset_direction) == 0x14);
CHECK(t_lens_flare_far, OFF(LensFlare, far_fade_distance) == 0x1c);
CHECK(t_lens_flare_flags, OFF(LensFlare, flags) == 0x30);
CHECK(t_lens_flare_refl, OFF(LensFlare, reflections) == 0xc4);
CHECK(t_light_cos_falloff, OFF(Light, cos_falloff_angle) == 0x1c);
CHECK(t_light_cos_cutoff, OFF(Light, cos_cutoff_angle) == 0x20);
CHECK(t_light_spec, OFF(Light, specular_radius_multiplier) == 0x24);
CHECK(t_light_cube, OFF(Light, primary_cube_map) + 0x0c == 0x70);
CHECK(t_light_cube2, OFF(Light, secondary_cube_map) + 0x0c == 0x88);
CHECK(t_shader_type, OFF(Shader, shader_type) == 0x24);
CHECK(t_bsp_clusters, OFF(ScenarioStructureBSP, clusters) + 4 == 0x138);
CHECK(t_bsp_flares, OFF(ScenarioStructureBSP, lens_flares) + 4 == 0x120);
CHECK(t_bsp_markers, OFF(ScenarioStructureBSP, lens_flare_markers) + 4 == 0x12c);
CHECK(t_cluster, sizeof(ScenarioStructureBSPCluster) == 0x68);
CHECK(t_cluster_first, OFF(ScenarioStructureBSPCluster, first_lens_flare_marker_index) == 0x40);
CHECK(t_cluster_count, OFF(ScenarioStructureBSPCluster, lens_flare_marker_count) == 0x42);
CHECK(t_marker, sizeof(ScenarioStructureBSPLensFlareMarker) == 0x10);
CHECK(t_material_rendered, OFF(ScenarioStructureBSPMaterial, rendered_vertices_type) == 0xb0);
CHECK(t_material_lightmap, OFF(ScenarioStructureBSPMaterial, lightmap_vertices_type) == 0xc4);
CHECK(t_part_triangles, OFF(ModelGeometryPart, triangle_buffer_type) == 0x44);
CHECK(t_part_vertices, OFF(ModelGeometryPart, vertex_type) == 0x54);
CHECK(t_globals_rasterizer, OFF(Globals, rasterizer_data) + 4 == 0x138);
CHECK(t_font_char, OFF(FontCharacter, hardware_character_index) == 0x0c);

// phase 4 review: types folded into rasterizer.h and the layouts the rewritten draws rely on
CHECK(dynamic_screen_vertex, sizeof(rasterizer_dynamic_screen_vertex) == 0x18);
CHECK(screen_vertex, sizeof(rasterizer_screen_vertex) == 0x1c);
CHECK(group_link, sizeof(transparent_geometry_group_link) == 0x0c);
CHECK(d3dx_macro_size, sizeof(d3dx_macro) == 0x08);
CHECK(vertex_element9, sizeof(d3d_vertex_element9) == 0x08);
CHECK(surface_desc_width, OFF(d3d_surface_desc, width) == 0x18);
CHECK(viewport_size, sizeof(d3d_viewport) == 0x18);
CHECK(shadow_quad, 0x006e1720 + 4 * sizeof(rasterizer_dynamic_screen_vertex) == 0x006e1780);
CHECK(object_shadow_blur_quad, 0x006e1b80 + 4 * sizeof(rasterizer_dynamic_screen_vertex) == 0x006e1be0);
CHECK(object_shadow_border, 0x006e1be0 + 8 * sizeof(rasterizer_screen_vertex) == 0x006e1cc0);
CHECK(object_shadow_projection, 0x006e1cd0 + sizeof(real_matrix4x3) == 0x006e1d04);
CHECK(window_viewport_bounds, OFF(rasterizer_window_parameters, camera) + OFF(render_camera, viewport_bounds) == 0x34);
CHECK(t_model_base_map, OFF(ShaderModel, base_map) + 0x0c == 0xb0);
CHECK(t_model_detail_scale, OFF(ShaderModel, detail_map_scale) == 0xd8);
CHECK(t_model_detail_v_scale, OFF(ShaderModel, detail_map_v_scale) == 0xec);
CHECK(t_model_u_animation, OFF(ShaderModel, u_animation_source) == 0xfc);
CHECK(t_chicago_maps, OFF(ShaderTransparentChicago, maps) == 0x54);
CHECK(t_chicago_ext_maps4, OFF(ShaderTransparentChicagoExtended, maps_4_stage) == 0x54);
CHECK(t_chicago_ext_maps2, OFF(ShaderTransparentChicagoExtended, maps_2_stage) == 0x60);
CHECK(t_chicago_map, sizeof(ShaderTransparentChicagoMap) == 0xdc);
CHECK(t_chicago_map_color_function, OFF(ShaderTransparentChicagoMap, color_function) == 0x2c);
CHECK(t_globals_corner_fade, OFF(GlobalsRasterizerData, linear_corner_fade) + 0x0c == 0x4c);
CHECK(t_flare_rotation_function, OFF(LensFlare, rotation_function) == 0x80);
CHECK(t_flare_rotation_scale, OFF(LensFlare, rotation_function_scale) == 0x84);
CHECK(t_bitmap_mipmaps, OFF(BitmapData, mipmap_count) == 0x14);
CHECK(t_bitmap_format, OFF(BitmapData, format) == 0x0c);
CHECK(lighting_distant, OFF(render_lighting, distant_lights) == 0x10);
CHECK(lighting_points, OFF(render_lighting, point_light_indices) == 0x44);
CHECK(frustum_view_to_world, 0x007c1220 + OFF(rasterizer_window_parameters, frustum) + OFF(render_frustum, view_to_world) + 4 == 0x007c12c4);

int main(void) { return 0; }
