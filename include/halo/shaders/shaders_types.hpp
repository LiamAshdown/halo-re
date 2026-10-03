/**
 * @file include/halo/shaders/shaders_types.hpp
 * The engine C type headers the shaders module's code depends on, included once and in the order the original sources used.
 */
#pragma once

#include "tags.h"
#include "shaders.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "camera.h"
#include "rasterizer.h"
#include "render.h"
#include <stdint.h>

namespace halo::shaders {

/**
 * Views a shader tag as one of its derived layouts. Every derived shader structure (ShaderEnvironment, ShaderModel,
 * ShaderTransparentGlass, ...) starts with the `Shader` base block, so the base pointer is also the derived pointer.
 */
template <typename Derived>
inline Derived *shader_cast(const Shader *base)
{
    return reinterpret_cast<Derived *>(const_cast<Shader *>(base));
}

}  // namespace halo::shaders
