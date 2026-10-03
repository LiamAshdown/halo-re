/**
 * @file src/shaders/shaders_c_api.cpp
 * The shaders module's C ABI: one extern "C" function per original symbol, same name and signature,
 * forwarding to the halo::shaders implementation. The shims do nothing else.
 */

#include "halo/shaders/shaders_c_api.h"

extern "C" int16_t chimera__shader_get_vertex_shader_permutation(Shader *shader)
{
    return halo::shaders::shader_view(shader).vertex_shader_permutation();
}

extern "C" uint8_t shader_is_decal(Shader *shader)
{
    return halo::shaders::shader_view(shader).is_decal();
}

extern "C" uint8_t shader_draw_before_water(Shader *shader)
{
    return halo::shaders::shader_view(shader).draw_before_water();
}

extern "C" void shader_texture_animation_evaluate(render_animation *frame_animation, shader_texture_animation *texture_animation, float *u_row, float *v_row, real u_scale, real v_scale, real u_offset, real v_offset, real rotation_degrees, real time)
{
    halo::shaders::shader_texture_animation_evaluate(frame_animation, texture_animation, u_row, v_row, u_scale, v_scale, u_offset, v_offset, rotation_degrees, time);
}

extern "C" void shader_environment_texture_scrolling_evaluate(float *u_out, float *v_out, double time, ShaderEnvironment *environment)
{
    halo::shaders::shader_environment_texture_scrolling_evaluate(u_out, v_out, time, environment);
}

extern "C" int16_t numeric_countdown_timer_get_digit(int16_t digit_index)
{
    return halo::shaders::numeric_countdown_timer::get_digit(digit_index);
}

extern "C" void numeric_countdown_timer_update(void)
{
    halo::shaders::numeric_countdown_timer::update();
}
