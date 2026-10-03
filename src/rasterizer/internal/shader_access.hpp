/**
 * @file src/rasterizer/internal/shader_access.hpp
 * Typed views the rasterizer uses on shader tag data and on the per-group animation source.
 */
#pragma once

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
