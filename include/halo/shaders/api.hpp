/**
 * @file include/halo/shaders/api.hpp
 * Functions of the shaders module that other modules and the data tables call (namespace halo::shaders). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdarg.h>
#include <stdint.h>

typedef float real;
struct Shader;
struct ShaderEnvironment;
struct render_animation;
struct shader_texture_animation;

namespace halo::shaders {

int16_t chimera__shader_get_vertex_shader_permutation(Shader *shader);
uint8_t shader_is_decal(Shader *shader);
uint8_t shader_draw_before_water(Shader *shader);
void shader_texture_animation_evaluate(render_animation *frame_animation, shader_texture_animation *texture_animation, float *u_row, float *v_row, real u_scale, real v_scale, real u_offset, real v_offset, real rotation_degrees, real time);
void shader_environment_texture_scrolling_evaluate(float *u_out, float *v_out, double time, ShaderEnvironment *environment);
int16_t numeric_countdown_timer_get_digit(int16_t digit_index);
void numeric_countdown_timer_update(void);

}
