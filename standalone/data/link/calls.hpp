/**
 * @file standalone/data/link/calls.hpp
 * Link names of engine functions that live in modules which have no C++ API header yet (game, interface, networking, the
 * debug overlay). Each declaration goes when its owner publishes a halo::<module> declaration; the services modules include
 * this header instead of declaring them locally.
 */
#pragma once

#include <stdint.h>

struct real_vector3d;
struct ColorARGB;

extern "C" {
void interface_handle_quit_request(void);
char *network_log_path_resolve(char *requested_path);
void debug_fp_render_model_note(uint32_t model_tag, float pixels, int32_t lod, const float *node0, const float *center, int32_t early_out);
void debug_fp_clip_note(const float *world, int32_t effect_type);
void debug_fp_state_arm(int32_t armed);
void value_step_toward_target(float *value, float target, float max_step);
void console_printf_verbose(ColorARGB *color, char *format, ...);
uint32_t game_variant_option_default_by_index(uint32_t selector);
void vector3d_clamp_length(real_vector3d *v, float max_length);
}
