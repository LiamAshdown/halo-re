/**
 * @file include/halo/cutscene/vars.hpp
 * Addresses of the engine variables the cutscene module owns, by name. Other files bind a typed reference to the entry they use
 * (halo::link::ref<T>); the link names themselves appear only in standalone/data/link/cutscene_vars.hpp.
 */
#pragma once

namespace halo::cutscene {

/** Address table of the engine variables owned by the cutscene module. */
struct Vars {
    void *cinematic_saved_music_gain;
    void *cinematic_screen_effect_state;
    void *rasterizer_model_ambient_reflection_tint;
    void *recorded_animation_angle_scale;
    void *recorded_animation_codecs_by_version;
    void *recorded_animation_compressed_event_handlers;
    void *recorded_animation_v1_event_handlers;
    void *text_shadow_color_argb;
    void *unit_control_data_version_layouts;
};

/** The singleton address table; its storage is constant-initialised. */
const Vars &vars();

}  // namespace halo::cutscene
