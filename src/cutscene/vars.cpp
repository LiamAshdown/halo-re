/**
 * @file src/cutscene/vars.cpp
 * Binds halo::cutscene::Vars to the engine variables the data image defines under their original link names.
 */

#include "halo/cutscene/vars.hpp"
#include "link/cutscene_vars.hpp"
#include "halo/cutscene/api.hpp"

namespace halo::cutscene {

const Vars &vars()
{
    static const Vars table{
        cinematic_saved_music_gain,
        cinematic_screen_effect_state,
        rasterizer_model_ambient_reflection_tint,
        recorded_animation_angle_scale,
        recorded_animation_codecs_by_version,
        recorded_animation_compressed_event_handlers,
        recorded_animation_v1_event_handlers,
        text_shadow_color_argb,
        unit_control_data_version_layouts,
    };
    return table;
}

}  // namespace halo::cutscene
