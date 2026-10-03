/**
 * @file include/halo/render/d3d9.hpp
 * Named Direct3D 9 method slots, render states and capability bits used where the engine calls the device through its method table.
 */
#pragma once

#include <cstdint>
#include "halo/core/flags.hpp"

namespace halo::d3d9 {

/** Index of an IDirect3DDevice9 method in its method table (byte offset / 4). */
enum class device_method : uint32_t {
    query_interface = 0,
    add_ref = 1,
    release = 2,
    test_cooperative_level = 3,
    get_available_texture_mem = 4,
    evict_managed_resources = 5,
    get_direct3d = 6,
    get_device_caps = 7,
    get_display_mode = 8,
    get_creation_parameters = 9,
    set_cursor_properties = 10,
    set_cursor_position = 11,
    show_cursor = 12,
    create_additional_swap_chain = 13,
    get_swap_chain = 14,
    get_number_of_swap_chains = 15,
    reset = 16,
    present = 17,
    get_back_buffer = 18,
    get_raster_status = 19,
    set_dialog_box_mode = 20,
    set_gamma_ramp = 21,
    get_gamma_ramp = 22,
    create_texture = 23,
    create_volume_texture = 24,
    create_cube_texture = 25,
    create_vertex_buffer = 26,
    create_index_buffer = 27,
    create_render_target = 28,
    create_depth_stencil_surface = 29,
    update_surface = 30,
    update_texture = 31,
    get_render_target_data = 32,
    get_front_buffer_data = 33,
    stretch_rect = 34,
    color_fill = 35,
    create_offscreen_plain_surface = 36,
    set_render_target = 37,
    get_render_target = 38,
    set_depth_stencil_surface = 39,
    get_depth_stencil_surface = 40,
    begin_scene = 41,
    end_scene = 42,
    clear = 43,
    set_transform = 44,
    get_transform = 45,
    multiply_transform = 46,
    set_viewport = 47,
    get_viewport = 48,
    set_material = 49,
    get_material = 50,
    set_light = 51,
    get_light = 52,
    light_enable = 53,
    get_light_enable = 54,
    set_clip_plane = 55,
    get_clip_plane = 56,
    set_render_state = 57,
    get_render_state = 58,
    create_state_block = 59,
    begin_state_block = 60,
    end_state_block = 61,
    set_clip_status = 62,
    get_clip_status = 63,
    get_texture = 64,
    set_texture = 65,
    get_texture_stage_state = 66,
    set_texture_stage_state = 67,
    get_sampler_state = 68,
    set_sampler_state = 69,
    validate_device = 70,
    set_palette_entries = 71,
    get_palette_entries = 72,
    set_current_texture_palette = 73,
    get_current_texture_palette = 74,
    set_scissor_rect = 75,
    get_scissor_rect = 76,
    set_software_vertex_processing = 77,
    get_software_vertex_processing = 78,
    set_npatch_mode = 79,
    get_npatch_mode = 80,
    draw_primitive = 81,
    draw_indexed_primitive = 82,
    draw_primitive_up = 83,
    draw_indexed_primitive_up = 84,
    process_vertices = 85,
    create_vertex_declaration = 86,
    set_vertex_declaration = 87,
    get_vertex_declaration = 88,
    set_fvf = 89,
    get_fvf = 90,
    create_vertex_shader = 91,
    set_vertex_shader = 92,
    get_vertex_shader = 93,
    set_vertex_shader_constant_f = 94,
    get_vertex_shader_constant_f = 95,
    set_vertex_shader_constant_i = 96,
    get_vertex_shader_constant_i = 97,
    set_vertex_shader_constant_b = 98,
    get_vertex_shader_constant_b = 99,
    set_stream_source = 100,
    get_stream_source = 101,
    set_stream_source_freq = 102,
    get_stream_source_freq = 103,
    set_indices = 104,
    get_indices = 105,
    create_pixel_shader = 106,
    set_pixel_shader = 107,
    get_pixel_shader = 108,
    set_pixel_shader_constant_f = 109,
};

/** Index of a method shared by IDirect3DVertexBuffer9 and IDirect3DIndexBuffer9. */
enum class buffer_method : uint32_t {
    release = 2,
    lock = 11,
    unlock = 12,
    get_desc = 13,
};

/** D3DRENDERSTATETYPE values the engine sets. */
enum class render_state : uint32_t {
    z_enable = 7,
    fill_mode = 8,
    shade_mode = 9,
    z_write_enable = 14,
    alpha_test_enable = 15,
    src_blend = 19,
    dest_blend = 20,
    cull_mode = 22,
    z_func = 23,
    alpha_ref = 24,
    alpha_func = 25,
    alpha_blend_enable = 27,
    fog_enable = 28,
    specular_enable = 29,
    fog_color = 34,
    texture_factor = 60,
    lighting = 137,
    ambient = 139,
    point_sprite_enable = 156,
    point_scale_enable = 157,
    color_write_enable = 168,
    slope_scale_depth_bias = 175,
    depth_bias = 195,
};

/** D3DTEXTURESTAGESTATETYPE values the engine sets. */
enum class texture_stage_state : uint32_t {
    color_op = 1,
    color_arg1 = 2,
    color_arg2 = 3,
    alpha_op = 4,
    alpha_arg1 = 5,
    alpha_arg2 = 6,
    texcoord_index = 11,
    texture_transform_flags = 24,
};

/** Plain-integer D3DRENDERSTATETYPE ids for call sites that pass them as `uint32_t`. */
namespace rs {
inline constexpr uint32_t z_enable = 7;
inline constexpr uint32_t fill_mode = 8;
inline constexpr uint32_t z_write_enable = 14;
inline constexpr uint32_t alpha_test_enable = 15;
inline constexpr uint32_t src_blend = 19;
inline constexpr uint32_t dest_blend = 20;
inline constexpr uint32_t cull_mode = 22;
inline constexpr uint32_t z_func = 23;
inline constexpr uint32_t alpha_ref = 24;
inline constexpr uint32_t alpha_func = 25;
inline constexpr uint32_t alpha_blend_enable = 27;
inline constexpr uint32_t fog_enable = 28;
inline constexpr uint32_t fog_color = 34;
inline constexpr uint32_t fog_table_mode = 35;
inline constexpr uint32_t fog_start = 36;
inline constexpr uint32_t fog_end = 37;
inline constexpr uint32_t texture_factor = 60;
inline constexpr uint32_t lighting = 137;
inline constexpr uint32_t ambient = 139;
inline constexpr uint32_t fog_vertex_mode = 140;
inline constexpr uint32_t color_write_enable = 168;
inline constexpr uint32_t blend_op = 171;
inline constexpr uint32_t slope_scale_depth_bias = 175;
inline constexpr uint32_t depth_bias = 195;
}  // namespace rs

/** Plain-integer D3DTEXTURESTAGESTATETYPE ids. */
namespace ts {
inline constexpr uint32_t color_op = 1;
inline constexpr uint32_t color_arg1 = 2;
inline constexpr uint32_t color_arg2 = 3;
inline constexpr uint32_t alpha_op = 4;
inline constexpr uint32_t alpha_arg1 = 5;
inline constexpr uint32_t alpha_arg2 = 6;
inline constexpr uint32_t texcoord_index = 11;
inline constexpr uint32_t texture_transform_flags = 24;
inline constexpr uint32_t color_arg0 = 26;
}  // namespace ts

/** Plain-integer D3DSAMPLERSTATETYPE ids. */
namespace ss {
inline constexpr uint32_t address_u = 1;
inline constexpr uint32_t address_v = 2;
inline constexpr uint32_t address_w = 3;
inline constexpr uint32_t border_color = 4;
inline constexpr uint32_t mag_filter = 5;
inline constexpr uint32_t min_filter = 6;
inline constexpr uint32_t mip_filter = 7;
inline constexpr uint32_t max_anisotropy = 10;
}  // namespace ss

/** D3DBLEND values. */
namespace blend {
inline constexpr uint32_t zero = 1;
inline constexpr uint32_t one = 2;
inline constexpr uint32_t src_color = 3;
inline constexpr uint32_t inv_src_color = 4;
inline constexpr uint32_t src_alpha = 5;
inline constexpr uint32_t inv_src_alpha = 6;
inline constexpr uint32_t dest_alpha = 7;
inline constexpr uint32_t inv_dest_alpha = 8;
inline constexpr uint32_t dest_color = 9;
inline constexpr uint32_t inv_dest_color = 10;
}  // namespace blend

/** D3DTEXTUREOP values. */
enum class texture_op : uint32_t {
    disable = 1,
    select_arg1 = 2,
    select_arg2 = 3,
    modulate = 4,
};

/** D3DTA_* texture argument sources. */
inline constexpr uint32_t k_texture_argument_diffuse = 0;
inline constexpr uint32_t k_texture_argument_current = 1;
inline constexpr uint32_t k_texture_argument_texture = 2;

/** D3DCULL_* values. */
inline constexpr uint32_t k_cull_none = 1;
inline constexpr uint32_t k_cull_cw = 2;
inline constexpr uint32_t k_cull_ccw = 3;

/** D3DPS_VERSION(1, 1): pixel shader versions encoded the way the device capabilities report them. */
inline constexpr uint32_t k_pixel_shader_version_1_1 = 0xffff0101;

/** D3DCOLORWRITEENABLE_RED | GREEN | BLUE | ALPHA. */
inline constexpr uint32_t k_color_write_all = 0xf;

/** D3DUSAGE_SOFTWAREPROCESSING. */
inline constexpr uint32_t k_usage_software_processing = 0x10;

/** D3DLOCK_NOOVERWRITE and D3DLOCK_DISCARD. */
inline constexpr uint32_t k_lock_no_overwrite = 0x1000;
inline constexpr uint32_t k_lock_discard = 0x2000;

/** D3DPRIMITIVETYPE values. */
inline constexpr uint32_t k_primitive_line_strip = 3;

/** D3DPRASTERCAPS bits. */
enum class raster_cap : uint32_t {
    none = 0,
    slope_scale_depth_bias = 0x02000000,
    depth_bias = 0x04000000,
};

/** Index of a method of IDirect3DSurface9. */
enum class surface_method : uint32_t {
    release = 2,
    lock_rect = 13,
    unlock_rect = 14,
};

/** D3DFMT_X8R8G8B8. */
inline constexpr uint32_t k_format_x8r8g8b8 = 22;

/** D3DERR_DEVICENOTRESET. */
inline constexpr int32_t k_error_device_not_reset = static_cast<int32_t>(0x88760869);

/** The method table (first dword of a COM object) as an array of function pointers. */
inline void **method_table(void *com_object) noexcept { return *reinterpret_cast<void ***>(com_object); }

/** Function pointer of a device method. */
template <typename Fn>
inline Fn device_function(void *device, device_method method) noexcept {
    return reinterpret_cast<Fn>(method_table(device)[static_cast<uint32_t>(method)]);
}

/** Function pointer of a buffer method. */
template <typename Fn>
inline Fn surface_function(void *surface, surface_method method) noexcept {
    return reinterpret_cast<Fn>(method_table(surface)[static_cast<uint32_t>(method)]);
}

template <typename Fn>
inline Fn buffer_function(void *buffer, buffer_method method) noexcept {
    return reinterpret_cast<Fn>(method_table(buffer)[static_cast<uint32_t>(method)]);
}

}  // namespace halo::d3d9

namespace halo {
template <> struct enable_bit_flags<d3d9::raster_cap> : std::true_type {};
}  // namespace halo
