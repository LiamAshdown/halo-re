/**
 * @file include/halo/shaders/shader_animation.hpp
 * Texture animation evaluation for shader tags.
 */
#pragma once

#include "halo/shaders/shaders_types.hpp"

namespace halo::shaders {

/**
 * Evaluates one shader_texture_animation (u/v scroll plus rotation) at the given time and writes the 2x4 texture
 * transform as two rows, u_row and v_row. frame_animation supplies the per-object function values a channel's source
 * field indexes; when NULL every channel source counts as 1.0.
 *
 * The periodic function arguments and the rotation are computed in double precision exactly as the original x87 code
 * did.
 *
 * @address 0x53fe50
 */
void shader_texture_animation_evaluate(render_animation *frame_animation, shader_texture_animation *texture_animation, float *u_row, float *v_row, real u_scale, real v_scale, real u_offset, real v_offset, real rotation_degrees, real time);

/**
 * Evaluates the u/v base-map scroll pair of a ShaderEnvironment at the given time: each axis runs its own periodic
 * function over time / period, scaled by that axis's amplitude.
 *
 * @address 0x540060
 */
void shader_environment_texture_scrolling_evaluate(float *u_out, float *v_out, double time, ShaderEnvironment *environment);

}  // namespace halo::shaders
