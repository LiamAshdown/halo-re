/**
 * @file include/halo/shaders/api.hpp
 * Functions of the shaders module that other modules and the data tables call (namespace halo::shaders). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdarg.h>
#include <stdint.h>



typedef float real;
struct game_time_globals;
struct Shader;
struct ShaderEnvironment;
struct render_animation;
struct shader_texture_animation;

namespace halo::shaders {

/**
 * The engine globals the shaders module owns (their storage is defined by standalone/data under the original link names);
 * other modules reach them through globals().
 */
struct Globals {
    int32_t &numeric_countdown_timer_remaining_ms;
    uint8_t &numeric_countdown_timer_running;
    game_time_globals *&game_time;
    int32_t &numeric_countdown_timer_last_update_ms;
};

/**
 * The shaders service singleton. instance() builds the Globals reference table on first use (Meyers singleton); the state it
 * refers to lives in the data image. globals() is the short form every caller uses.
 */
class Service {
public:
    static Globals &instance();
};

inline Globals &globals() { return Service::instance(); }

int16_t chimera__shader_get_vertex_shader_permutation(Shader *shader);
uint8_t shader_is_decal(Shader *shader);
uint8_t shader_draw_before_water(Shader *shader);
void shader_texture_animation_evaluate(render_animation *frame_animation, shader_texture_animation *texture_animation, float *u_row, float *v_row, real u_scale, real v_scale, real u_offset, real v_offset, real rotation_degrees, real time);
void shader_environment_texture_scrolling_evaluate(float *u_out, float *v_out, double time, ShaderEnvironment *environment);
int16_t numeric_countdown_timer_get_digit(int16_t digit_index);
void numeric_countdown_timer_update(void);

}
