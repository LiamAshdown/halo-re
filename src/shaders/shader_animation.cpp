/**
 * @file src/shaders/shader_animation.cpp
 * Texture animation evaluation for shader tags.
 * The original author notes and decompiles are in docs/original/shaders/.
 */

#include "halo/shaders/shaders.hpp"

extern "C" {
extern real periodic_function_evaluate(periodic_function_t type, double time);
extern double cos(double x);
extern double sin(double x);
}

namespace halo::shaders {

void shader_texture_animation_evaluate(render_animation *frame_animation, shader_texture_animation *texture_animation, float *u_row, float *v_row, real u_scale, real v_scale, real u_offset, real v_offset, real rotation_degrees, real time)
{
    real u_period, v_period, rotation_period;
    real u_source_value, v_source_value, rotation_source_value;
    real u_wave, v_wave, rotation_wave;
    real du, dv, total_rotation;
    double cos_r, sin_r;

    u_period = (texture_animation->u.period == 0.0f) ? 1.0f : texture_animation->u.period;
    v_period = (texture_animation->v.period == 0.0f) ? 1.0f : texture_animation->v.period;
    rotation_period = (texture_animation->rotation.period == 0.0f) ? 1.0f : texture_animation->rotation.period;

    if (frame_animation == (render_animation *)0) {
        u_source_value = 1.0f;
        v_source_value = 1.0f;
        rotation_source_value = 1.0f;
    } else {
        real *function_values = (real *)(uintptr_t)frame_animation->function_values;

        u_source_value = (texture_animation->u.source == 0) ? 1.0f
                                                             : function_values[texture_animation->u.source - 1];
        v_source_value = (texture_animation->v.source == 0) ? 1.0f
                                                             : function_values[texture_animation->v.source - 1];
        rotation_source_value = (texture_animation->rotation.source == 0)
                                     ? 1.0f
                                     : function_values[texture_animation->rotation.source - 1];
    }

    u_wave = periodic_function_evaluate(texture_animation->u.function,
                                        ((double)time + texture_animation->u.phase) / u_period);
    v_wave = periodic_function_evaluate(texture_animation->v.function,
                                        ((double)time + texture_animation->v.phase) / v_period);
    rotation_wave = periodic_function_evaluate(
        texture_animation->rotation.function,
        ((double)time + texture_animation->rotation.phase) / rotation_period);

    du = (u_offset - texture_animation->rotation_center.x) +
         u_wave * texture_animation->u.scale * u_source_value;
    dv = (v_offset - texture_animation->rotation_center.y) +
         v_wave * texture_animation->v.scale * v_source_value;
    total_rotation = rotation_wave * texture_animation->rotation.scale * rotation_source_value +
                     rotation_degrees;

    if (total_rotation == 0.0f || total_rotation == (real)k_shader_texture_rotation_full_turn) {
        cos_r = 1.0f;
        sin_r = 0.0f;
    } else {
        double radians = (double)total_rotation * (double)0.017453292f;

        cos_r = cos(radians);
        sin_r = sin((double)(real)radians);
    }

    u_row[2] = 0.0f;
    u_row[0] = (float)(cos_r * u_scale);
    u_row[1] = (float)-(v_scale * sin_r);
    u_row[3] = (float)((cos_r * du - sin_r * dv) + texture_animation->rotation_center.x);

    v_row[2] = 0.0f;
    v_row[0] = (float)(u_scale * sin_r);
    v_row[1] = (float)(cos_r * v_scale);
    v_row[3] = (float)((sin_r * du + cos_r * dv) + texture_animation->rotation_center.y);
}

void shader_environment_texture_scrolling_evaluate(float *u_out, float *v_out, double time, ShaderEnvironment *environment)
{
    *u_out = (float)periodic_function_evaluate(
        environment->u_animation_function,
        time / (double)environment->u_animation_period) * environment->u_animation_scale;

    *v_out = (float)periodic_function_evaluate(
        environment->v_animation_function,
        time / (double)environment->v_animation_period) * environment->v_animation_scale;
}

}  // namespace halo::shaders
