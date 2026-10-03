/**
 * @file src/rasterizer/rasterizer_c_api.cpp
 * The rasterizer module's C ABI: one extern "C" function per original symbol, same name, signature and calling
 * convention, forwarding to the halo::rasterizer implementation.
 */

#include "internal/state.hpp"

extern "C" int16_t bitmap_compute_mipmap_count(BitmapData *bitmap)
{
    return halo::rasterizer::bitmap_compute_mipmap_count(bitmap);
}

extern "C" int32_t bitmap_compute_texture_data_size(BitmapData *bitmap)
{
    return halo::rasterizer::bitmap_compute_texture_data_size(bitmap);
}

extern "C" int16_t * chimera__rasterizer_set_texture(uint32_t bitmap_tag_id, int16_t stage, int16_t bitmap_type, int16_t default_index, int16_t frame)
{
    return halo::rasterizer::chimera__rasterizer_set_texture(bitmap_tag_id, stage, bitmap_type, default_index, frame);
}

extern "C" uint8_t chimera__rasterizer_set_texture_direct_d3d9(uint32_t bitmap_tag_id, int16_t stage, int16_t frame)
{
    return halo::rasterizer::chimera__rasterizer_set_texture_direct_d3d9(bitmap_tag_id, stage, frame);
}

extern "C" uint8_t chimera__rasterizer_set_texture_direct_d3dx(uint32_t bitmap_tag_id, int16_t stage, int16_t frame, rasterizer_effect_slot *effect_slot)
{
    return halo::rasterizer::chimera__rasterizer_set_texture_direct_d3dx(bitmap_tag_id, stage, frame, effect_slot);
}

extern "C" uint8_t __cdecl color_channel_real_to_byte(float channel)
{
    return halo::rasterizer::color_channel_real_to_byte(channel);
}

extern "C" uint8_t rasterizer_bind_texture_d3d9(int16_t stage, BitmapData *bitmap)
{
    return halo::rasterizer::rasterizer_bind_texture_d3d9(stage, bitmap);
}

extern "C" uint8_t rasterizer_bind_texture_d3dx(int16_t stage, BitmapData *bitmap, rasterizer_effect_slot *effect_slot)
{
    return halo::rasterizer::rasterizer_bind_texture_d3dx(stage, bitmap, effect_slot);
}

extern "C" int32_t rasterizer_bitmap_compute_mipmap_skip_count(BitmapData *bitmap, int16_t *out_width, int16_t *out_height)
{
    return halo::rasterizer::rasterizer_bitmap_compute_mipmap_skip_count(bitmap, out_width, out_height);
}

extern "C" uint8_t rasterizer_bitmap_create_hardware_texture(BitmapData *bitmap)
{
    return halo::rasterizer::rasterizer_bitmap_create_hardware_texture(bitmap);
}

extern "C" int32_t rasterizer_bitmap_sample_texel(BitmapData *bitmap, float *uv, float mip_bias)
{
    return halo::rasterizer::rasterizer_bitmap_sample_texel(bitmap, uv, mip_bias);
}

extern "C" void rasterizer_bitmap_upload_2d_mipmaps(BitmapData *bitmap)
{
    halo::rasterizer::rasterizer_bitmap_upload_2d_mipmaps(bitmap);
}

extern "C" void rasterizer_bitmap_upload_cubemap_mipmaps(BitmapData *bitmap)
{
    halo::rasterizer::rasterizer_bitmap_upload_cubemap_mipmaps(bitmap);
}

extern "C" void rasterizer_bitmap_upload_cubemap_mipmaps_by_face(BitmapData *bitmap)
{
    halo::rasterizer::rasterizer_bitmap_upload_cubemap_mipmaps_by_face(bitmap);
}

extern "C" void rasterizer_force_bilinear_filtering(void)
{
    halo::rasterizer::rasterizer_force_bilinear_filtering();
}

extern "C" void rasterizer_render_target_bind_effect_texture(int16_t target_index, rasterizer_effect_slot *effect_slot, int16_t handle_index)
{
    halo::rasterizer::rasterizer_render_target_bind_effect_texture(target_index, effect_slot, handle_index);
}

extern "C" void * rasterizer_render_target_bind_texture_stage(int16_t target_index, int16_t stage)
{
    return halo::rasterizer::rasterizer_render_target_bind_texture_stage(target_index, stage);
}

extern "C" int16_t * rasterizer_resolve_and_cache_submap_b(uint32_t bitmap_tag_id, int16_t bitmap_type, int16_t stage, int16_t default_index, int16_t frame, rasterizer_effect_slot *effect_slot)
{
    return halo::rasterizer::rasterizer_resolve_and_cache_submap_b(bitmap_tag_id, bitmap_type, stage, default_index, frame, effect_slot);
}

extern "C" uint8_t rasterizer_resolve_and_cache_submap_c(uint32_t bitmap_tag_id, int16_t bitmap_type, int16_t stage, int16_t default_index, int16_t frame)
{
    return halo::rasterizer::rasterizer_resolve_and_cache_submap_c(bitmap_tag_id, bitmap_type, stage, default_index, frame);
}

extern "C" void rasterizer_unbind_stream_and_textures(void)
{
    halo::rasterizer::rasterizer_unbind_stream_and_textures();
}

extern "C" uint8_t rasterizer_validate_and_rebind_texture(uint32_t bitmap_tag_id, int16_t stage, int16_t frame)
{
    return halo::rasterizer::rasterizer_validate_and_rebind_texture(bitmap_tag_id, stage, frame);
}

extern "C" void bsp_compressed_lightmap_vertex_unpack_normal(ScenarioStructureBSPMaterialCompressedLightmapVertex *vertex, real_vector3d *out)
{
    halo::rasterizer::bsp_compressed_lightmap_vertex_unpack_normal(vertex, out);
}

extern "C" void bsp_compressed_rendered_vertex_unpack_normal(ScenarioStructureBSPMaterialCompressedRenderedVertex *vertex, real_vector3d *out)
{
    halo::rasterizer::bsp_compressed_rendered_vertex_unpack_normal(vertex, out);
}

extern "C" uint32_t vector3d_pack_normal_11_11_10(real_vector3d *direction)
{
    return halo::rasterizer::vector3d_pack_normal_11_11_10(direction);
}

extern "C" real_vector3d * vector3d_unpack_normal_11_11_10(real_vector3d *out, uint32_t packed)
{
    return halo::rasterizer::vector3d_unpack_normal_11_11_10(out, packed);
}

extern "C" void chimera__cinematic_screen_effect(rasterizer_frame_time *time_source)
{
    halo::rasterizer::chimera__cinematic_screen_effect(time_source);
}

extern "C" void chimera__gamma(void)
{
    halo::rasterizer::chimera__gamma();
}

extern "C" void chimera__registry_check_3(void)
{
    halo::rasterizer::chimera__registry_check_3();
}

extern "C" void chimera__registry_check_4(void)
{
    halo::rasterizer::chimera__registry_check_4();
}

extern "C" void rasterizer_fog_screen_overlay_set_states(void)
{
    halo::rasterizer::rasterizer_fog_screen_overlay_set_states();
}

extern "C" void rasterizer_gamma_brightness_to_exponent(rasterizer_gamma_settings *settings)
{
    halo::rasterizer::rasterizer_gamma_brightness_to_exponent(settings);
}

extern "C" void rasterizer_motion_sensor_begin(void)
{
    halo::rasterizer::rasterizer_motion_sensor_begin();
}

extern "C" void rasterizer_motion_sensor_blip_draw(const float *position, const float *color, float brightness, float size)
{
    halo::rasterizer::rasterizer_motion_sensor_blip_draw(position, color, brightness, size);
}

extern "C" void rasterizer_motion_sensor_end(const float *position, float sweep)
{
    halo::rasterizer::rasterizer_motion_sensor_end(position, sweep);
}

extern "C" void rasterizer_screen_effect_compute_uv_transform(uint32_t width, uint32_t height, weapon_screen_effect_parameters *params, int16_t pass, int16_t pass_count, uint8_t shift_down)
{
    halo::rasterizer::rasterizer_screen_effect_compute_uv_transform(width, height, params, pass, pass_count, shift_down);
}

extern "C" uint8_t rasterizer_screen_effect_init_shaders(void)
{
    return halo::rasterizer::rasterizer_screen_effect_init_shaders();
}

extern "C" void rasterizer_screen_effect_render(weapon_screen_effect_parameters *input)
{
    halo::rasterizer::rasterizer_screen_effect_render(input);
}

extern "C" void rasterizer_screen_effect_render_fixed_function(weapon_screen_effect_parameters *input)
{
    halo::rasterizer::rasterizer_screen_effect_render_fixed_function(input);
}

extern "C" int rasterizer_screen_flash_init_shaders(void)
{
    return halo::rasterizer::rasterizer_screen_flash_init_shaders();
}

extern "C" void rasterizer_screen_flash_render(void)
{
    halo::rasterizer::rasterizer_screen_flash_render();
}

extern "C" int16_t rasterizer_sun_glow_blur(int16_t first, int16_t second, int16_t passes)
{
    return halo::rasterizer::rasterizer_sun_glow_blur(first, second, passes);
}

extern "C" void rasterizer_sun_glow_capture(const float *rect, int16_t target_index)
{
    halo::rasterizer::rasterizer_sun_glow_capture(rect, target_index);
}

extern "C" uint8_t rasterizer_sun_glow_project_point(real_point3d *point, float radius, float *out_screen, float *out_scale)
{
    return halo::rasterizer::rasterizer_sun_glow_project_point(point, radius, out_screen, out_scale);
}

extern "C" void rasterizer_sun_glow_render(lens_flare_instance *instance)
{
    halo::rasterizer::rasterizer_sun_glow_render(instance);
}

extern "C" void rasterizer_ui_quad_draw(ui_quad_render_state *state, hud_quad_vertex *vertices)
{
    halo::rasterizer::rasterizer_ui_quad_draw(state, vertices);
}

extern "C" void rasterizer_underwater_tint_jitter_update(BitmapData *lightmap)
{
    halo::rasterizer::rasterizer_underwater_tint_jitter_update(lightmap);
}

extern "C" void rasterizer_underwater_tint_set_states(void)
{
    halo::rasterizer::rasterizer_underwater_tint_set_states();
}

extern "C" void chimera__draw_16_bit_text(Rectangle2D *clip_rect_override, int32_t *dest_rect_override, uint32_t position_or_color1, uint32_t position_or_color2, const int16_t *text)
{
    halo::rasterizer::chimera__draw_16_bit_text(clip_rect_override, dest_rect_override, position_or_color1, position_or_color2, text);
}

extern "C" void chimera__draw_8_bit_text(Rectangle2D *clip_rect_override, int32_t *dest_rect_override, uint32_t position_or_color1, uint32_t position_or_color2, const char *text)
{
    halo::rasterizer::chimera__draw_8_bit_text(clip_rect_override, dest_rect_override, position_or_color1, position_or_color2, text);
}

extern "C" void chimera__widescreen_text_scaling(void)
{
    halo::rasterizer::chimera__widescreen_text_scaling();
}

extern "C" void font_glyph_cache_allocate_and_upload(Font *font, FontCharacter *character)
{
    halo::rasterizer::font_glyph_cache_allocate_and_upload(font, character);
}

extern "C" void __cdecl font_glyph_cache_clear_all(void)
{
    halo::rasterizer::font_glyph_cache_clear_all();
}

extern "C" void rasterizer_draw_text_begin(ui_quad_render_state *state)
{
    halo::rasterizer::rasterizer_draw_text_begin(state);
}

extern "C" void rasterizer_draw_text_end(void)
{
    halo::rasterizer::rasterizer_draw_text_end();
}

extern "C" void rasterizer_editbox_log_dump(void)
{
    halo::rasterizer::rasterizer_editbox_log_dump();
}

extern "C" void text_draw_glyph_callback(void *state, void *font, uint8_t *character, uint32_t color, int16_t x, int16_t y, int16_t source_x, int16_t source_y, int16_t width, int16_t height)
{
    halo::rasterizer::text_draw_glyph_callback(state, font, character, color, x, y, source_x, source_y, width, height);
}

extern "C" int32_t __cdecl text_font_system_initialize(void)
{
    return halo::rasterizer::text_font_system_initialize();
}

extern "C" void __cdecl chimera__rasterizer_dispose_free_memory(void)
{
    halo::rasterizer::chimera__rasterizer_dispose_free_memory();
}

extern "C" void chimera__rasterizer_draw_dynamic_triangles_static_vertices(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_index_slot, int32_t first_primitive)
{
    halo::rasterizer::chimera__rasterizer_draw_dynamic_triangles_static_vertices(primitive_count, vertex_buffer, dynamic_index_slot, first_primitive);
}

extern "C" void chimera__rasterizer_draw_dynamic_triangles_static_vertices2(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_index_slot, int32_t first_primitive, rasterizer_vertex_buffer *second_stream)
{
    halo::rasterizer::chimera__rasterizer_draw_dynamic_triangles_static_vertices2(primitive_count, vertex_buffer, dynamic_index_slot, first_primitive, second_stream);
}

extern "C" void * chimera__rasterizer_memory_alloc(void *source, uint32_t size)
{
    return halo::rasterizer::chimera__rasterizer_memory_alloc(source, size);
}

extern "C" void rasterizer_dynamic_geometry_chain_draw(int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, rasterizer_index_buffer *index_buffer)
{
    halo::rasterizer::rasterizer_dynamic_geometry_chain_draw(primitive_count, vertex_buffer, index_buffer);
}

extern "C" void rasterizer_dynamic_geometry_dispose(void)
{
    halo::rasterizer::rasterizer_dynamic_geometry_dispose();
}

extern "C" void rasterizer_dynamic_geometry_draw_dispatch(rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, rasterizer_vertex_buffer *vertex_buffer, int32_t primitive_count, int32_t first_primitive, int32_t dynamic_vertex_slot)
{
    halo::rasterizer::rasterizer_dynamic_geometry_draw_dispatch(index_buffer, dynamic_index_slot, vertex_buffer, primitive_count, first_primitive, dynamic_vertex_slot);
}

extern "C" void rasterizer_dynamic_index_cache_draw(int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, int32_t dynamic_vertex_slot)
{
    halo::rasterizer::rasterizer_dynamic_index_cache_draw(dynamic_index_slot, first_primitive, primitive_count, dynamic_vertex_slot);
}

extern "C" int32_t rasterizer_dynamic_index_cache_reserve(int32_t count)
{
    return halo::rasterizer::rasterizer_dynamic_index_cache_reserve(count);
}

extern "C" void rasterizer_dynamic_light_technique_ps2_set_states(void)
{
    halo::rasterizer::rasterizer_dynamic_light_technique_ps2_set_states();
}

extern "C" void * rasterizer_dynamic_vertex_cache_lock(int32_t slot_index)
{
    return halo::rasterizer::rasterizer_dynamic_vertex_cache_lock(slot_index);
}

extern "C" int32_t rasterizer_dynamic_vertex_cache_reserve(int16_t vertex_type, int32_t count)
{
    return halo::rasterizer::rasterizer_dynamic_vertex_cache_reserve(vertex_type, count);
}

extern "C" void rasterizer_dynamic_vertex_draw(int32_t first_primitive, int32_t primitive_count, int32_t dynamic_vertex_slot, int16_t primitive_kind)
{
    halo::rasterizer::rasterizer_dynamic_vertex_draw(first_primitive, primitive_count, dynamic_vertex_slot, primitive_kind);
}

extern "C" void rasterizer_dynamic_vertex_draw_indexed(rasterizer_index_buffer *index_buffer, int32_t primitive_count, int32_t dynamic_vertex_slot)
{
    halo::rasterizer::rasterizer_dynamic_vertex_draw_indexed(index_buffer, primitive_count, dynamic_vertex_slot);
}

extern "C" uint32_t rasterizer_dynamic_vertex_process_and_get_handle(rasterizer_vertex_buffer *vertex_buffer)
{
    return halo::rasterizer::rasterizer_dynamic_vertex_process_and_get_handle(vertex_buffer);
}

extern "C" void rasterizer_geometry_draw_fixed_function(uint32_t flags, int32_t dynamic_vertex_slot, rasterizer_vertex_buffer *vertex_buffer, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count)
{
    halo::rasterizer::rasterizer_geometry_draw_fixed_function(flags, dynamic_vertex_slot, vertex_buffer, index_buffer, dynamic_index_slot, primitive_count);
}

extern "C" void rasterizer_geometry_part_draw(transparent_geometry_group *group)
{
    halo::rasterizer::rasterizer_geometry_part_draw(group);
}

extern "C" void chimera__rasterizer_set_framebuffer_blend_function(int16_t mode)
{
    halo::rasterizer::chimera__rasterizer_set_framebuffer_blend_function(mode);
}

extern "C" void chimera__rasterizer_set_frustum_z_func(uint32_t z_near, uint32_t z_far)
{
    halo::rasterizer::chimera__rasterizer_set_frustum_z_func(z_near, z_far);
}

extern "C" void display_mode_get_current(rasterizer_display_mode *out)
{
    halo::rasterizer::display_mode_get_current(out);
}

extern "C" void __cdecl rasterizer_begin_frame(rasterizer_window_parameters *source)
{
    halo::rasterizer::rasterizer_begin_frame(source);
}

extern "C" void rasterizer_build_present_parameters(d3d_present_parameters *dest, rasterizer_display_mode *source)
{
    halo::rasterizer::rasterizer_build_present_parameters(dest, source);
}

extern "C" void rasterizer_capture_and_present(const int16_t *tile, BitmapData *bitmap)
{
    halo::rasterizer::rasterizer_capture_and_present(tile, bitmap);
}

extern "C" uint32_t rasterizer_create_game_window(int32_t height, int32_t width)
{
    return halo::rasterizer::rasterizer_create_game_window(height, width);
}

extern "C" uint8_t rasterizer_device_reset(d3d_present_parameters *present_parameters)
{
    return halo::rasterizer::rasterizer_device_reset(present_parameters);
}

extern "C" uint8_t rasterizer_display_mode_differs(rasterizer_display_mode *requested)
{
    return halo::rasterizer::rasterizer_display_mode_differs(requested);
}

extern "C" void rasterizer_end_frame(void)
{
    halo::rasterizer::rasterizer_end_frame();
}

extern "C" int32_t rasterizer_get_refresh_rate(int32_t requested_rate)
{
    return halo::rasterizer::rasterizer_get_refresh_rate(requested_rate);
}

extern "C" uint8_t rasterizer_initialize_direct3d(void)
{
    return halo::rasterizer::rasterizer_initialize_direct3d();
}

extern "C" uint8_t rasterizer_parse_vidmode_commandline(int32_t *width_out, int32_t *height_out, long *refresh_out)
{
    return halo::rasterizer::rasterizer_parse_vidmode_commandline(width_out, height_out, refresh_out);
}

extern "C" uint8_t __cdecl rasterizer_reset_device_if_needed(void)
{
    return halo::rasterizer::rasterizer_reset_device_if_needed();
}

extern "C" void rasterizer_resize_game_window(int32_t height, int32_t width)
{
    halo::rasterizer::rasterizer_resize_game_window(height, width);
}

extern "C" int32_t rasterizer_round_up_resolution_height(int32_t height)
{
    return halo::rasterizer::rasterizer_round_up_resolution_height(height);
}

extern "C" void __cdecl rasterizer_select_hardware_codepaths(void)
{
    halo::rasterizer::rasterizer_select_hardware_codepaths();
}

extern "C" void rasterizer_service_deferred_windowed_ops(void)
{
    halo::rasterizer::rasterizer_service_deferred_windowed_ops();
}

extern "C" void __cdecl rasterizer_set_default_render_states(void)
{
    halo::rasterizer::rasterizer_set_default_render_states();
}

extern "C" void __cdecl rasterizer_shutdown(void)
{
    halo::rasterizer::rasterizer_shutdown();
}

extern "C" void chimera__rasterizer_set_model_skinning(uint8_t upload, rasterizer_node_matrices *nodes)
{
    halo::rasterizer::chimera__rasterizer_set_model_skinning(upload, nodes);
}

extern "C" void chimera__rasterizer_set_up_node_parts(int32_t node_part_count, uint8_t *node_part_indices)
{
    halo::rasterizer::chimera__rasterizer_set_up_node_parts(node_part_count, node_part_indices);
}

extern "C" void rasterizer_model_draw_prepare_states(rasterizer_model_draw_context *context, uint8_t mode)
{
    halo::rasterizer::rasterizer_model_draw_prepare_states(context, mode);
}

extern "C" void rasterizer_model_draw_restore_states(void)
{
    halo::rasterizer::rasterizer_model_draw_restore_states();
}

extern "C" uint8_t rasterizer_object_shadow_begin(const real_matrix4x3 *projection, const ColorRGB *color, float radius, float *out_radius)
{
    return halo::rasterizer::rasterizer_object_shadow_begin(projection, color, radius, out_radius);
}

extern "C" void rasterizer_object_shadow_blur(void)
{
    halo::rasterizer::rasterizer_object_shadow_blur();
}

extern "C" void rasterizer_object_shadow_model_draw(const ShaderModel *shader, int16_t frame, rasterizer_index_buffer *index_buffer, rasterizer_vertex_buffer *vertex_buffer)
{
    halo::rasterizer::rasterizer_object_shadow_model_draw(shader, frame, index_buffer, vertex_buffer);
}

extern "C" void rasterizer_object_shadow_structure_draw(rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count)
{
    halo::rasterizer::rasterizer_object_shadow_structure_draw(vertex_buffer, dynamic_index_slot, first_primitive, primitive_count);
}

extern "C" void rasterizer_shader_model_draw_fixed_function(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot)
{
    halo::rasterizer::rasterizer_shader_model_draw_fixed_function(shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer, dynamic_vertex_slot);
}

extern "C" void rasterizer_shader_model_draw_limited(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot)
{
    halo::rasterizer::rasterizer_shader_model_draw_limited(shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer, dynamic_vertex_slot);
}

extern "C" void rasterizer_shader_model_draw_pixel_shader(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot)
{
    halo::rasterizer::rasterizer_shader_model_draw_pixel_shader(shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer, dynamic_vertex_slot);
}

extern "C" rasterizer_effect_slot * rasterizer_shader_model_select_technique(const ShaderModel *shader)
{
    return halo::rasterizer::rasterizer_shader_model_select_technique(shader);
}

extern "C" void chimera__transparent_decal_zbias(void)
{
    halo::rasterizer::chimera__transparent_decal_zbias();
}

extern "C" void decal_and_font_system_reset(void)
{
    halo::rasterizer::decal_and_font_system_reset();
}

extern "C" void decal_geometry_cache_restore_procs(void)
{
    halo::rasterizer::decal_geometry_cache_restore_procs();
}

extern "C" uint8_t decal_vertex_cache_in_use(datum_index handle)
{
    return halo::rasterizer::decal_vertex_cache_in_use(handle);
}

extern "C" void decal_vertex_cache_release(datum_index handle)
{
    halo::rasterizer::decal_vertex_cache_release(handle);
}

extern "C" void rasterizer_apply_decal_zbias(void)
{
    halo::rasterizer::rasterizer_apply_decal_zbias();
}

extern "C" void rasterizer_clear_decal_zbias(void)
{
    halo::rasterizer::rasterizer_clear_decal_zbias();
}

extern "C" uint8_t rasterizer_decal_index_buffer_initialize(void)
{
    return halo::rasterizer::rasterizer_decal_index_buffer_initialize();
}

extern "C" void rasterizer_decal_pass_begin(int16_t stage)
{
    halo::rasterizer::rasterizer_decal_pass_begin(stage);
}

extern "C" void * rasterizer_decal_vertex_cache_lock(uint32_t decal_index, int32_t byte_count)
{
    return halo::rasterizer::rasterizer_decal_vertex_cache_lock(decal_index, byte_count);
}

extern "C" int __cdecl rasterizer_decal_zbias_active(void)
{
    return halo::rasterizer::rasterizer_decal_zbias_active();
}

extern "C" void rasterizer_decals_draw_cluster(int16_t cluster_index)
{
    halo::rasterizer::rasterizer_decals_draw_cluster(cluster_index);
}

extern "C" void __cdecl rasterizer_decals_initialize(void)
{
    halo::rasterizer::rasterizer_decals_initialize();
}

extern "C" void rasterizer_end_decal_pass(void)
{
    halo::rasterizer::rasterizer_end_decal_pass();
}

extern "C" void rasterizer_shader_decal_pass_set_states(void)
{
    halo::rasterizer::rasterizer_shader_decal_pass_set_states();
}

extern "C" void lens_flare_add_instance(lens_flare_instance *candidate)
{
    halo::rasterizer::lens_flare_add_instance(candidate);
}

extern "C" float lens_flare_compute_rotation(lens_flare_instance *flare, int16_t mode)
{
    return halo::rasterizer::lens_flare_compute_rotation(flare, mode);
}

extern "C" uint8_t * lens_flare_get_visibility_byte(lens_flare_instance *flare)
{
    return halo::rasterizer::lens_flare_get_visibility_byte(flare);
}

extern "C" void lens_flare_render_all(void)
{
    halo::rasterizer::lens_flare_render_all();
}

extern "C" void lens_flare_update_samples(void)
{
    halo::rasterizer::lens_flare_update_samples();
}

extern "C" void lens_flare_update_visibility(void)
{
    halo::rasterizer::lens_flare_update_visibility();
}

extern "C" uint8_t rasterizer_lens_flare_batch_apply_material(lens_flare_batch_key *key)
{
    return halo::rasterizer::rasterizer_lens_flare_batch_apply_material(key);
}

extern "C" void rasterizer_lens_flare_batch_draw_slot(int32_t batch_index)
{
    halo::rasterizer::rasterizer_lens_flare_batch_draw_slot(batch_index);
}

extern "C" int32_t rasterizer_lens_flare_batch_find_slot(void)
{
    return halo::rasterizer::rasterizer_lens_flare_batch_find_slot();
}

extern "C" void rasterizer_lens_flare_batch_flush_all(void)
{
    halo::rasterizer::rasterizer_lens_flare_batch_flush_all();
}

extern "C" void rasterizer_lens_flare_batching_select_mode(int16_t mode, uint32_t flags)
{
    halo::rasterizer::rasterizer_lens_flare_batching_select_mode(mode, flags);
}

extern "C" uint8_t rasterizer_lens_flare_occlusion_queries_create(void)
{
    return halo::rasterizer::rasterizer_lens_flare_occlusion_queries_create();
}

extern "C" int32_t rasterizer_lens_flare_occlusion_query_get_result(int32_t slot_index)
{
    return halo::rasterizer::rasterizer_lens_flare_occlusion_query_get_result(slot_index);
}

extern "C" void rasterizer_lens_flare_occlusion_sample_add(void *procedure, const real_point3d *position, uint32_t id_1, uint32_t id_2)
{
    halo::rasterizer::rasterizer_lens_flare_occlusion_sample_add(procedure, position, id_1, id_2);
}

extern "C" int32_t rasterizer_lens_flare_occlusion_test_issue(int32_t slot_index, const real_point3d *position, float radius)
{
    return halo::rasterizer::rasterizer_lens_flare_occlusion_test_issue(slot_index, position, radius);
}

extern "C" uint8_t rasterizer_lens_flare_project_to_screen(const real_point3d *position, float radius, float *out_screen, float *out_inverse_w, float *out_billboard_size)
{
    return halo::rasterizer::rasterizer_lens_flare_project_to_screen(position, radius, out_screen, out_inverse_w, out_billboard_size);
}

extern "C" void rasterizer_lens_flare_quad_add(const float *scale, uint32_t diffuse, const real_point3d *position, float radius, float rotation_degrees)
{
    halo::rasterizer::rasterizer_lens_flare_quad_add(scale, diffuse, position, radius, rotation_degrees);
}

extern "C" void structure_cluster_add_lens_flares(int16_t cluster_index)
{
    halo::rasterizer::structure_cluster_add_lens_flares(cluster_index);
}

extern "C" uint8_t rasterizer_detail_object_vertex_buffer_create(void)
{
    return halo::rasterizer::rasterizer_detail_object_vertex_buffer_create();
}

extern "C" void rasterizer_detail_objects_begin(void)
{
    halo::rasterizer::rasterizer_detail_objects_begin();
}

extern "C" void rasterizer_detail_objects_draw(const rasterizer_detail_object_batches *list)
{
    halo::rasterizer::rasterizer_detail_objects_draw(list);
}

extern "C" void rasterizer_detail_objects_expand_quad_vertices(int32_t quad_count, uint32_t *vertices, const uint8_t *instances, const DetailObjectCollection *collection, const rasterizer_detail_object_draw *draw)
{
    halo::rasterizer::rasterizer_detail_objects_expand_quad_vertices(quad_count, vertices, instances, collection, draw);
}

extern "C" void rasterizer_detail_objects_vertex_buffer_fill(rasterizer_detail_object_batches *list)
{
    halo::rasterizer::rasterizer_detail_objects_vertex_buffer_fill(list);
}

extern "C" void * rasterizer_dx9_create_vertex_buffer(int32_t vertex_type, uint32_t length, uint32_t fvf, uint8_t not_dynamic)
{
    return halo::rasterizer::rasterizer_dx9_create_vertex_buffer(vertex_type, length, fvf, not_dynamic);
}

extern "C" uint8_t rasterizer_dx9_effects_initialize(void)
{
    return halo::rasterizer::rasterizer_dx9_effects_initialize();
}

extern "C" int32_t rasterizer_dx9_pixel_shader_effect_load(int32_t effect_index, const void *data, uint32_t size)
{
    return halo::rasterizer::rasterizer_dx9_pixel_shader_effect_load(effect_index, data, size);
}

extern "C" uint8_t rasterizer_dx9_pixel_shaders_load_all(void)
{
    return halo::rasterizer::rasterizer_dx9_pixel_shaders_load_all();
}

extern "C" void rasterizer_dx9_pixel_shaders_release(void)
{
    halo::rasterizer::rasterizer_dx9_pixel_shaders_release();
}

extern "C" int32_t rasterizer_dx9_shaders_init_effect(int32_t effect_index)
{
    return halo::rasterizer::rasterizer_dx9_shaders_init_effect(effect_index);
}

extern "C" void rasterizer_dx9_shaders_release_all(void)
{
    halo::rasterizer::rasterizer_dx9_shaders_release_all();
}

extern "C" void rasterizer_dx9_vertex_declarations_release(void)
{
    halo::rasterizer::rasterizer_dx9_vertex_declarations_release();
}

extern "C" uint8_t rasterizer_dx9_vertex_shaders_initialize(void)
{
    return halo::rasterizer::rasterizer_dx9_vertex_shaders_initialize();
}

extern "C" uint32_t rasterizer_dx9_vertex_shaders_load_all(void)
{
    return halo::rasterizer::rasterizer_dx9_vertex_shaders_load_all();
}

extern "C" uint8_t rasterizer_dx9_vertex_shaders_reload(void)
{
    return halo::rasterizer::rasterizer_dx9_vertex_shaders_reload();
}

extern "C" void * rasterizer_get_capture_surface(uint8_t *object, void *fallback)
{
    return halo::rasterizer::rasterizer_get_capture_surface(object, fallback);
}

extern "C" uint8_t rasterizer_index_buffer_create(int32_t count, int16_t type, rasterizer_index_buffer *out, const void *source)
{
    return halo::rasterizer::rasterizer_index_buffer_create(count, type, out, source);
}

extern "C" void __cdecl rasterizer_ksml_ui_shutdown(void)
{
    halo::rasterizer::rasterizer_ksml_ui_shutdown();
}

extern "C" uint32_t rasterizer_load_file_and_verify(void **out_buffer, uint32_t *out_size, const char *path)
{
    return halo::rasterizer::rasterizer_load_file_and_verify(out_buffer, out_size, path);
}

extern "C" uint8_t rasterizer_misc_vertex_buffer_create(void)
{
    return halo::rasterizer::rasterizer_misc_vertex_buffer_create();
}

extern "C" void rasterizer_render_target_capture_frame(void)
{
    halo::rasterizer::rasterizer_render_target_capture_frame();
}

extern "C" void rasterizer_render_target_dispose(void)
{
    halo::rasterizer::rasterizer_render_target_dispose();
}

extern "C" uint8_t rasterizer_render_target_initialize(void)
{
    return halo::rasterizer::rasterizer_render_target_initialize();
}

extern "C" void rasterizer_render_target_set_active(int16_t target_index, uint32_t clear_color, uint8_t clear)
{
    halo::rasterizer::rasterizer_render_target_set_active(target_index, clear_color, clear);
}

extern "C" uint8_t rasterizer_resource_file_verify_signature(uint8_t *buffer, uint32_t size)
{
    return halo::rasterizer::rasterizer_resource_file_verify_signature(buffer, size);
}

extern "C" uint8_t rasterizer_vertex_buffer_create(rasterizer_vertex_buffer *record, int16_t vertex_type, int32_t count, uint32_t *source_data, int32_t second_stream, uint32_t size)
{
    return halo::rasterizer::rasterizer_vertex_buffer_create(record, vertex_type, count, source_data, second_stream, size);
}

extern "C" int32_t rasterizer_vertex_buffer_slot_allocate(int32_t vertex_type, uint32_t fvf, uint32_t length)
{
    return halo::rasterizer::rasterizer_vertex_buffer_slot_allocate(vertex_type, fvf, length);
}

extern "C" void rasterizer_vertex_buffer_slot_recreate_lost(void)
{
    halo::rasterizer::rasterizer_vertex_buffer_slot_recreate_lost();
}

extern "C" uint8_t rasterizer_dx9_shaders_initialize(void)
{
    return halo::rasterizer::rasterizer_dx9_shaders_initialize();
}

extern "C" uint8_t rasterizer_dx9_vertex_declarations_create(void)
{
    return halo::rasterizer::rasterizer_dx9_vertex_declarations_create();
}

extern "C" void rasterizer_render_loading_screen(int32_t mode)
{
    halo::rasterizer::rasterizer_render_loading_screen(mode);
}

extern "C" void rasterizer_glass_diffuse_draw(transparent_geometry_group *group)
{
    halo::rasterizer::rasterizer_glass_diffuse_draw(group);
}

extern "C" void rasterizer_glass_diffuse_draw_fixed_function(transparent_geometry_group *group)
{
    halo::rasterizer::rasterizer_glass_diffuse_draw_fixed_function(group);
}

extern "C" void rasterizer_glass_draw_procedures_select(void)
{
    halo::rasterizer::rasterizer_glass_draw_procedures_select();
}

extern "C" void rasterizer_glass_reflection_draw(transparent_geometry_group *group, int16_t reflection_kind)
{
    halo::rasterizer::rasterizer_glass_reflection_draw(group, reflection_kind);
}

extern "C" void rasterizer_glass_reflection_draw_fixed_function(transparent_geometry_group *group, uint32_t reflection_kind)
{
    halo::rasterizer::rasterizer_glass_reflection_draw_fixed_function(group, reflection_kind);
}

extern "C" void rasterizer_glass_tint_draw(transparent_geometry_group *group)
{
    halo::rasterizer::rasterizer_glass_tint_draw(group);
}

extern "C" void rasterizer_glass_tint_draw_fixed_function(transparent_geometry_group *group)
{
    halo::rasterizer::rasterizer_glass_tint_draw_fixed_function(group);
}

extern "C" void rasterizer_shader_transparent_chicago_draw(transparent_geometry_group *group, uint8_t attached)
{
    halo::rasterizer::rasterizer_shader_transparent_chicago_draw(group, attached);
}

extern "C" void rasterizer_shader_transparent_chicago_extended_draw(transparent_geometry_group *group, uint8_t attached)
{
    halo::rasterizer::rasterizer_shader_transparent_chicago_extended_draw(group, attached);
}

extern "C" uint8_t rasterizer_shader_transparent_chicago_extended_set_texture_stages(const ShaderTransparentChicagoExtended *shader)
{
    return halo::rasterizer::rasterizer_shader_transparent_chicago_extended_set_texture_stages(shader);
}

extern "C" uint8_t rasterizer_shader_transparent_chicago_set_texture_stages(const ShaderTransparentChicago *shader)
{
    return halo::rasterizer::rasterizer_shader_transparent_chicago_set_texture_stages(shader);
}

extern "C" void rasterizer_shader_transparent_plasma_draw(transparent_geometry_group *group)
{
    halo::rasterizer::rasterizer_shader_transparent_plasma_draw(group);
}

extern "C" void rasterizer_water_draw_fixed_function(transparent_geometry_group *group)
{
    halo::rasterizer::rasterizer_water_draw_fixed_function(group);
}

extern "C" void rasterizer_water_draw_pixel_shader(transparent_geometry_group *group)
{
    halo::rasterizer::rasterizer_water_draw_pixel_shader(group);
}

extern "C" void rasterizer_water_fade_compute_and_set_states(void)
{
    halo::rasterizer::rasterizer_water_fade_compute_and_set_states();
}

extern "C" void rasterizer_water_ripple_draw(rasterizer_vertex_buffer *vertex_buffer, const Shader *shader, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count)
{
    halo::rasterizer::rasterizer_water_ripple_draw(vertex_buffer, shader, dynamic_index_slot, first_primitive, primitive_count);
}

extern "C" void rasterizer_water_update_ripple_texture(void *water_shader)
{
    halo::rasterizer::rasterizer_water_update_ripple_texture(water_shader);
}

extern "C" void rasterizer_light_cone_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    halo::rasterizer::rasterizer_light_cone_draw(shader, frame, dynamic_index_slot, first_primitive, primitive_count, vertex_buffer);
}

extern "C" void rasterizer_light_cone_set_orientation_constants(int32_t light_index)
{
    halo::rasterizer::rasterizer_light_cone_set_orientation_constants(light_index);
}

extern "C" void rasterizer_light_cone_set_texture_stage_states(void)
{
    halo::rasterizer::rasterizer_light_cone_set_texture_stage_states();
}

extern "C" void rasterizer_light_disable_all(void)
{
    halo::rasterizer::rasterizer_light_disable_all();
}

extern "C" void rasterizer_light_set(rasterizer_light *light)
{
    halo::rasterizer::rasterizer_light_set(light);
}

extern "C" void rasterizer_light_set_point_constants(int32_t light_index, int16_t slot, rasterizer_point_light_constants *dest_base)
{
    halo::rasterizer::rasterizer_light_set_point_constants(light_index, slot, dest_base);
}

extern "C" void rasterizer_prepare_lighting_constants(render_lighting *lighting)
{
    halo::rasterizer::rasterizer_prepare_lighting_constants(lighting);
}

extern "C" void rasterizer_projected_light_constants_build(int32_t light_index)
{
    halo::rasterizer::rasterizer_projected_light_constants_build(light_index);
}

extern "C" void rasterizer_projected_light_constants_build_cube_map(int32_t light_index)
{
    halo::rasterizer::rasterizer_projected_light_constants_build_cube_map(light_index);
}

extern "C" void rasterizer_set_fog_constants(const render_fog *fog)
{
    halo::rasterizer::rasterizer_set_fog_constants(fog);
}

extern "C" void rasterizer_set_shader_stage_config(int16_t mode)
{
    halo::rasterizer::rasterizer_set_shader_stage_config(mode);
}

extern "C" void * rasterizer_shader_technique_for_name(void *effect, const char *name)
{
    return halo::rasterizer::rasterizer_shader_technique_for_name(effect, name);
}

extern "C" uint8_t rasterizer_shader_environment_build_technique_table(void)
{
    return halo::rasterizer::rasterizer_shader_environment_build_technique_table();
}

extern "C" void rasterizer_shader_environment_draw_dispatch(int32_t dynamic_vertex_slot, uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    halo::rasterizer::rasterizer_shader_environment_draw_dispatch(dynamic_vertex_slot, shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer);
}

extern "C" void rasterizer_shader_environment_draw_fixed_function(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot)
{
    halo::rasterizer::rasterizer_shader_environment_draw_fixed_function(shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer, dynamic_vertex_slot);
}

extern "C" void rasterizer_shader_environment_draw_pixel_shader(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot)
{
    halo::rasterizer::rasterizer_shader_environment_draw_pixel_shader(shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer, dynamic_vertex_slot);
}

extern "C" void rasterizer_shader_environment_draw_single_stream(uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot)
{
    halo::rasterizer::rasterizer_shader_environment_draw_single_stream(shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer, dynamic_vertex_slot);
}

extern "C" void rasterizer_shader_environment_dynamic_mirror_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    halo::rasterizer::rasterizer_shader_environment_dynamic_mirror_draw(shader, frame, dynamic_index_slot, first_primitive, primitive_count, vertex_buffer);
}

extern "C" void rasterizer_shader_environment_lightmap_draw(uint8_t *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, void *vertex_buffer)
{
    halo::rasterizer::rasterizer_shader_environment_lightmap_draw(shader, frame, dynamic_index_slot, first_primitive, primitive_count, vertex_buffer);
}

extern "C" void rasterizer_shader_environment_lightmap_draw_single_stream(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    halo::rasterizer::rasterizer_shader_environment_lightmap_draw_single_stream(shader, frame, dynamic_index_slot, first_primitive, primitive_count, vertex_buffer);
}

extern "C" void rasterizer_shader_environment_lightmap_draw_two_stream(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    halo::rasterizer::rasterizer_shader_environment_lightmap_draw_two_stream(shader, frame, dynamic_index_slot, first_primitive, primitive_count, vertex_buffer);
}

extern "C" void rasterizer_shader_environment_lightmap_specular_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    halo::rasterizer::rasterizer_shader_environment_lightmap_specular_draw(shader, frame, dynamic_index_slot, first_primitive, primitive_count, vertex_buffer);
}

extern "C" void rasterizer_shader_environment_projected_light_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    halo::rasterizer::rasterizer_shader_environment_projected_light_draw(shader, frame, dynamic_index_slot, first_primitive, primitive_count, vertex_buffer);
}

extern "C" void rasterizer_shader_environment_reflection_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    halo::rasterizer::rasterizer_shader_environment_reflection_draw(shader, frame, dynamic_index_slot, first_primitive, primitive_count, vertex_buffer);
}

extern "C" void rasterizer_shader_environment_select_draw_functions(void)
{
    halo::rasterizer::rasterizer_shader_environment_select_draw_functions();
}

extern "C" void rasterizer_shader_environment_self_illumination_draw(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    halo::rasterizer::rasterizer_shader_environment_self_illumination_draw(shader, frame, dynamic_index_slot, first_primitive, primitive_count, vertex_buffer);
}

extern "C" void rasterizer_shader_environment_self_illumination_draw_single_stream(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    halo::rasterizer::rasterizer_shader_environment_self_illumination_draw_single_stream(shader, frame, dynamic_index_slot, first_primitive, primitive_count, vertex_buffer);
}

extern "C" void rasterizer_shader_environment_self_illumination_draw_two_stream(const ShaderEnvironment *shader, int16_t frame, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer)
{
    halo::rasterizer::rasterizer_shader_environment_self_illumination_draw_two_stream(shader, frame, dynamic_index_slot, first_primitive, primitive_count, vertex_buffer);
}

extern "C" void rasterizer_shader_environment_set_lightmap(BitmapData *lightmap)
{
    halo::rasterizer::rasterizer_shader_environment_set_lightmap(lightmap);
}

extern "C" void rasterizer_shader_environment_technique_draw(rasterizer_vertex_buffer *vertex_buffer, const ShaderEnvironment *shader, int32_t dynamic_index_slot, int32_t first_primitive, int32_t primitive_count)
{
    halo::rasterizer::rasterizer_shader_environment_technique_draw(vertex_buffer, shader, dynamic_index_slot, first_primitive, primitive_count);
}

extern "C" void rasterizer_shader_environment_technique_multipurpose_set_states(void)
{
    halo::rasterizer::rasterizer_shader_environment_technique_multipurpose_set_states();
}

extern "C" void rasterizer_shader_environment_technique_ps2_set_states(void)
{
    halo::rasterizer::rasterizer_shader_environment_technique_ps2_set_states();
}

extern "C" void rasterizer_shader_environment_technique_self_illumination_set_states(void)
{
    halo::rasterizer::rasterizer_shader_environment_technique_self_illumination_set_states();
}

extern "C" int __cdecl rasterizer_transparent_decals_enabled(void)
{
    return halo::rasterizer::rasterizer_transparent_decals_enabled();
}

extern "C" transparent_geometry_group * rasterizer_transparent_geometry_group_build(transparent_geometry_group_link *link, uint8_t *shader, int16_t frame, rasterizer_index_buffer *index_buffer, int32_t dynamic_index_slot, int32_t primitive_count, rasterizer_vertex_buffer *vertex_buffer, int32_t dynamic_vertex_slot, const real_point3d *position)
{
    return halo::rasterizer::rasterizer_transparent_geometry_group_build(link, shader, frame, index_buffer, dynamic_index_slot, primitive_count, vertex_buffer, dynamic_vertex_slot, position);
}

extern "C" void rasterizer_transparent_geometry_group_draw(transparent_geometry_group *group, uint8_t attached)
{
    halo::rasterizer::rasterizer_transparent_geometry_group_draw(group, attached);
}

extern "C" void rasterizer_transparent_geometry_group_draw_active_camouflage(transparent_geometry_group *group)
{
    halo::rasterizer::rasterizer_transparent_geometry_group_draw_active_camouflage(group);
}

extern "C" void rasterizer_transparent_geometry_group_draw_vertices(transparent_geometry_group *group, uint8_t flag)
{
    halo::rasterizer::rasterizer_transparent_geometry_group_draw_vertices(group, flag);
}

extern "C" void rasterizer_transparent_geometry_group_new(Shader *shader, int16_t shader_permutation, uint32_t lightmap_bitmap, uint32_t dynamic_index_slot, uint32_t first_index, uint32_t primitive_count, uint32_t vertex_buffer, ColorARGB *tint, uint32_t lighting, uint32_t flags, real_point3d *world_position)
{
    halo::rasterizer::rasterizer_transparent_geometry_group_new(shader, shader_permutation, lightmap_bitmap, dynamic_index_slot, first_index, primitive_count, vertex_buffer, tint, lighting, flags, world_position);
}

extern "C" void rasterizer_transparent_object_append(uint32_t lightmap_bitmap, int32_t dynamic_index_slot, int32_t dynamic_vertex_slot, int32_t primitive_count, uint32_t flags, real_point3d *world_position, Shader *shader)
{
    halo::rasterizer::rasterizer_transparent_object_append(lightmap_bitmap, dynamic_index_slot, dynamic_vertex_slot, primitive_count, flags, world_position, shader);
}

extern "C" transparent_geometry_group * __cdecl transparent_geometry_group_allocate(void)
{
    return halo::rasterizer::transparent_geometry_group_allocate();
}

extern "C" transparent_geometry_group * __cdecl transparent_geometry_group_allocate_secondary(void)
{
    return halo::rasterizer::transparent_geometry_group_allocate_secondary();
}

extern "C" int __cdecl transparent_geometry_group_compare(int16_t *a, int16_t *b)
{
    return halo::rasterizer::transparent_geometry_group_compare(a, b);
}

extern "C" void transparent_geometry_group_draw_all(uint8_t resort)
{
    halo::rasterizer::transparent_geometry_group_draw_all(resort);
}

extern "C" transparent_geometry_group * transparent_geometry_group_get_next_sorted(transparent_geometry_group *group)
{
    return halo::rasterizer::transparent_geometry_group_get_next_sorted(group);
}

extern "C" uint32_t transparent_geometry_group_get_vertex_type_reference(transparent_geometry_group *group)
{
    return halo::rasterizer::transparent_geometry_group_get_vertex_type_reference(group);
}

extern "C" int32_t transparent_geometry_group_index_from_pointer(transparent_geometry_group *group)
{
    return halo::rasterizer::transparent_geometry_group_index_from_pointer(group);
}

extern "C" void transparent_geometry_group_set_drawn_bit(transparent_geometry_group *group, uint8_t clear)
{
    halo::rasterizer::transparent_geometry_group_set_drawn_bit(group, clear);
}

extern "C" void __cdecl transparent_geometry_group_sort(void)
{
    halo::rasterizer::transparent_geometry_group_sort();
}

extern "C" uint8_t transparent_geometry_group_test_drawn_bit(transparent_geometry_group *group)
{
    return halo::rasterizer::transparent_geometry_group_test_drawn_bit(group);
}

extern "C" int32_t __cdecl transparent_geometry_pool_initialize(void)
{
    return halo::rasterizer::transparent_geometry_pool_initialize();
}
