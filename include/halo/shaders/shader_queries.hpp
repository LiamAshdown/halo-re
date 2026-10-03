/**
 * @file include/halo/shaders/shader_queries.hpp
 * Questions the rasterizer asks of a shader tag: vertex shader permutation, decal flag, draw-before-water flag.
 */
#pragma once

#include "halo/shaders/shaders_types.hpp"

namespace halo::shaders {

/**
 * Non-owning view of a Shader tag instance. It never copies or owns tag memory; the wrapped pointer may be NULL or -1
 * where the original function accepted those.
 */
class shader_view {
public:
    explicit shader_view(Shader *p) : self(p) {}

    /**
     * Picks the vertex shader permutation index the rasterizer selects for the shader. A pointer value of -1 gives the
     * default permutation; effect, model and transparent generic/chicago shaders refine it from their own fields.
     *
     * @address 0x53fd60
     */
    int16_t vertex_shader_permutation();

    /**
     * Returns whether the shader is a decal: the decal flag of transparent generic/chicago/chicago_extended, glass and
     * meter shaders. NULL and every other shader type return 0.
     *
     * @address 0x53fde0
     */
    uint8_t is_decal();

    /**
     * Returns whether a transparent generic/chicago/chicago_extended shader draws before the water plane. NULL and every
     * other shader type return 0.
     *
     * @address 0x53fe30
     */
    uint8_t draw_before_water();

private:
    Shader *self;
};

}  // namespace halo::shaders
