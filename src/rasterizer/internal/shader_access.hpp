/**
 * @file src/rasterizer/internal/shader_access.hpp
 * Typed views the rasterizer uses on shader tag data and on the per-group animation source.
 */
#pragma once

#include <cstddef>
#include <cstdint>

#include "halo/core/datum.hpp"
#include "halo/core/flag_bits.hpp"
#include "halo/shaders/shaders_types.hpp"
#include "halo/tags/flags.hpp"

namespace halo::rasterizer {

using halo::shaders::shader_cast;

/**
 * The function value table of a group's animation source (render_animation.function_values), or NULL when the group has
 * no animation source or the source carries no table.
 */
inline const float *animation_function_values(const render_animation *animation)
{
    if (animation == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<const float *>(static_cast<uintptr_t>(animation->function_values));
}

/** The change color table of a group's animation source (render_animation.change_colors). */
inline const ColorRGB *animation_change_colors(const render_animation *animation)
{
    return reinterpret_cast<const ColorRGB *>(static_cast<uintptr_t>(animation->change_colors));
}

/** True when the tag reference has a target (its tag id is not the none value). */
inline bool has_tag(const TagDependency &dependency)
{
    return halo::tag_id_bits(dependency.tag_id) != halo::k_dword_none;
}

}  // namespace halo::rasterizer

// Offsets the rasterizer reached with byte arithmetic before these layouts were used; each field must stay where the
// retail code read it.
static_assert(offsetof(shader_effect, flags) == 0x28);
static_assert(offsetof(shader_effect, framebuffer_blend_function) == 0x2a);
static_assert(offsetof(shader_effect, map_flags) == 0x2e);
static_assert(offsetof(shader_effect, secondary_map) + offsetof(TagDependency, tag_id) == 0x58);
static_assert(offsetof(shader_effect, anchor) == 0x5c);
static_assert(offsetof(shader_effect, texture_animation) == 0x60);

static_assert(offsetof(ShaderModel, shader_model_flags) == 0x28);
static_assert(offsetof(ShaderModel, shader_model_more_flags) == 0x6c);
static_assert(offsetof(ShaderModel, u_animation_source) == 0xfc);
static_assert(offsetof(ShaderTransparentChicago, shader_transparent_chicago_flags) == 0x29);
static_assert(offsetof(ShaderTransparentWater, water_flags) == 0x28);

static_assert(offsetof(ShaderTransparentGlass, shader_transparent_glass_flags) == 0x28);
static_assert(offsetof(ShaderTransparentGlass, background_tint_color) == 0x54);
static_assert(offsetof(ShaderTransparentGlass, background_tint_map) + offsetof(TagDependency, tag_id) == 0x70);
static_assert(offsetof(ShaderTransparentGlass, reflection_type) == 0x8a);
static_assert(offsetof(ShaderTransparentGlass, perpendicular_brightness) == 0x8c);
static_assert(offsetof(ShaderTransparentGlass, parallel_brightness) == 0x9c);
static_assert(offsetof(ShaderTransparentGlass, reflection_map) + offsetof(TagDependency, tag_id) == 0xb8);
static_assert(offsetof(ShaderTransparentGlass, bump_map) + offsetof(TagDependency, tag_id) == 0xcc);

static_assert(offsetof(ShaderTransparentMeter, meter_flags) == 0x28);
static_assert(offsetof(ShaderTransparentMeter, map) + offsetof(TagDependency, tag_id) == 0x58);
static_assert(offsetof(ShaderTransparentMeter, gradient_min_color) == 0x7c);
static_assert(offsetof(ShaderTransparentMeter, gradient_max_color) == 0x88);
static_assert(offsetof(ShaderTransparentMeter, background_color) == 0x94);
static_assert(offsetof(ShaderTransparentMeter, flash_color) == 0xa0);
static_assert(offsetof(ShaderTransparentMeter, meter_tint_color) == 0xac);
static_assert(offsetof(ShaderTransparentMeter, meter_transparency) == 0xb8);
static_assert(offsetof(ShaderTransparentMeter, background_transparency) == 0xbc);
static_assert(offsetof(ShaderTransparentMeter, meter_brightness_source) == 0xd8);
static_assert(offsetof(ShaderTransparentMeter, gradient_source) == 0xde);

static_assert(sizeof(BitmapData) == 0x30);
static_assert(sizeof(ScenarioStructureBSPCluster) == 0x68);
static_assert(offsetof(ScenarioStructureBSPCluster, first_lens_flare_marker_index) == 0x40);
static_assert(sizeof(ScenarioStructureBSPLensFlareMarker) == 0x10);
static_assert(sizeof(ScenarioStructureBSPLensFlare) == 0x10);
static_assert(sizeof(ScenarioDetailObjectCollectionPalette) == 0x30);
static_assert(offsetof(ScenarioStructureBSPDetailObjectData, instances) + offsetof(TagReflexive, pointer) == 0x10);
static_assert(offsetof(GlobalsRasterizerData, default_2d) + offsetof(TagDependency, tag_id) == 0xb8);
static_assert(sizeof(TagDependency) == 0x10);

static_assert(offsetof(Light, flags) == 0);
static_assert(offsetof(Light, cos_falloff_angle) == 0x1c);
static_assert(offsetof(Light, cos_cutoff_angle) == 0x20);
static_assert(offsetof(Light, primary_cube_map) + offsetof(TagDependency, tag_id) == 0x70);
static_assert(offsetof(Light, yaw_function) == 0x8e);
static_assert(offsetof(Light, yaw_period) == 0x90);
static_assert(offsetof(Light, roll_function) == 0x96);
static_assert(offsetof(Light, roll_period) == 0x98);
static_assert(offsetof(Light, pitch_function) == 0x9e);
static_assert(offsetof(Light, pitch_period) == 0xa0);
