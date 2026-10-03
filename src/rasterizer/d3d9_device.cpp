/**
 * @file src/rasterizer/d3d9_device.cpp
 * Direct3D 9 implementation of the RenderDevice interface.
 */

#include "halo/rasterizer/render_device.hpp"
#include "halo/render/d3d9.hpp"
#include <type_traits>
#include "halo/core/link.hpp"
#include "halo/game/vars.hpp"
#include "halo/game/api.hpp"

static auto &rasterizer_device = halo::link::ref<void *>(halo::game::vars().rasterizer_device);

namespace halo::rasterizer {

namespace {

/** The function stored in `index` of the COM object's vtable. */
template <typename Fn>
Fn com_slot(void *object, uint32_t index)
{
    return reinterpret_cast<Fn>((*reinterpret_cast<void ***>(object))[index]);
}

/** The function the named method of the COM object's method table points at. */
template <typename Fn, typename Method>
    requires std::is_enum_v<Method>
Fn com_slot(void *object, Method method)
{
    return com_slot<Fn>(object, static_cast<uint32_t>(method));
}

}  // namespace

int32_t D3D9Device::reset(d3d_arg present_parameters)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, d3d9::device_method::reset)(self, present_parameters.get());
}

int32_t D3D9Device::present(d3d_arg source_rect, d3d_arg dest_rect, d3d_arg dest_window, d3d_arg dirty_region)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *, void *, void *, void *)>(self, d3d9::device_method::present)(self, source_rect.get(), dest_rect.get(), dest_window.get(), dirty_region.get());
}

int32_t D3D9Device::create_texture(uint32_t width, uint32_t height, uint32_t levels, uint32_t usage, uint32_t format, uint32_t pool, d3d_arg out_texture, d3d_arg shared_handle)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, void *, void *)>(self, d3d9::device_method::create_texture)(self, width, height, levels, usage, format, pool, out_texture.get(), shared_handle.get());
}

int32_t D3D9Device::create_volume_texture(uint32_t width, uint32_t height, uint32_t depth, uint32_t levels, uint32_t usage, uint32_t format, uint32_t pool, d3d_arg out_texture, d3d_arg shared_handle)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, void *, void *)>(self, d3d9::device_method::create_volume_texture)(self, width, height, depth, levels, usage, format, pool, out_texture.get(), shared_handle.get());
}

int32_t D3D9Device::create_cube_texture(uint32_t edge_length, uint32_t levels, uint32_t usage, uint32_t format, uint32_t pool, d3d_arg out_texture, d3d_arg shared_handle)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, void *, void *)>(self, d3d9::device_method::create_cube_texture)(self, edge_length, levels, usage, format, pool, out_texture.get(), shared_handle.get());
}

int32_t D3D9Device::create_vertex_buffer(uint32_t length, uint32_t usage, uint32_t fvf, uint32_t pool, d3d_arg out_buffer, d3d_arg shared_handle)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t, uint32_t, void *, void *)>(self, d3d9::device_method::create_vertex_buffer)(self, length, usage, fvf, pool, out_buffer.get(), shared_handle.get());
}

int32_t D3D9Device::create_index_buffer(uint32_t length, uint32_t usage, uint32_t format, uint32_t pool, d3d_arg out_buffer, d3d_arg shared_handle)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t, uint32_t, void *, void *)>(self, d3d9::device_method::create_index_buffer)(self, length, usage, format, pool, out_buffer.get(), shared_handle.get());
}

int32_t D3D9Device::stretch_rect(d3d_arg source_surface, d3d_arg source_rect, d3d_arg dest_surface, d3d_arg dest_rect, uint32_t filter)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *, void *, void *, void *, uint32_t)>(self, d3d9::device_method::stretch_rect)(self, source_surface.get(), source_rect.get(), dest_surface.get(), dest_rect.get(), filter);
}

int32_t D3D9Device::create_offscreen_plain_surface(uint32_t width, uint32_t height, uint32_t format, uint32_t pool, d3d_arg out_surface, d3d_arg shared_handle)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t, uint32_t, void *, void *)>(self, d3d9::device_method::create_offscreen_plain_surface)(self, width, height, format, pool, out_surface.get(), shared_handle.get());
}

int32_t D3D9Device::set_render_target(uint32_t index, d3d_arg surface)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *)>(self, d3d9::device_method::set_render_target)(self, index, surface.get());
}

int32_t D3D9Device::get_render_target(uint32_t index, d3d_arg out_surface)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *)>(self, d3d9::device_method::get_render_target)(self, index, out_surface.get());
}

int32_t D3D9Device::begin_scene()
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *self)>(self, d3d9::device_method::begin_scene)(self);
}

int32_t D3D9Device::end_scene()
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *self)>(self, d3d9::device_method::end_scene)(self);
}

int32_t D3D9Device::clear(uint32_t count, d3d_arg rects, uint32_t flags, uint32_t color, float z, uint32_t stencil)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *, uint32_t, uint32_t, float, uint32_t)>(self, d3d9::device_method::clear)(self, count, rects.get(), flags, color, z, stencil);
}

int32_t D3D9Device::set_transform(uint32_t state, d3d_arg matrix)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *)>(self, d3d9::device_method::set_transform)(self, state, matrix.get());
}

int32_t D3D9Device::set_viewport(d3d_arg viewport)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, d3d9::device_method::set_viewport)(self, viewport.get());
}

int32_t D3D9Device::set_material(d3d_arg material)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, d3d9::device_method::set_material)(self, material.get());
}

int32_t D3D9Device::set_light(uint32_t index, d3d_arg light)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *)>(self, d3d9::device_method::set_light)(self, index, light.get());
}

int32_t D3D9Device::light_enable(uint32_t index, int32_t enable)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, int32_t)>(self, d3d9::device_method::light_enable)(self, index, enable);
}

int32_t D3D9Device::set_render_state(uint32_t state, uint32_t value)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t)>(self, d3d9::device_method::set_render_state)(self, state, value);
}

int32_t D3D9Device::set_texture(uint32_t stage, d3d_arg texture)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *)>(self, d3d9::device_method::set_texture)(self, stage, texture.get());
}

int32_t D3D9Device::set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t)>(self, d3d9::device_method::set_texture_stage_state)(self, stage, type, value);
}

int32_t D3D9Device::set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t)>(self, d3d9::device_method::set_sampler_state)(self, sampler, type, value);
}

int32_t D3D9Device::set_software_vertex_processing(uint32_t enable)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t)>(self, d3d9::device_method::set_software_vertex_processing)(self, enable);
}

int32_t D3D9Device::draw_primitive(uint32_t type, uint32_t start_vertex, uint32_t primitive_count)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t)>(self, d3d9::device_method::draw_primitive)(self, type, start_vertex, primitive_count);
}

int32_t D3D9Device::draw_indexed_primitive(uint32_t type, int32_t base_vertex, uint32_t min_index, uint32_t vertex_count, uint32_t start_index, uint32_t primitive_count)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, int32_t, uint32_t, uint32_t, uint32_t, uint32_t)>(self, d3d9::device_method::draw_indexed_primitive)(self, type, base_vertex, min_index, vertex_count, start_index, primitive_count);
}

int32_t D3D9Device::draw_primitive_up(uint32_t type, uint32_t primitive_count, d3d_arg data, uint32_t stride)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, void *, uint32_t)>(self, d3d9::device_method::draw_primitive_up)(self, type, primitive_count, data.get(), stride);
}

int32_t D3D9Device::draw_indexed_primitive_up(uint32_t type, uint32_t min_index, uint32_t vertex_count, uint32_t primitive_count, d3d_arg index_data, uint32_t index_format, d3d_arg vertex_data, uint32_t stride)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t, uint32_t, void *, uint32_t, void *, uint32_t)>(self, d3d9::device_method::draw_indexed_primitive_up)(self, type, min_index, vertex_count, primitive_count, index_data.get(), index_format, vertex_data.get(), stride);
}

int32_t D3D9Device::set_vertex_declaration(d3d_arg declaration)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, d3d9::device_method::set_vertex_declaration)(self, declaration.get());
}

int32_t D3D9Device::set_fvf(uint32_t fvf)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t)>(self, d3d9::device_method::set_fvf)(self, fvf);
}

int32_t D3D9Device::set_vertex_shader(d3d_arg shader)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, d3d9::device_method::set_vertex_shader)(self, shader.get());
}

int32_t D3D9Device::set_vertex_shader_constant_f(uint32_t start_register, d3d_arg data, uint32_t vector4_count)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *, uint32_t)>(self, d3d9::device_method::set_vertex_shader_constant_f)(self, start_register, data.get(), vector4_count);
}

int32_t D3D9Device::set_stream_source(uint32_t stream, d3d_arg buffer, uint32_t offset, uint32_t stride)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *, uint32_t, uint32_t)>(self, d3d9::device_method::set_stream_source)(self, stream, buffer.get(), offset, stride);
}

int32_t D3D9Device::set_indices(d3d_arg index_buffer)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, d3d9::device_method::set_indices)(self, index_buffer.get());
}

int32_t D3D9Device::set_pixel_shader(d3d_arg shader)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, d3d9::device_method::set_pixel_shader)(self, shader.get());
}

int32_t D3D9Device::set_pixel_shader_constant_f(uint32_t start_register, d3d_arg data, uint32_t vector4_count)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *, uint32_t)>(self, d3d9::device_method::set_pixel_shader_constant_f)(self, start_register, data.get(), vector4_count);
}

int32_t D3D9Device::get_back_buffer(uint32_t swap_chain, uint32_t index, uint32_t type, d3d_arg out_surface)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t, void *)>(self, d3d9::device_method::get_back_buffer)(self, swap_chain, index, type, out_surface.get());
}

int32_t D3D9Device::set_gamma_ramp(uint32_t swap_chain, uint32_t flags, d3d_arg ramp)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, void *)>(self, d3d9::device_method::set_gamma_ramp)(self, swap_chain, flags, ramp.get());
}

int32_t D3D9Device::process_vertices(uint32_t source_start, uint32_t dest_index, uint32_t vertex_count, d3d_arg dest_buffer, d3d_arg declaration, uint32_t flags)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t, void *, void *, uint32_t)>(self, d3d9::device_method::process_vertices)(self, source_start, dest_index, vertex_count, dest_buffer.get(), declaration.get(), flags);
}

int32_t D3D9Device::create_vertex_declaration(d3d_arg elements, d3d_arg out_declaration)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *, void *)>(self, d3d9::device_method::create_vertex_declaration)(self, elements.get(), out_declaration.get());
}

int32_t D3D9Device::create_vertex_shader(d3d_arg function, d3d_arg out_shader)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *, void *)>(self, d3d9::device_method::create_vertex_shader)(self, function.get(), out_shader.get());
}

int32_t D3D9Device::create_pixel_shader(d3d_arg function, d3d_arg out_shader)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *, void *)>(self, d3d9::device_method::create_pixel_shader)(self, function.get(), out_shader.get());
}

int32_t D3D9Device::create_query(uint32_t type, d3d_arg out_query)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *)>(self, d3d9::device_method::create_query)(self, type, out_query.get());
}

uint32_t D3D9Device::release(d3d_arg object)
{
    void *self = object.get();
    return com_slot<uint32_t (__stdcall *)(void *self)>(self, d3d9::device_method::release)(self);
}

uint32_t D3D9Device::get_adapter_count(d3d_arg object)
{
    void *self = object.get();
    return com_slot<uint32_t (__stdcall *)(void *self)>(self, d3d9::direct3d_method::get_adapter_count)(self);
}

int32_t D3D9Device::get_adapter_display_mode(d3d_arg object, uint32_t adapter, d3d_arg mode)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *)>(self, d3d9::direct3d_method::get_adapter_display_mode)(self, adapter, mode.get());
}

int32_t D3D9Device::check_device_format(d3d_arg object, uint32_t adapter, uint32_t device_type, uint32_t adapter_format, uint32_t usage, uint32_t resource_type, uint32_t check_format)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>(self, d3d9::direct3d_method::check_device_format)(self, adapter, device_type, adapter_format, usage, resource_type, check_format);
}

int32_t D3D9Device::get_device_caps(d3d_arg object, uint32_t adapter, uint32_t device_type, d3d_arg caps)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, void *)>(self, d3d9::direct3d_method::get_device_caps)(self, adapter, device_type, caps.get());
}

int32_t D3D9Device::create_device(d3d_arg object, uint32_t adapter, uint32_t device_type, d3d_arg focus_window, uint32_t behavior_flags, d3d_arg present_parameters, d3d_arg out_device)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, void *, uint32_t, void *, void *)>(self, d3d9::direct3d_method::create_device)(self, adapter, device_type, focus_window.get(), behavior_flags, present_parameters.get(), out_device.get());
}

int32_t D3D9Device::surface_get_desc(d3d_arg object, d3d_arg desc)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, d3d9::surface_method::get_desc)(self, desc.get());
}

int32_t D3D9Device::surface_lock_rect(d3d_arg object, d3d_arg locked_rect, d3d_arg rect, uint32_t flags)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *, void *, uint32_t)>(self, d3d9::surface_method::lock_rect)(self, locked_rect.get(), rect.get(), flags);
}

int32_t D3D9Device::surface_unlock_rect(d3d_arg object)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *self)>(self, d3d9::surface_method::unlock_rect)(self);
}

int32_t D3D9Device::buffer_lock(d3d_arg object, uint32_t offset, uint32_t size, d3d_arg out_data, uint32_t flags)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, void *, uint32_t)>(self, d3d9::buffer_method::lock)(self, offset, size, out_data.get(), flags);
}

int32_t D3D9Device::buffer_unlock(d3d_arg object)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *self)>(self, d3d9::buffer_method::unlock)(self);
}

int32_t D3D9Device::buffer_get_desc(d3d_arg object, d3d_arg desc)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, d3d9::buffer_method::get_desc)(self, desc.get());
}

int32_t D3D9Device::texture_get_surface_level(d3d_arg object, uint32_t level, d3d_arg out_surface)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *)>(self, d3d9::texture_method::get_surface_level)(self, level, out_surface.get());
}

int32_t D3D9Device::texture_lock_rect(d3d_arg object, uint32_t level, d3d_arg locked_rect, d3d_arg rect, uint32_t flags)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *, void *, uint32_t)>(self, d3d9::texture_method::lock_rect)(self, level, locked_rect.get(), rect.get(), flags);
}

int32_t D3D9Device::texture_unlock_rect(d3d_arg object, uint32_t level)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t)>(self, d3d9::texture_method::unlock_rect)(self, level);
}

int32_t D3D9Device::volume_texture_lock_box(d3d_arg object, uint32_t level, d3d_arg locked_box, d3d_arg box, uint32_t flags)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *, void *, uint32_t)>(self, d3d9::texture_method::lock_box)(self, level, locked_box.get(), box.get(), flags);
}

int32_t D3D9Device::volume_texture_unlock_box(d3d_arg object, uint32_t level)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t)>(self, d3d9::texture_method::unlock_box)(self, level);
}

int32_t D3D9Device::cube_texture_lock_rect(d3d_arg object, uint32_t face, uint32_t level, d3d_arg locked_rect, d3d_arg rect, uint32_t flags)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, void *, void *, uint32_t)>(self, d3d9::texture_method::lock_rect)(self, face, level, locked_rect.get(), rect.get(), flags);
}

int32_t D3D9Device::cube_texture_unlock_rect(d3d_arg object, uint32_t face, uint32_t level)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t)>(self, d3d9::texture_method::unlock_rect)(self, face, level);
}

int32_t D3D9Device::cube_texture_get_surface(d3d_arg object, uint32_t face, uint32_t level, d3d_arg out_surface)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, void *)>(self, d3d9::texture_method::get_cube_map_surface)(self, face, level, out_surface.get());
}

int32_t D3D9Device::query_issue(d3d_arg object, uint32_t flags)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t)>(self, d3d9::query_method::issue)(self, flags);
}

int32_t D3D9Device::query_get_data(d3d_arg object, d3d_arg data, uint32_t size, uint32_t flags)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *, uint32_t, uint32_t)>(self, d3d9::query_method::get_data)(self, data.get(), size, flags);
}

int32_t D3D9Device::effect_set_vector(d3d_arg object, d3d_arg handle, d3d_arg vector)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *, void *)>(self, d3d9::effect_method::set_vector)(self, handle.get(), vector.get());
}

int32_t D3D9Device::effect_set_texture(d3d_arg object, d3d_arg handle, d3d_arg texture)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *, void *)>(self, d3d9::effect_method::set_texture)(self, handle.get(), texture.get());
}

int32_t D3D9Device::effect_set_technique(d3d_arg object, d3d_arg technique)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, d3d9::effect_method::set_technique)(self, technique.get());
}

int32_t D3D9Device::effect_validate_technique(d3d_arg object, d3d_arg technique)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, d3d9::effect_method::validate_technique)(self, technique.get());
}

int32_t D3D9Device::effect_find_next_valid_technique(d3d_arg object, d3d_arg technique, d3d_arg out_technique)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *, void *)>(self, d3d9::effect_method::find_next_valid_technique)(self, technique.get(), out_technique.get());
}

int32_t D3D9Device::effect_begin(d3d_arg object, d3d_arg out_pass_count, uint32_t flags)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *, uint32_t)>(self, d3d9::effect_method::begin)(self, out_pass_count.get(), flags);
}

int32_t D3D9Device::effect_pass(d3d_arg object, uint32_t pass)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t)>(self, d3d9::effect_method::begin_pass)(self, pass);
}

int32_t D3D9Device::effect_end(d3d_arg object)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *self)>(self, d3d9::effect_method::end)(self);
}

int32_t D3D9Device::effect_get_parameter_by_name(d3d_arg object, d3d_arg parent, d3d_arg name)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *, void *)>(self, d3d9::effect_method::get_parameter_by_name)(self, parent.get(), name.get());
}

int32_t D3D9Device::effect_get_technique_by_name(d3d_arg object, d3d_arg name)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, d3d9::effect_method::get_technique_by_name)(self, name.get());
}

int32_t D3D9Device::effect_get_technique_by_name_scoped(d3d_arg object, d3d_arg parent, d3d_arg name)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *, void *)>(self, d3d9::effect_method::get_technique_by_name)(self, parent.get(), name.get());
}

D3D9Device &d3d9_device()
{
    static D3D9Device device;
    return device;
}

RenderDevice &render_device()
{
    return d3d9_device();
}

}  // namespace halo::rasterizer
