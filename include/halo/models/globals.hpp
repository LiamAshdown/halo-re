/**
 * @file include/halo/models/globals.hpp
 * The models module's engine globals as one service object. The variables live at fixed addresses in the data
 * image (standalone/data) under their original link names; Globals holds a reference to each, so no other file
 * declares them.
 */
#pragma once

#include <stdint.h>

namespace halo::models {

struct Globals {
    uint8_t &animation_compressed_data_enabled;
    real_quaternion *&global_identity_quaternion_pointer;
    uint8_t &model_render_first_person;
    uint8_t (&model_render_default_region_permutations)[8];
    render_model_effect &model_render_default_effect;
    ColorRGB (&model_render_default_change_colors)[4];
    float (&model_render_default_function_values)[4];
    int16_t &console_model_lod_override;
    uint8_t &rasterizer_caps_flag_689;
    uint8_t &console_debug_toggle_6893f2;
    rasterizer_model_draw_context *&rasterizer_object_shadow_model_context;
    uint8_t &rasterizer_object_shadow_model_active;
};

/**
 * The models service singleton. instance() builds the Globals reference table on first use (Meyers singleton); the state it
 * refers to lives in the data image. globals() is the short form every caller uses.
 */
class Service {
public:
    static Globals &instance();
};

inline Globals &globals() { return Service::instance(); }

}  // namespace halo::models
