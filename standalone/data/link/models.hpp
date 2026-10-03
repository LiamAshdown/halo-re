/**
 * @file standalone/data/link/models.hpp
 * Link names of the engine variables the models module binds in halo::models::Globals (src/models/globals.cpp). The variables are
 * defined in standalone/data under these C names; this header is included by that one file only.
 */
#pragma once

extern "C" {
extern uint8_t animation_compressed_data_enabled;
extern real_quaternion *global_identity_quaternion_pointer;
extern uint8_t model_render_first_person;
extern uint8_t model_render_default_region_permutations[8];
extern render_model_effect model_render_default_effect;
extern ColorRGB model_render_default_change_colors[4];
extern float model_render_default_function_values[4];
extern int16_t console_model_lod_override;
extern uint8_t rasterizer_caps_flag_689;
extern rasterizer_model_draw_context *rasterizer_object_shadow_model_context;
extern uint8_t rasterizer_object_shadow_model_active;
}
