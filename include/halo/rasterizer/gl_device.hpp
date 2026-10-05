#pragma once

#include "halo/rasterizer/render_device.hpp"

namespace halo::rasterizer {

/** True when the OpenGL backend is in use: requested with HALO_RENDERER=gl, or the only one built (HALO_D3D9 0). */
bool gl_renderer_requested();

/** Direct3DCreate9 of builds without Direct3D 9: the factory object answering adapter and capability queries. */
void *__stdcall gl_direct3d_create(uint32_t sdk_version);

/**
 * OpenGL implementation of the RenderDevice interface (work in progress, see docs/OPENGL_PORT.md).
 *
 * Milestone 1: a WGL context on the game window that clears and presents, plus CPU-side stand-ins for every resource
 * (textures, surfaces, buffers, shaders, effects, queries) so the engine can create, lock and fill them as it does on
 * Direct3D 9. Nothing is drawn yet. The adapter and capability queries still go to the real Direct3D 9 object the
 * shell creates; they only read the machine's display and format support.
 */
class GlDevice final : public RenderDevice {
public:
    int32_t reset(d3d_arg present_parameters) override;
    int32_t present(d3d_arg source_rect, d3d_arg dest_rect, d3d_arg dest_window, d3d_arg dirty_region) override;
    int32_t create_texture(uint32_t width, uint32_t height, uint32_t levels, uint32_t usage, uint32_t format, uint32_t pool, d3d_arg out_texture, d3d_arg shared_handle) override;
    int32_t create_volume_texture(uint32_t width, uint32_t height, uint32_t depth, uint32_t levels, uint32_t usage, uint32_t format, uint32_t pool, d3d_arg out_texture, d3d_arg shared_handle) override;
    int32_t create_cube_texture(uint32_t edge_length, uint32_t levels, uint32_t usage, uint32_t format, uint32_t pool, d3d_arg out_texture, d3d_arg shared_handle) override;
    int32_t create_vertex_buffer(uint32_t length, uint32_t usage, uint32_t fvf, uint32_t pool, d3d_arg out_buffer, d3d_arg shared_handle) override;
    int32_t create_index_buffer(uint32_t length, uint32_t usage, uint32_t format, uint32_t pool, d3d_arg out_buffer, d3d_arg shared_handle) override;
    int32_t stretch_rect(d3d_arg source_surface, d3d_arg source_rect, d3d_arg dest_surface, d3d_arg dest_rect, uint32_t filter) override;
    int32_t create_offscreen_plain_surface(uint32_t width, uint32_t height, uint32_t format, uint32_t pool, d3d_arg out_surface, d3d_arg shared_handle) override;
    int32_t set_render_target(uint32_t index, d3d_arg surface) override;
    int32_t get_render_target(uint32_t index, d3d_arg out_surface) override;
    int32_t begin_scene() override;
    int32_t end_scene() override;
    int32_t clear(uint32_t count, d3d_arg rects, uint32_t flags, uint32_t color, float z, uint32_t stencil) override;
    int32_t set_transform(uint32_t state, d3d_arg matrix) override;
    int32_t set_viewport(d3d_arg viewport) override;
    int32_t set_material(d3d_arg material) override;
    int32_t set_light(uint32_t index, d3d_arg light) override;
    int32_t light_enable(uint32_t index, int32_t enable) override;
    int32_t set_render_state(uint32_t state, uint32_t value) override;
    int32_t set_texture(uint32_t stage, d3d_arg texture) override;
    int32_t set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value) override;
    int32_t set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value) override;
    int32_t set_software_vertex_processing(uint32_t enable) override;
    int32_t draw_primitive(uint32_t type, uint32_t start_vertex, uint32_t primitive_count) override;
    int32_t draw_indexed_primitive(uint32_t type, int32_t base_vertex, uint32_t min_index, uint32_t vertex_count, uint32_t start_index, uint32_t primitive_count) override;
    int32_t draw_primitive_up(uint32_t type, uint32_t primitive_count, d3d_arg data, uint32_t stride) override;
    int32_t draw_indexed_primitive_up(uint32_t type, uint32_t min_index, uint32_t vertex_count, uint32_t primitive_count, d3d_arg index_data, uint32_t index_format, d3d_arg vertex_data, uint32_t stride) override;
    int32_t set_vertex_declaration(d3d_arg declaration) override;
    int32_t set_fvf(uint32_t fvf) override;
    int32_t set_vertex_shader(d3d_arg shader) override;
    int32_t set_vertex_shader_constant_f(uint32_t start_register, d3d_arg data, uint32_t vector4_count) override;
    int32_t set_stream_source(uint32_t stream, d3d_arg buffer, uint32_t offset, uint32_t stride) override;
    int32_t set_indices(d3d_arg index_buffer) override;
    int32_t set_pixel_shader(d3d_arg shader) override;
    int32_t set_pixel_shader_constant_f(uint32_t start_register, d3d_arg data, uint32_t vector4_count) override;
    int32_t get_display_mode(uint32_t swap_chain, d3d_arg mode) override;
    int32_t get_back_buffer(uint32_t swap_chain, uint32_t index, uint32_t type, d3d_arg out_surface) override;
    int32_t set_gamma_ramp(uint32_t swap_chain, uint32_t flags, d3d_arg ramp) override;
    int32_t process_vertices(uint32_t source_start, uint32_t dest_index, uint32_t vertex_count, d3d_arg dest_buffer, d3d_arg declaration, uint32_t flags) override;
    int32_t create_vertex_declaration(d3d_arg elements, d3d_arg out_declaration) override;
    int32_t create_vertex_shader(d3d_arg function, d3d_arg out_shader) override;
    int32_t create_pixel_shader(d3d_arg function, d3d_arg out_shader) override;
    int32_t create_query(uint32_t type, d3d_arg out_query) override;
    uint32_t release(d3d_arg object) override;
    uint32_t get_adapter_count(d3d_arg object) override;
    int32_t get_adapter_display_mode(d3d_arg object, uint32_t adapter, d3d_arg mode) override;
    int32_t check_device_format(d3d_arg object, uint32_t adapter, uint32_t device_type, uint32_t adapter_format, uint32_t usage, uint32_t resource_type, uint32_t check_format) override;
    int32_t get_device_caps(d3d_arg object, uint32_t adapter, uint32_t device_type, d3d_arg caps) override;
    int32_t create_device(d3d_arg object, uint32_t adapter, uint32_t device_type, d3d_arg focus_window, uint32_t behavior_flags, d3d_arg present_parameters, d3d_arg out_device) override;
    int32_t surface_get_desc(d3d_arg object, d3d_arg desc) override;
    int32_t surface_lock_rect(d3d_arg object, d3d_arg locked_rect, d3d_arg rect, uint32_t flags) override;
    int32_t surface_unlock_rect(d3d_arg object) override;
    int32_t buffer_lock(d3d_arg object, uint32_t offset, uint32_t size, d3d_arg out_data, uint32_t flags) override;
    int32_t buffer_unlock(d3d_arg object) override;
    int32_t buffer_get_desc(d3d_arg object, d3d_arg desc) override;
    int32_t texture_get_surface_level(d3d_arg object, uint32_t level, d3d_arg out_surface) override;
    int32_t texture_lock_rect(d3d_arg object, uint32_t level, d3d_arg locked_rect, d3d_arg rect, uint32_t flags) override;
    int32_t texture_unlock_rect(d3d_arg object, uint32_t level) override;
    int32_t volume_texture_lock_box(d3d_arg object, uint32_t level, d3d_arg locked_box, d3d_arg box, uint32_t flags) override;
    int32_t volume_texture_unlock_box(d3d_arg object, uint32_t level) override;
    int32_t cube_texture_lock_rect(d3d_arg object, uint32_t face, uint32_t level, d3d_arg locked_rect, d3d_arg rect, uint32_t flags) override;
    int32_t cube_texture_unlock_rect(d3d_arg object, uint32_t face, uint32_t level) override;
    int32_t cube_texture_get_surface(d3d_arg object, uint32_t face, uint32_t level, d3d_arg out_surface) override;
    int32_t query_issue(d3d_arg object, uint32_t flags) override;
    int32_t query_get_data(d3d_arg object, d3d_arg data, uint32_t size, uint32_t flags) override;
    int32_t effect_set_vector(d3d_arg object, d3d_arg handle, d3d_arg vector) override;
    int32_t effect_set_texture(d3d_arg object, d3d_arg handle, d3d_arg texture) override;
    int32_t effect_set_technique(d3d_arg object, d3d_arg technique) override;
    int32_t effect_validate_technique(d3d_arg object, d3d_arg technique) override;
    int32_t effect_find_next_valid_technique(d3d_arg object, d3d_arg technique, d3d_arg out_technique) override;
    int32_t effect_begin(d3d_arg object, d3d_arg out_pass_count, uint32_t flags) override;
    int32_t effect_pass(d3d_arg object, uint32_t pass) override;
    int32_t effect_end(d3d_arg object) override;
    int32_t effect_get_parameter_by_name(d3d_arg object, d3d_arg parent, d3d_arg name) override;
    int32_t effect_get_technique_by_name(d3d_arg object, d3d_arg name) override;
    int32_t effect_get_technique_by_name_scoped(d3d_arg object, d3d_arg parent, d3d_arg name) override;
};

/** The GL device instance. */
GlDevice &gl_device();

/** Creates a stand-in effect object (the real D3DX effect compiler needs a Direct3D 9 device). Returns 0. */
int32_t gl_create_effect(const void *data, uint32_t size, void *out_effect);

}  // namespace halo::rasterizer
