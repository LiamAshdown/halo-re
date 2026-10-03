/**
 * @file include/halo/render/shader_types.hpp
 * Shader tag type ids and the grouping the draw code switches on.
 */
#pragma once

#include <cstdint>

namespace halo::render {

/** Shader.shader_type values. */
enum class shader_type_id : int16_t {
    screen = 0,
    effect = 1,
    decal = 2,
    environment = 3,
    model = 4,
    transparent_generic = 5,
    transparent_chicago = 6,
    transparent_chicago_extended = 7,
    transparent_water = 8,
    transparent_glass = 9,
    transparent_meter = 10,
    transparent_plasma = 11,
};

/** True when `shader_type` draws through the transparent geometry pass (effect and the transparent_* shaders). */
constexpr bool shader_type_is_transparent(int32_t shader_type) noexcept {
    return shader_type == static_cast<int32_t>(shader_type_id::effect) ||
           (shader_type > static_cast<int32_t>(shader_type_id::model) &&
            shader_type < static_cast<int32_t>(shader_type_id::transparent_plasma) + 1);
}

}  // namespace halo::render
