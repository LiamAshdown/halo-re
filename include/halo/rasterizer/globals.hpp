#pragma once

#include <stdint.h>

/**
 * Link names of the engine globals of the rasterizer that the original image keeps as anonymous data. The
 * definitions live in the standalone data layer under these original names; the module reaches them through
 * the named references of halo::rasterizer::globals below.
 */
extern "C" {
extern uint8_t unknown_006893ef;
extern uint8_t console_debug_toggle_6893e0;
extern uint8_t console_debug_toggle_6893e4;
extern uint8_t console_debug_toggle_6893e6;
extern uint8_t console_debug_toggle_6893ec;
extern uint8_t console_debug_toggle_6893f2;
extern uint8_t console_debug_toggle_6893f4;
extern uint8_t console_debug_toggle_6893fa;
extern uint8_t console_debug_toggle_689400;
extern uint8_t console_debug_toggle_689407;
extern uint8_t console_debug_toggle_689408;
extern uint8_t console_debug_toggle_69c614;
extern uint8_t unknown_006893ff;
extern int16_t unknown_00689450;
extern uint16_t unknown_006893e2;
extern uint8_t unknown_006893f6;
extern uint8_t unknown_006893f7;
extern uint8_t unknown_006893f8;
extern uint8_t unknown_006893fd;
extern uint8_t unknown_0068941d;
extern float unknown_00689418;
extern uint8_t unknown_00689426;
extern float renderer_unknown_68940c;
extern uint32_t renderer_unknown_69c684;
extern int16_t unknown_0069c63e;
extern int16_t unknown_0069c640;
extern int16_t unknown_0069c642;
extern void *unknown_0069da10;
extern uint8_t unknown_006e0a04;
extern int16_t unknown_00719aac;
extern uint32_t unknown_0071d160;
extern uint8_t unknown_0071d18d;
extern uint8_t unknown_0071d1b0;
extern uint16_t unknown_0071d1b4;
extern uint8_t unknown_0071d1c4;
extern uint8_t unknown_0071d1fa;
extern uint8_t unknown_0071d1fb;
extern uint8_t unknown_0071d1fc;
extern uint8_t unknown_0071d1fd;
extern uint8_t unknown_0071d275;
extern uint8_t unknown_0071d276;
extern void *(*unknown_00721ea0)(void *hwnd, void *device, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e);
extern void (*unknown_00721eac)(void *engine);
extern int32_t (*unknown_00721eb4)(void *engine, void *path, void *key, uint32_t flags, void *rect, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f);
extern int32_t (*unknown_00721eb8)(void *engine, void *key);
extern int32_t (*unknown_00721ebc)(void *engine);
extern void (*unknown_00721ec8)(int32_t document);
extern void (*unknown_00721edc)(int32_t document, uint32_t a);
extern int16_t unknown_00746fbc;
extern float unknown_007c047c;
extern void *unknown_007c048c;
extern void *unknown_007c0490;
extern void *unknown_007c0494;
}

namespace halo::rasterizer::globals {

/**
 * Statistics display level (the script global "rasterizer_stats" by position in the debug table). The
 * frame statistics sampler records frame drops while it or the fps display toggle is non-zero.
 *
 * @address 0x6893e2
 */
inline uint16_t &frame_statistics_level = unknown_006893e2;

/**
 * Non-zero while decals and lens flares are rendered; the video options set it every time they apply. The same
 * variable is halo::interface::state::decals_and_lens_flares_enabled.
 *
 * @address 0x6893ff
 */
inline uint8_t &decals_and_lens_flares_enabled = unknown_006893ff;

/**
 * Object level-of-detail quality set by the video options: 0 low, 1 medium, 2 full. The same variable is
 * halo::interface::state::object_lod_quality.
 *
 * @address 0x689450
 */
inline int16_t &object_lod_quality = unknown_00689450;

/**
 * Set by the video options and read by the shader stage switch: when zero every request to change the shader
 * stage configuration is forced to configuration 0. Defaults to 1. Behaviour name; the script global behind it
 * is not confirmed.
 *
 * @address 0x6893ef
 */
inline uint8_t &shader_stage_config_enabled = unknown_006893ef;

/**
 * Specular gate of projected lights. The video options set it together with the other specular gates from the
 * profile's specular setting; the device reset clears it on pixel shader versions below 1.1. Read by the
 * projected light constant, cube map and draw helpers.
 *
 * @address 0x6893f6
 */
inline uint8_t &specular_projected_light_enabled = unknown_006893f6;

/**
 * Specular gate of lightmap and dynamic light draws, set from the same profile setting as the other specular
 * gates and cleared by the device reset on old pixel shader versions. Read by the dynamic light technique, the
 * environment specular lightmap draw, the dynamic geometry draw and the lighting disable workaround.
 *
 * @address 0x6893f7
 */
inline uint8_t &specular_lightmap_enabled = unknown_006893f7;

/**
 * Gate of the multipurpose environment technique (read by its state setup, the lightmap setter and the
 * environment technique draw); cleared by the device reset on old pixel shader versions. Behaviour name.
 *
 * @address 0x6893f8
 */
inline uint8_t &environment_multipurpose_enabled = unknown_006893f8;

/**
 * Cleared by the device reset on old pixel shader versions. Nothing else in the image reads or sets it.
 *
 * @address 0x6893fd
 */
inline uint8_t &device_reset_cleared_flag = unknown_006893fd;

/**
 * Gate of the screen space fog overlay: read when the overlay render states are set, cleared by the device
 * reset on old pixel shader versions. Behaviour name.
 *
 * @address 0x68941d
 */
inline uint8_t &fog_screen_overlay_enabled = unknown_0068941d;

/**
 * Uniform ambient override of model lighting. Zero (the default) uses the lighting's own ambient colour; a
 * positive value zeroes the light block and uses the value as the red, green and blue ambient.
 *
 * @address 0x689418
 */
inline float &model_lighting_ambient_override = unknown_00689418;

/**
 * Lens flare occlusion queries enabled (default 1). Read by the lens flare draw when the hardware has no
 * query support. Behaviour name.
 *
 * @address 0x689426
 */
inline uint8_t &lens_flare_occlusion_enabled = unknown_00689426;

/**
 * Value the three underwater tint jitter components are forced to while the render force flag is 2 (default
 * 1.0).
 *
 * @address 0x68940c
 */
inline float &underwater_tint_jitter_forced_value = renderer_unknown_68940c;

/**
 * Packed ARGB colour of the fixed function ambient light, default 0xff202020. The script function
 * "rasterizer_fixed_function_ambient" sets all three channels to its argument; the blue channel also feeds
 * the ambient boost of the model lighting block and the ambient render state of the underwater tint pass.
 *
 * @address 0x69c684
 */
inline uint32_t &fixed_function_ambient_color = renderer_unknown_69c684;

/**
 * Left edge of the game screen rectangle (the safe area inside the window): 8 after every window resize. The
 * top edge is game_screen_rect.
 *
 * @address 0x69c63e
 */
inline int16_t &game_screen_rect_left = unknown_0069c63e;

/**
 * Bottom edge of the game screen rectangle: window height minus 8 after every window resize.
 *
 * @address 0x69c640
 */
inline int16_t &game_screen_rect_bottom = unknown_0069c640;

/**
 * Right edge of the game screen rectangle: window width minus 8 after every window resize.
 *
 * @address 0x69c642
 */
inline int16_t &game_screen_rect_right = unknown_0069c642;

/**
 * Effect object (ID3DXEffect) the lens flare pass runs through; null when lens flares use the fixed function
 * path.
 *
 * @address 0x69da10
 */
inline void *&lens_flare_effect = unknown_0069da10;

/**
 * Selects the variant of the environment lightmap / self illumination effects and the vertex stream used for
 * them. Nothing in the image writes it, so it stays zero.
 *
 * @address 0x6e0a04
 */
inline uint8_t &environment_effect_variant = unknown_006e0a04;

/**
 * main_globals.screenshot_tile_count: the pending screenshot is taken as n by n tiles, 0 when idle; the movie
 * capture sets it to 1. The frustum of a screenshot is divided by screenshot_scale times this value per axis,
 * and lens flares and decals are skipped for values of 2 and more.
 *
 * @address 0x719aac
 */
inline int16_t &screenshot_tile_count = unknown_00719aac;

/**
 * Cleared at the start of every frame by the dynamic geometry reset. Nothing in the image reads it.
 *
 * @address 0x71d160
 */
inline uint32_t &frame_reset_cleared_word = unknown_0071d160;

/**
 * Non-zero requests a lockable back buffer in the present parameters. Nothing in the image sets it.
 *
 * @address 0x71d18d
 */
inline uint8_t &lockable_back_buffer_requested = unknown_0071d18d;

/**
 * Set once the video mode command line has been parsed; the first parse also forces the mode when no refresh
 * rate was given.
 *
 * @address 0x71d1b0
 */
inline uint8_t &video_mode_command_line_parsed = unknown_0071d1b0;

/**
 * Cleared at the start of every frame and by the device reset. Nothing in the image reads it.
 *
 * @address 0x71d1b4
 */
inline uint16_t &frame_reset_cleared_word_b = unknown_0071d1b4;

/**
 * Set by the filthy decal fog hack once its fog state is applied; cleared when the decal pass begins.
 *
 * @address 0x71d1c4
 */
inline uint8_t &decal_fog_state_applied = unknown_0071d1c4;

/**
 * Non-zero while the sky model is being rendered (it uses its own render states): set by the sky render and
 * cleared by the object, camouflage and king of the hill passes that follow.
 *
 * @address 0x71d1fa
 */
inline uint8_t &sky_pass_active = unknown_0071d1fa;

/**
 * Non-zero when the model or environment being drawn uses the fixed function planar fog vertex shader (0x19):
 * set by the model begin when the fog plane applies to the camera and the model.
 *
 * @address 0x71d1fb
 */
inline uint8_t &planar_fog_vertex_shader_active = unknown_0071d1fb;

/**
 * Cleared at the start of every model draw group. Nothing in the image reads it.
 *
 * @address 0x71d1fc
 */
inline uint8_t &model_begin_cleared_flag = unknown_0071d1fc;

/**
 * Draw mode argument of the current model begin; the model end checks it to decide whether the shader stage
 * must be restored.
 *
 * @address 0x71d1fd
 */
inline uint8_t &model_draw_mode = unknown_0071d1fd;

/**
 * Set at the start of every frame on pixel shader 1.1 hardware and cleared once the water ripple texture has
 * been refreshed by the first water draw.
 *
 * @address 0x71d275
 */
inline uint8_t &water_ripple_update_pending = unknown_0071d275;

/**
 * Set when a transparent geometry group is created and cleared at the start of the frame. Nothing in the image
 * reads it.
 *
 * @address 0x71d276
 */
inline uint8_t &transparent_group_created = unknown_0071d276;

/**
 * Lens flare batching mode: 2 when the flare being added has bit 8 set and its instance flags carry 0x80, else
 * 0. Read when a batch slot is chosen.
 *
 * @address 0x746fbc
 */
inline int16_t &lens_flare_batch_mode = unknown_00746fbc;

/**
 * Attenuation of the planar fog added by the fixed function model and environment fog: 1 minus the maximum
 * planar density scaled by the distance blend, recomputed by the model begin.
 *
 * @address 0x7c047c
 */
inline float &planar_fog_attenuation = unknown_007c047c;

/**
 * Draw routine for self illuminated environment surfaces, picked by the hardware code path selection
 * (single stream, two stream or generic).
 *
 * @address 0x7c048c
 */
inline void *&environment_self_illumination_draw = unknown_007c048c;

/**
 * Draw routine for lightmapped environment surfaces, picked by the hardware code path selection.
 *
 * @address 0x7c0490
 */
inline void *&environment_lightmap_draw = unknown_007c0490;

/**
 * Draw routine for light cones: the light cone draw on capable hardware, else an empty function.
 *
 * @address 0x7c0494
 */
inline void *&light_cone_draw = unknown_007c0494;

/**
 * Keystone library entry "KeystoneCreate": creates the UI engine for a window and device. Null when the
 * library is not loaded.
 *
 * @address 0x721ea0
 */
inline void *(*&keystone_create)(void *hwnd, void *device, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e) = unknown_00721ea0;

/**
 * Keystone library entry "Call_KsRelease": destroys the UI engine.
 *
 * @address 0x721eac
 */
inline void (*&keystone_release)(void *engine) = unknown_00721eac;

/**
 * Keystone library entry "Call_KsCreateWindow": loads a KSML document and returns it under a key.
 *
 * @address 0x721eb4
 */
inline int32_t (*&keystone_create_window)(void *engine, void *path, void *key, uint32_t flags, void *rect, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f) = unknown_00721eb4;

/**
 * Keystone library entry "Call_KsGetWindow": finds a loaded document by key.
 *
 * @address 0x721eb8
 */
inline int32_t (*&keystone_get_window)(void *engine, void *key) = unknown_00721eb8;

/**
 * Keystone library entry "Call_KsUpdate": renders the UI engine; a negative result marks the UI render as
 * failed.
 *
 * @address 0x721ebc
 */
inline int32_t (*&keystone_update)(void *engine) = unknown_00721ebc;

/**
 * Keystone library entry "Call_KW_Release": releases a document reference.
 *
 * @address 0x721ec8
 */
inline void (*&keystone_window_release)(int32_t document) = unknown_00721ec8;

/**
 * Keystone library entry "Call_KW_ShowWindow": shows (non-zero) or hides a document.
 *
 * @address 0x721edc
 */
inline void (*&keystone_window_show)(int32_t document, uint32_t visible) = unknown_00721edc;

/**
 * hs global "rasterizer_fps": show the frame rate statistics; also enables the frame statistics sampler.
 *
 * @address 0x6893e0
 */
inline uint8_t &rasterizer_fps = console_debug_toggle_6893e0;

/**
 * Non-zero selects a debug rendering mode: decals, dynamic geometry, specular and projected light passes are skipped and the clear colour is black when it equals 1 (the script debug table lists it as "rasterizer_mode"). A 16 bit value.
 *
 * @address 0x6893e4
 */
inline uint8_t &rasterizer_debug_mode = console_debug_toggle_6893e4;

/**
 * hs global "rasterizer_wireframe": draw everything as wireframe (fill mode 2) and skip text backdrops.
 *
 * @address 0x6893e6
 */
inline uint8_t &rasterizer_wireframe = console_debug_toggle_6893e6;

/**
 * Gates the model draw path: while set the model begin selects its shader stage configuration and render states, and the sky, object and camouflage passes mark the render states dirty.
 *
 * @address 0x6893ec
 */
inline uint8_t &models_enabled = console_debug_toggle_6893ec;

/**
 * Object shadows: set from the profile shadows setting (zero on shader versions below 1.1 and by the device reset); read by the model shadow passes and shown as Shadows by the time demo.
 *
 * @address 0x6893f2
 */
inline uint8_t &object_shadows_enabled = console_debug_toggle_6893f2;

/**
 * hs global "rasterizer_environment_diffuse_textures": draw environment diffuse textures.
 *
 * @address 0x6893f4
 */
inline uint8_t &rasterizer_environment_diffuse_textures = console_debug_toggle_6893f4;

/**
 * Specular: set from the profile specular setting with the other specular gates and shown as Specular by the time demo; read by the decal pass and the environment specular passes.
 *
 * @address 0x6893fa
 */
inline uint8_t &specular_enabled = console_debug_toggle_6893fa;

/**
 * Gate of rasterizer_transparent_object_append (default 1, read nowhere else).
 *
 * @address 0x689400
 */
inline uint8_t &transparent_object_append_enabled = console_debug_toggle_689400;

/**
 * hs global "rasterizer_fog_atmosphere": atmospheric fog.
 *
 * @address 0x689407
 */
inline uint8_t &rasterizer_fog_atmosphere = console_debug_toggle_689407;

/**
 * hs global "rasterizer_fog_plane": planar fog.
 *
 * @address 0x689408
 */
inline uint8_t &rasterizer_fog_plane = console_debug_toggle_689408;

/**
 * Gate of the object shadow pass of the window render.
 *
 * @address 0x69c614
 */
inline uint8_t &object_shadow_pass_enabled = console_debug_toggle_69c614;

}
