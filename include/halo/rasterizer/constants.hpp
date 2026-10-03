/**
 * @file include/halo/rasterizer/constants.hpp
 * Named constants of the rasterizer: float bit patterns, fixed-function formats and the numeric ids it compares.
 */
#pragma once

#include <cstdint>

#include "halo/render/d3d9.hpp"

namespace halo::rasterizer {

/** Bit pattern of -1.0f; the light tags use it to mark an omni light (no falloff cone). */
inline constexpr uint32_t k_float_bits_minus_one = 0xbf800000u;
/** Bit pattern of 1.0f. */
inline constexpr uint32_t k_float_bits_one = 0x3f800000u;

/** `value` with its low byte replaced (the value a function returns in EAX when it only sets AL). */
constexpr uint32_t replace_low_byte(uint32_t value, uint8_t low) noexcept
{
    return (value & ~0xffu) | low;
}

/**
 * Packs three unit-range channels into an opaque D3DCOLOR the way the model and environment passes build their
 * texture factor: each channel is truncated to an integer and merged into the 0xff.. pattern, so out-of-range values
 * spill into the higher channels exactly as they do in the original code.
 */
inline uint32_t pack_opaque_color(float red, float green, float blue) noexcept
{
    uint32_t color = ~0xffu | static_cast<uint32_t>(static_cast<int32_t>(red * 255.0f));

    color = (color << 8) | (static_cast<uint32_t>(static_cast<int32_t>(green * 255.0f)) & 0xffu);
    color = (color << 8) | (static_cast<uint32_t>(static_cast<int32_t>(blue * 255.0f)) & 0xffu);
    return color;
}

/** Language id (LCID) the shader compiler runs under. */
inline constexpr uint32_t k_locale_english_us = 1033;

/** Values of the force_shader setting with a special meaning. */
inline constexpr uint32_t k_force_shader_fallback = 9997;
inline constexpr uint32_t k_force_shader_ps_2_a = 9998;
inline constexpr uint32_t k_force_shader_disabled = 9999;

/** Size of the loading screen surface and the id of its bitmap resource. */
inline constexpr uint32_t k_loading_screen_width = 640;
inline constexpr uint32_t k_loading_screen_height = 480;
inline constexpr uint32_t k_loading_screen_resource_id = 134;

/** The signed 16 bit halves a packed screen coordinate pair carries (low half first). */
constexpr int16_t low_half(uint32_t packed) noexcept
{
    return static_cast<int16_t>(packed);
}

constexpr int16_t high_half(uint32_t packed) noexcept
{
    return static_cast<int16_t>(packed >> 16);
}

/** Frustum depth range the decal and model passes switch to, as the float bit patterns the depth setup takes. */
inline constexpr float k_decal_frustum_z_near = 1.0f / 256.0f;
inline constexpr float k_decal_frustum_z_far = 4096.0f;

/** The pool a vertex buffer with the given usage is created in: system memory for dynamic or software-processed buffers. */
constexpr uint32_t vertex_buffer_pool_for_usage(uint32_t usage) noexcept
{
    return ((usage & halo::d3d9::k_usage_software_processing) != 0 || (usage & halo::d3d9::k_usage_dynamic) != 0)
               ? halo::d3d9::k_pool_system_memory
               : halo::d3d9::k_pool_managed;
}

/** Usage word of a dynamic vertex buffer: its declaration's usage, dynamic, and software processing when enabled. */
constexpr uint32_t dynamic_vertex_buffer_usage(uint32_t declaration_usage, bool software_processing) noexcept
{
    return (software_processing ? halo::d3d9::k_usage_software_processing : 0u) | declaration_usage | halo::d3d9::k_usage_dynamic;
}

/** Sizes of the vertex and index buffers the dynamic geometry systems create once. */
inline constexpr uint32_t k_dynamic_index_buffer_bytes = 0x30000;
inline constexpr uint32_t k_decal_vertex_buffer_bytes = 0x3c000;
inline constexpr uint32_t k_detail_object_vertex_buffer_bytes = 0x78000;

/** The alpha byte and the colour channels of an ARGB colour word. */
inline constexpr uint32_t k_color_alpha_mask = 0xffu << 24;
inline constexpr uint32_t k_color_rgb_mask = ~k_color_alpha_mask;

/** Frustum depth range the lens flare occlusion pass draws with. */
inline constexpr float k_lens_flare_frustum_z_near = 0.0312519073f;
inline constexpr float k_lens_flare_frustum_z_far = 4096.0f;

/** Bit 15 of a BSP lens flare's visibility word marks it as a marker flare; the rest holds the marker offset. */
inline constexpr uint16_t k_lens_flare_marker_flag = 1u << 15;
inline constexpr uint32_t k_lens_flare_marker_offset_mask = k_lens_flare_marker_flag - 1u;

/** Quads one frame of detail objects may fill the vertex buffer with. */
inline constexpr int32_t k_detail_object_maximum_quads = 0x1000;

/** Bytes of the per-frame scratch memory the model draw code copies lighting and skinning into. */
inline constexpr uint32_t k_scratch_memory_bytes = 0x18000;

/** D3DFMT_INDEX16. */
inline constexpr uint32_t k_format_index16 = 101;

}  // namespace halo::rasterizer
