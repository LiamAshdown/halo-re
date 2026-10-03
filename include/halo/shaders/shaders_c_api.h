/**
 * @file include/halo/shaders/shaders_c_api.h
 * The C ABI of the shaders module: every original function with its original signature and C linkage.
 * Defined in src/shaders/shaders_c_api.cpp; documented on the halo::shaders C++ API.
 */
#pragma once

#include "halo/shaders/shaders.hpp"

#ifdef __cplusplus
extern "C" {
#endif

int16_t chimera__shader_get_vertex_shader_permutation(Shader *shader);
uint8_t shader_is_decal(Shader *shader);
uint8_t shader_draw_before_water(Shader *shader);
void shader_texture_animation_evaluate(render_animation *frame_animation, shader_texture_animation *texture_animation, float *u_row, float *v_row, real u_scale, real v_scale, real u_offset, real v_offset, real rotation_degrees, real time);
void shader_environment_texture_scrolling_evaluate(float *u_out, float *v_out, double time, ShaderEnvironment *environment);
int16_t numeric_countdown_timer_get_digit(int16_t digit_index);
void numeric_countdown_timer_update(void);

#ifdef __cplusplus
}
#endif
