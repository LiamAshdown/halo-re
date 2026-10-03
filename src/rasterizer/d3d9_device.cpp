/**
 * @file src/rasterizer/d3d9_device.cpp
 * Direct3D 9 implementation of the RenderDevice interface.
 */

#include "halo/rasterizer/render_device.hpp"

extern "C" void *rasterizer_device;

namespace halo::rasterizer {

namespace {

/** The function stored in `index` of the COM object's vtable. */
template <typename Fn>
Fn com_slot(void *object, uint32_t index)
{
    return reinterpret_cast<Fn>((*reinterpret_cast<void ***>(object))[index]);
}

}  // namespace

int32_t D3D9Device::reset(d3d_arg present_parameters)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, 16)(self, present_parameters.get());
}

int32_t D3D9Device::present(d3d_arg source_rect, d3d_arg dest_rect, d3d_arg dest_window, d3d_arg dirty_region)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *, void *, void *, void *)>(self, 17)(self, source_rect.get(), dest_rect.get(), dest_window.get(), dirty_region.get());
}

int32_t D3D9Device::create_texture(uint32_t width, uint32_t height, uint32_t levels, uint32_t usage, uint32_t format, uint32_t pool, d3d_arg out_texture, d3d_arg shared_handle)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, void *, void *)>(self, 23)(self, width, height, levels, usage, format, pool, out_texture.get(), shared_handle.get());
}

int32_t D3D9Device::create_vertex_buffer(uint32_t length, uint32_t usage, uint32_t fvf, uint32_t pool, d3d_arg out_buffer, d3d_arg shared_handle)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t, uint32_t, void *, void *)>(self, 26)(self, length, usage, fvf, pool, out_buffer.get(), shared_handle.get());
}

int32_t D3D9Device::create_index_buffer(uint32_t length, uint32_t usage, uint32_t format, uint32_t pool, d3d_arg out_buffer, d3d_arg shared_handle)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t, uint32_t, void *, void *)>(self, 27)(self, length, usage, format, pool, out_buffer.get(), shared_handle.get());
}

int32_t D3D9Device::stretch_rect(d3d_arg source_surface, d3d_arg source_rect, d3d_arg dest_surface, d3d_arg dest_rect, uint32_t filter)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *, void *, void *, void *, uint32_t)>(self, 34)(self, source_surface.get(), source_rect.get(), dest_surface.get(), dest_rect.get(), filter);
}

int32_t D3D9Device::create_offscreen_plain_surface(uint32_t width, uint32_t height, uint32_t format, uint32_t pool, d3d_arg out_surface, d3d_arg shared_handle)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t, uint32_t, void *, void *)>(self, 36)(self, width, height, format, pool, out_surface.get(), shared_handle.get());
}

int32_t D3D9Device::set_render_target(uint32_t index, d3d_arg surface)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *)>(self, 37)(self, index, surface.get());
}

int32_t D3D9Device::get_render_target(uint32_t index, d3d_arg out_surface)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *)>(self, 38)(self, index, out_surface.get());
}

int32_t D3D9Device::begin_scene()
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *)>(self, 41)(self);
}

int32_t D3D9Device::end_scene()
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *)>(self, 42)(self);
}

int32_t D3D9Device::clear(uint32_t count, d3d_arg rects, uint32_t flags, uint32_t color, float z, uint32_t stencil)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *, uint32_t, uint32_t, float, uint32_t)>(self, 43)(self, count, rects.get(), flags, color, z, stencil);
}

int32_t D3D9Device::set_transform(uint32_t state, d3d_arg matrix)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *)>(self, 44)(self, state, matrix.get());
}

int32_t D3D9Device::set_viewport(d3d_arg viewport)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, 47)(self, viewport.get());
}

int32_t D3D9Device::set_material(d3d_arg material)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, 49)(self, material.get());
}

int32_t D3D9Device::set_light(uint32_t index, d3d_arg light)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *)>(self, 51)(self, index, light.get());
}

int32_t D3D9Device::light_enable(uint32_t index, int32_t enable)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, int32_t)>(self, 53)(self, index, enable);
}

int32_t D3D9Device::set_render_state(uint32_t state, uint32_t value)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t)>(self, 57)(self, state, value);
}

int32_t D3D9Device::set_texture(uint32_t stage, d3d_arg texture)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *)>(self, 65)(self, stage, texture.get());
}

int32_t D3D9Device::set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t)>(self, 67)(self, stage, type, value);
}

int32_t D3D9Device::set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t)>(self, 69)(self, sampler, type, value);
}

int32_t D3D9Device::set_software_vertex_processing(uint32_t enable)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t)>(self, 77)(self, enable);
}

int32_t D3D9Device::draw_primitive(uint32_t type, uint32_t start_vertex, uint32_t primitive_count)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t)>(self, 81)(self, type, start_vertex, primitive_count);
}

int32_t D3D9Device::draw_indexed_primitive(uint32_t type, int32_t base_vertex, uint32_t min_index, uint32_t vertex_count, uint32_t start_index, uint32_t primitive_count)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, int32_t, uint32_t, uint32_t, uint32_t, uint32_t)>(self, 82)(self, type, base_vertex, min_index, vertex_count, start_index, primitive_count);
}

int32_t D3D9Device::draw_primitive_up(uint32_t type, uint32_t primitive_count, d3d_arg data, uint32_t stride)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, void *, uint32_t)>(self, 83)(self, type, primitive_count, data.get(), stride);
}

int32_t D3D9Device::draw_indexed_primitive_up(uint32_t type, uint32_t min_index, uint32_t vertex_count, uint32_t primitive_count, d3d_arg index_data, uint32_t index_format, d3d_arg vertex_data, uint32_t stride)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t, uint32_t, void *, uint32_t, void *, uint32_t)>(self, 84)(self, type, min_index, vertex_count, primitive_count, index_data.get(), index_format, vertex_data.get(), stride);
}

int32_t D3D9Device::set_vertex_declaration(d3d_arg declaration)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, 87)(self, declaration.get());
}

int32_t D3D9Device::set_fvf(uint32_t fvf)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t)>(self, 89)(self, fvf);
}

int32_t D3D9Device::set_vertex_shader(d3d_arg shader)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, 92)(self, shader.get());
}

int32_t D3D9Device::set_vertex_shader_constant_f(uint32_t start_register, d3d_arg data, uint32_t vector4_count)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *, uint32_t)>(self, 94)(self, start_register, data.get(), vector4_count);
}

int32_t D3D9Device::set_stream_source(uint32_t stream, d3d_arg buffer, uint32_t offset, uint32_t stride)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *, uint32_t, uint32_t)>(self, 100)(self, stream, buffer.get(), offset, stride);
}

int32_t D3D9Device::set_indices(d3d_arg index_buffer)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, 104)(self, index_buffer.get());
}

int32_t D3D9Device::set_pixel_shader(d3d_arg shader)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, 107)(self, shader.get());
}

int32_t D3D9Device::set_pixel_shader_constant_f(uint32_t start_register, d3d_arg data, uint32_t vector4_count)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *, uint32_t)>(self, 109)(self, start_register, data.get(), vector4_count);
}

int32_t D3D9Device::get_back_buffer(uint32_t swap_chain, uint32_t index, uint32_t type, d3d_arg out_surface)
{
    void *self = rasterizer_device;
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t, void *)>(self, 18)(self, swap_chain, index, type, out_surface.get());
}

uint32_t D3D9Device::release(d3d_arg object)
{
    void *self = object.get();
    return com_slot<uint32_t (__stdcall *)(void *)>(self, 2)(self);
}

uint32_t D3D9Device::get_adapter_count(d3d_arg object)
{
    void *self = object.get();
    return com_slot<uint32_t (__stdcall *)(void *)>(self, 4)(self);
}

int32_t D3D9Device::get_adapter_display_mode(d3d_arg object, uint32_t adapter, d3d_arg mode)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *)>(self, 8)(self, adapter, mode.get());
}

int32_t D3D9Device::check_device_format(d3d_arg object, uint32_t adapter, uint32_t device_type, uint32_t adapter_format, uint32_t usage, uint32_t resource_type, uint32_t check_format)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)>(self, 10)(self, adapter, device_type, adapter_format, usage, resource_type, check_format);
}

int32_t D3D9Device::get_device_caps(d3d_arg object, uint32_t adapter, uint32_t device_type, d3d_arg caps)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, void *)>(self, 14)(self, adapter, device_type, caps.get());
}

int32_t D3D9Device::create_device(d3d_arg object, uint32_t adapter, uint32_t device_type, d3d_arg focus_window, uint32_t behavior_flags, d3d_arg present_parameters, d3d_arg out_device)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, void *, uint32_t, void *, void *)>(self, 16)(self, adapter, device_type, focus_window.get(), behavior_flags, present_parameters.get(), out_device.get());
}

int32_t D3D9Device::surface_get_desc(d3d_arg object, d3d_arg desc)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, 12)(self, desc.get());
}

int32_t D3D9Device::surface_lock_rect(d3d_arg object, d3d_arg locked_rect, d3d_arg rect, uint32_t flags)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *, void *, uint32_t)>(self, 13)(self, locked_rect.get(), rect.get(), flags);
}

int32_t D3D9Device::surface_unlock_rect(d3d_arg object)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *)>(self, 14)(self);
}

int32_t D3D9Device::buffer_lock(d3d_arg object, uint32_t offset, uint32_t size, d3d_arg out_data, uint32_t flags)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, void *, uint32_t)>(self, 11)(self, offset, size, out_data.get(), flags);
}

int32_t D3D9Device::buffer_unlock(d3d_arg object)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *)>(self, 12)(self);
}

int32_t D3D9Device::buffer_get_desc(d3d_arg object, d3d_arg desc)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, 13)(self, desc.get());
}

int32_t D3D9Device::texture_get_surface_level(d3d_arg object, uint32_t level, d3d_arg out_surface)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *)>(self, 18)(self, level, out_surface.get());
}

int32_t D3D9Device::texture_lock_rect(d3d_arg object, uint32_t level, d3d_arg locked_rect, d3d_arg rect, uint32_t flags)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *, void *, uint32_t)>(self, 19)(self, level, locked_rect.get(), rect.get(), flags);
}

int32_t D3D9Device::texture_unlock_rect(d3d_arg object, uint32_t level)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t)>(self, 20)(self, level);
}

int32_t D3D9Device::volume_texture_lock_box(d3d_arg object, uint32_t level, d3d_arg locked_box, d3d_arg box, uint32_t flags)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, void *, void *, uint32_t)>(self, 19)(self, level, locked_box.get(), box.get(), flags);
}

int32_t D3D9Device::volume_texture_unlock_box(d3d_arg object, uint32_t level)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t)>(self, 20)(self, level);
}

int32_t D3D9Device::cube_texture_lock_rect(d3d_arg object, uint32_t face, uint32_t level, d3d_arg locked_rect, d3d_arg rect, uint32_t flags)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, void *, void *, uint32_t)>(self, 19)(self, face, level, locked_rect.get(), rect.get(), flags);
}

int32_t D3D9Device::cube_texture_unlock_rect(d3d_arg object, uint32_t face, uint32_t level)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t)>(self, 20)(self, face, level);
}

int32_t D3D9Device::cube_texture_get_surface(d3d_arg object, uint32_t face, uint32_t level, d3d_arg out_surface)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t, uint32_t, void *)>(self, 18)(self, face, level, out_surface.get());
}

int32_t D3D9Device::query_issue(d3d_arg object, uint32_t flags)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t)>(self, 6)(self, flags);
}

int32_t D3D9Device::effect_set_vector(d3d_arg object, d3d_arg handle, d3d_arg vector)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *, void *)>(self, 34)(self, handle.get(), vector.get());
}

int32_t D3D9Device::effect_set_texture(d3d_arg object, d3d_arg handle, d3d_arg texture)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *, void *)>(self, 52)(self, handle.get(), texture.get());
}

int32_t D3D9Device::effect_set_technique(d3d_arg object, d3d_arg technique)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, 59)(self, technique.get());
}

int32_t D3D9Device::effect_validate_technique(d3d_arg object, d3d_arg technique)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, 61)(self, technique.get());
}

int32_t D3D9Device::effect_find_next_valid_technique(d3d_arg object, d3d_arg technique, d3d_arg out_technique)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *, void *)>(self, 62)(self, technique.get(), out_technique.get());
}

int32_t D3D9Device::effect_begin(d3d_arg object, d3d_arg out_pass_count, uint32_t flags)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *, uint32_t)>(self, 64)(self, out_pass_count.get(), flags);
}

int32_t D3D9Device::effect_pass(d3d_arg object, uint32_t pass)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, uint32_t)>(self, 65)(self, pass);
}

int32_t D3D9Device::effect_end(d3d_arg object)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *)>(self, 66)(self);
}

int32_t D3D9Device::effect_get_parameter_by_name(d3d_arg object, d3d_arg parent, d3d_arg name)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *, void *)>(self, 9)(self, parent.get(), name.get());
}

int32_t D3D9Device::effect_get_technique_by_name(d3d_arg object, d3d_arg name)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *)>(self, 13)(self, name.get());
}

int32_t D3D9Device::effect_get_technique_by_name_scoped(d3d_arg object, d3d_arg parent, d3d_arg name)
{
    void *self = object.get();
    return com_slot<int32_t (__stdcall *)(void *, void *, void *)>(self, 13)(self, parent.get(), name.get());
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
