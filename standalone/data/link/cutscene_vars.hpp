/**
 * @file standalone/data/link/cutscene_vars.hpp
 * Link names of the engine variables owned by the cutscene module (halo::cutscene::vars()). The data image defines them under these C
 * names; only src/cutscene/vars.cpp includes this header, so the names are declared as untyped storage.
 */
#pragma once

extern "C" {
extern char cinematic_saved_music_gain[];
extern char cinematic_screen_effect_state[];
extern char rasterizer_model_ambient_reflection_tint[];
extern char recorded_animation_angle_scale[];
extern char recorded_animation_codecs_by_version[];
extern char recorded_animation_compressed_event_handlers[];
extern char recorded_animation_v1_event_handlers[];
extern char text_shadow_color_argb[];
extern char unit_control_data_version_layouts[];
}
