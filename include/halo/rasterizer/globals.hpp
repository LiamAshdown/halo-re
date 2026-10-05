#pragma once

#include <stdint.h>
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/rasterizer/vars.hpp"
#include "halo/render/vars.hpp"
#include "halo/rasterizer/draw_procedures.hpp"
#include "halo/core/shared_links.hpp"

/**
 * Link names of the engine globals of the rasterizer that the original image keeps as anonymous data. The
 * definitions live in the standalone data layer under these original names; the module reaches them through
 * the named references of halo::rasterizer::globals below.
 */
static auto &shader_stage_config_enabled = halo::link::ref<uint8_t>(halo::rasterizer::vars().shader_stage_config_enabled);
static auto &console_debug_toggle_689409 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689409);
static auto &console_debug_toggle_689403 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689403);
static auto &console_debug_toggle_689421 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689421);
static auto &console_debug_toggle_68941e = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_68941e);
static auto &console_debug_toggle_68941c = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_68941c);
static auto &console_debug_toggle_6893f3 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_6893f3);
static auto &console_debug_toggle_6893e0 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_6893e0);
static auto &console_debug_toggle_6893e4 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_6893e4);
static auto &console_debug_toggle_6893e6 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_6893e6);
static auto &console_debug_toggle_6893ec = halo::link::ref<uint8_t>(halo::render::vars().console_debug_toggle_6893ec);
static auto &console_debug_toggle_6893f2 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_6893f2);
static auto &console_debug_toggle_6893f4 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_6893f4);
static auto &console_debug_toggle_6893fa = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_6893fa);
static auto &console_debug_toggle_689400 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689400);
static auto &console_debug_toggle_689407 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689407);
static auto &console_debug_toggle_689408 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_689408);
static auto &console_debug_toggle_69c614 = halo::link::ref<uint8_t>(halo::rasterizer::vars().console_debug_toggle_69c614);
static auto &frame_statistics_level = halo::link::ref<uint16_t>(halo::rasterizer::vars().frame_statistics_level);
static auto &specular_projected_light_enabled = halo::link::ref<uint8_t>(halo::rasterizer::vars().specular_projected_light_enabled);
static auto &specular_lightmap_enabled = halo::link::ref<uint8_t>(halo::rasterizer::vars().specular_lightmap_enabled);
static auto &environment_multipurpose_enabled = halo::link::ref<uint8_t>(halo::rasterizer::vars().environment_multipurpose_enabled);
static auto &device_reset_cleared_flag = halo::link::ref<uint8_t>(halo::rasterizer::vars().device_reset_cleared_flag);
static auto &fog_screen_overlay_enabled = halo::link::ref<uint8_t>(halo::rasterizer::vars().fog_screen_overlay_enabled);
static auto &model_lighting_ambient_override = halo::link::ref<float>(halo::rasterizer::vars().model_lighting_ambient_override);
static auto &lens_flare_occlusion_enabled = halo::link::ref<uint8_t>(halo::rasterizer::vars().lens_flare_occlusion_enabled);
static auto &underwater_tint_jitter_forced_value = halo::link::ref<float>(halo::rasterizer::vars().underwater_tint_jitter_forced_value);
static auto &fixed_function_ambient_color = halo::link::ref<uint32_t>(halo::rasterizer::vars().fixed_function_ambient_color);
static auto &game_screen_rect_left = halo::link::ref<int16_t>(halo::rasterizer::vars().game_screen_rect_left);
static auto &game_screen_rect_bottom = halo::link::ref<int16_t>(halo::rasterizer::vars().game_screen_rect_bottom);
static auto &game_screen_rect_right = halo::link::ref<int16_t>(halo::rasterizer::vars().game_screen_rect_right);
static auto &lens_flare_effect = halo::link::ref<void *>(halo::rasterizer::vars().lens_flare_effect);
static auto &environment_effect_variant = halo::link::ref<uint8_t>(halo::rasterizer::vars().environment_effect_variant);
static auto &screenshot_tile_count = halo::link::ref<int16_t>(halo::rasterizer::vars().screenshot_tile_count);
static auto &frame_reset_cleared_word = halo::link::ref<uint32_t>(halo::rasterizer::vars().frame_reset_cleared_word);
static auto &lockable_back_buffer_requested = halo::link::ref<uint8_t>(halo::rasterizer::vars().lockable_back_buffer_requested);
static auto &video_mode_command_line_parsed = halo::link::ref<uint8_t>(halo::rasterizer::vars().video_mode_command_line_parsed);
static auto &frame_reset_cleared_word_b = halo::link::ref<uint16_t>(halo::rasterizer::vars().frame_reset_cleared_word_b);
static auto &decal_fog_state_applied = halo::link::ref<uint8_t>(halo::rasterizer::vars().decal_fog_state_applied);
static auto &sky_pass_active = halo::link::ref<uint8_t>(halo::rasterizer::vars().sky_pass_active);
static auto &planar_fog_vertex_shader_active = halo::link::ref<uint8_t>(halo::rasterizer::vars().planar_fog_vertex_shader_active);
static auto &model_begin_cleared_flag = halo::link::ref<uint8_t>(halo::rasterizer::vars().model_begin_cleared_flag);
static auto &model_draw_mode = halo::link::ref<uint8_t>(halo::rasterizer::vars().model_draw_mode);
static auto &unknown_00721ea0 = halo::link::ref<void *(*)(void *hwnd, void *device, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e)>(halo::rasterizer::vars().unknown_00721ea0);
static auto &unknown_00721eb4 = halo::link::ref<int32_t (*)(void *engine, void *path, void *key, uint32_t flags, void *rect, uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f)>(halo::rasterizer::vars().unknown_00721eb4);
static auto &unknown_00721eb8 = halo::link::ref<int32_t (*)(void *engine, void *key)>(halo::rasterizer::vars().unknown_00721eb8);
static auto &unknown_00721ebc = halo::link::ref<int32_t (*)(void *engine)>(halo::rasterizer::vars().unknown_00721ebc);
static auto &unknown_00721ec8 = halo::link::ref<void (*)(int32_t document)>(halo::rasterizer::vars().unknown_00721ec8);
static auto &unknown_00721edc = halo::link::ref<void (*)(int32_t document, uint32_t a)>(halo::rasterizer::vars().unknown_00721edc);
static auto &lens_flare_batch_mode = halo::link::ref<int16_t>(halo::rasterizer::vars().lens_flare_batch_mode);
static auto &planar_fog_attenuation = halo::link::ref<float>(halo::rasterizer::vars().planar_fog_attenuation);
static auto &unknown_007c048c = halo::link::ref<void *>(halo::rasterizer::vars().unknown_007c048c);
static auto &unknown_007c0490 = halo::link::ref<void *>(halo::rasterizer::vars().unknown_007c0490);
static auto &unknown_007c0494 = halo::link::ref<void *>(halo::rasterizer::vars().unknown_007c0494);

namespace halo::rasterizer::fields {

/**
 * Statistics display level (the script global "rasterizer_stats" by position in the debug table). The
 * frame statistics sampler records frame drops while it or the fps display toggle is non-zero.
 *
 * @address 0x6893e2
 */
static uint16_t &frame_statistics_level = ::frame_statistics_level;

/**
 * Non-zero while decals and lens flares are rendered; the video options set it every time they apply. The same
 * variable is halo::interface::state::decals_and_lens_flares_enabled.
 *
 * @address 0x6893ff
 */
static uint8_t &decals_and_lens_flares_enabled = ::decals_and_lens_flares_enabled;

/**
 * Object level-of-detail quality set by the video options: 0 low, 1 medium, 2 full. The same variable is
 * halo::interface::state::object_lod_quality.
 *
 * @address 0x689450
 */
static int16_t &object_lod_quality = ::object_lod_quality;

/**
 * Set by the video options and read by the shader stage switch: when zero every request to change the shader
 * stage configuration is forced to configuration 0. Defaults to 1. Behaviour name; the script global behind it
 * is not confirmed.
 *
 * @address 0x6893ef
 */
static uint8_t &shader_stage_config_enabled = ::shader_stage_config_enabled;

/**
 * Specular gate of projected lights. The video options set it together with the other specular gates from the
 * profile's specular setting; the device reset clears it on pixel shader versions below 1.1. Read by the
 * projected light constant, cube map and draw helpers.
 *
 * @address 0x6893f6
 */
static uint8_t &specular_projected_light_enabled = ::specular_projected_light_enabled;

/**
 * Specular gate of lightmap and dynamic light draws, set from the same profile setting as the other specular
 * gates and cleared by the device reset on old pixel shader versions. Read by the dynamic light technique, the
 * environment specular lightmap draw, the dynamic geometry draw and the lighting disable workaround.
 *
 * @address 0x6893f7
 */
static uint8_t &specular_lightmap_enabled = ::specular_lightmap_enabled;

/**
 * Gate of the multipurpose environment technique (read by its state setup, the lightmap setter and the
 * environment technique draw); cleared by the device reset on old pixel shader versions. Behaviour name.
 *
 * @address 0x6893f8
 */
static uint8_t &environment_multipurpose_enabled = ::environment_multipurpose_enabled;

/**
 * Cleared by the device reset on old pixel shader versions. Nothing else in the image reads or sets it.
 *
 * @address 0x6893fd
 */
static uint8_t &device_reset_cleared_flag = ::device_reset_cleared_flag;

/**
 * Gate of the screen space fog overlay: read when the overlay render states are set, cleared by the device
 * reset on old pixel shader versions. Behaviour name.
 *
 * @address 0x68941d
 */
static uint8_t &fog_screen_overlay_enabled = ::fog_screen_overlay_enabled;

/**
 * Uniform ambient override of model lighting. Zero (the default) uses the lighting's own ambient colour; a
 * positive value zeroes the light block and uses the value as the red, green and blue ambient.
 *
 * @address 0x689418
 */
static float &model_lighting_ambient_override = ::model_lighting_ambient_override;

/**
 * Lens flare occlusion queries enabled (default 1). Read by the lens flare draw when the hardware has no
 * query support. Behaviour name.
 *
 * @address 0x689426
 */
static uint8_t &lens_flare_occlusion_enabled = ::lens_flare_occlusion_enabled;

/**
 * Value the three underwater tint jitter components are forced to while the render force flag is 2 (default
 * 1.0).
 *
 * @address 0x68940c
 */
static float &underwater_tint_jitter_forced_value = ::underwater_tint_jitter_forced_value;

/**
 * Packed ARGB colour of the fixed function ambient light, default 0xff202020. The script function
 * "rasterizer_fixed_function_ambient" sets all three channels to its argument; the blue channel also feeds
 * the ambient boost of the model lighting block and the ambient render state of the underwater tint pass.
 *
 * @address 0x69c684
 */
static uint32_t &fixed_function_ambient_color = ::fixed_function_ambient_color;

/**
 * Left edge of the game screen rectangle (the safe area inside the window): 8 after every window resize. The
 * top edge is game_screen_rect.
 *
 * @address 0x69c63e
 */
static int16_t &game_screen_rect_left = ::game_screen_rect_left;

/**
 * Bottom edge of the game screen rectangle: window height minus 8 after every window resize.
 *
 * @address 0x69c640
 */
static int16_t &game_screen_rect_bottom = ::game_screen_rect_bottom;

/**
 * Right edge of the game screen rectangle: window width minus 8 after every window resize.
 *
 * @address 0x69c642
 */
static int16_t &game_screen_rect_right = ::game_screen_rect_right;

/**
 * Effect object (ID3DXEffect) the lens flare pass runs through; null when lens flares use the fixed function
 * path.
 *
 * @address 0x69da10
 */
static void *&lens_flare_effect = ::lens_flare_effect;

/**
 * Selects the variant of the environment lightmap / self illumination effects and the vertex stream used for
 * them. Nothing in the image writes it, so it stays zero.
 *
 * @address 0x6e0a04
 */
static uint8_t &environment_effect_variant = ::environment_effect_variant;

/**
 * main_globals.screenshot_tile_count: the pending screenshot is taken as n by n tiles, 0 when idle; the movie
 * capture sets it to 1. The frustum of a screenshot is divided by screenshot_scale times this value per axis,
 * and lens flares and decals are skipped for values of 2 and more.
 *
 * @address 0x719aac
 */
static int16_t &screenshot_tile_count = ::screenshot_tile_count;

/**
 * Cleared at the start of every frame by the dynamic geometry reset. Nothing in the image reads it.
 *
 * @address 0x71d160
 */
static uint32_t &frame_reset_cleared_word = ::frame_reset_cleared_word;

/**
 * Non-zero requests a lockable back buffer in the present parameters. Nothing in the image sets it.
 *
 * @address 0x71d18d
 */
static uint8_t &lockable_back_buffer_requested = ::lockable_back_buffer_requested;

/**
 * Set once the video mode command line has been parsed; the first parse also forces the mode when no refresh
 * rate was given.
 *
 * @address 0x71d1b0
 */
static uint8_t &video_mode_command_line_parsed = ::video_mode_command_line_parsed;

/**
 * Cleared at the start of every frame and by the device reset. Nothing in the image reads it.
 *
 * @address 0x71d1b4
 */
static uint16_t &frame_reset_cleared_word_b = ::frame_reset_cleared_word_b;

/**
 * Set by the filthy decal fog hack once its fog state is applied; cleared when the decal pass begins.
 *
 * @address 0x71d1c4
 */
static uint8_t &decal_fog_state_applied = ::decal_fog_state_applied;

/**
 * Non-zero while the sky model is being rendered (it uses its own render states): set by the sky render and
 * cleared by the object, camouflage and king of the hill passes that follow.
 *
 * @address 0x71d1fa
 */
static uint8_t &sky_pass_active = ::sky_pass_active;

/**
 * Non-zero when the model or environment being drawn uses the fixed function planar fog vertex shader (0x19):
 * set by the model begin when the fog plane applies to the camera and the model.
 *
 * @address 0x71d1fb
 */
static uint8_t &planar_fog_vertex_shader_active = ::planar_fog_vertex_shader_active;

/**
 * Cleared at the start of every model draw group. Nothing in the image reads it.
 *
 * @address 0x71d1fc
 */
static uint8_t &model_begin_cleared_flag = ::model_begin_cleared_flag;

/**
 * Draw mode argument of the current model begin; the model end checks it to decide whether the shader stage
 * must be restored.
 *
 * @address 0x71d1fd
 */
static uint8_t &model_draw_mode = ::model_draw_mode;

/**
 * Set at the start of every frame on pixel shader 1.1 hardware and cleared once the water ripple texture has
 * been refreshed by the first water draw.
 *
 * @address 0x71d275
 */
static uint8_t &water_ripple_update_pending = ::water_ripple_update_pending;

/**
 * Set when a transparent geometry group is created and cleared at the start of the frame. Nothing in the image
 * reads it.
 *
 * @address 0x71d276
 */
static uint8_t &transparent_group_created = ::transparent_group_created;

/**
 * Lens flare batching mode: 2 when the flare being added has bit 8 set and its instance flags carry 0x80, else
 * 0. Read when a batch slot is chosen.
 *
 * @address 0x746fbc
 */
static int16_t &lens_flare_batch_mode = ::lens_flare_batch_mode;

/**
 * Attenuation of the planar fog added by the fixed function model and environment fog: 1 minus the maximum
 * planar density scaled by the distance blend, recomputed by the model begin.
 *
 * @address 0x7c047c
 */
static float &planar_fog_attenuation = ::planar_fog_attenuation;

/**
 * Draw routine for self illuminated environment surfaces, picked by the hardware code path selection
 * (single stream, two stream or generic).
 *
 * @address 0x7c048c
 */
inline halo::rasterizer::surface_draw_procedure &environment_self_illumination_draw = halo::link::ref<halo::rasterizer::surface_draw_procedure>(halo::rasterizer::vars().unknown_007c048c);

/**
 * Draw routine for lightmapped environment surfaces, picked by the hardware code path selection.
 *
 * @address 0x7c0490
 */
inline halo::rasterizer::surface_draw_procedure &environment_lightmap_draw = halo::link::ref<halo::rasterizer::surface_draw_procedure>(halo::rasterizer::vars().unknown_007c0490);

/**
 * Draw routine for light cones: the light cone draw on capable hardware, else an empty function.
 *
 * @address 0x7c0494
 */
inline halo::rasterizer::surface_draw_procedure &light_cone_draw = halo::link::ref<halo::rasterizer::surface_draw_procedure>(halo::rasterizer::vars().unknown_007c0494);

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
static uint8_t &rasterizer_fps = console_debug_toggle_6893e0;

/**
 * Non-zero selects a debug rendering mode: decals, dynamic geometry, specular and projected light passes are skipped and the clear colour is black when it equals 1 (the script debug table lists it as "rasterizer_mode"). A 16 bit value.
 *
 * @address 0x6893e4
 */
static uint8_t &rasterizer_debug_mode = console_debug_toggle_6893e4;

/**
 * The same toggle read as the 16 bit value the retail tests compare: the debug mode byte and the byte after it.
 *
 * @address 0x6893e4
 */
inline uint16_t &rasterizer_debug_mode_word = halo::link::ref<uint16_t>(halo::rasterizer::vars().console_debug_toggle_6893e4);

/**
 * hs global "rasterizer_wireframe": draw everything as wireframe (fill mode 2) and skip text backdrops.
 *
 * @address 0x6893e6
 */
static uint8_t &rasterizer_wireframe = console_debug_toggle_6893e6;

/**
 * Gates the model draw path: while set the model begin selects its shader stage configuration and render states, and the sky, object and camouflage passes mark the render states dirty.
 *
 * @address 0x6893ec
 */
static uint8_t &models_enabled = console_debug_toggle_6893ec;

/**
 * Object shadows: set from the profile shadows setting (zero on shader versions below 1.1 and by the device reset); read by the model shadow passes and shown as Shadows by the time demo.
 *
 * @address 0x6893f2
 */
static uint8_t &object_shadows_enabled = console_debug_toggle_6893f2;

/**
 * hs global "rasterizer_environment_diffuse_textures": draw environment diffuse textures.
 *
 * @address 0x6893f4
 */
static uint8_t &rasterizer_environment_diffuse_textures = console_debug_toggle_6893f4;

/**
 * Specular: set from the profile specular setting with the other specular gates and shown as Specular by the time demo; read by the decal pass and the environment specular passes.
 *
 * @address 0x6893fa
 */
static uint8_t &specular_enabled = console_debug_toggle_6893fa;

/**
 * Gate of rasterizer_transparent_object_append (default 1, read nowhere else).
 *
 * @address 0x689400
 */
static uint8_t &transparent_object_append_enabled = console_debug_toggle_689400;

/**
 * hs global "rasterizer_fog_atmosphere": atmospheric fog.
 *
 * @address 0x689407
 */
static uint8_t &rasterizer_fog_atmosphere = console_debug_toggle_689407;

/**
 * hs global "rasterizer_fog_plane": planar fog.
 *
 * @address 0x689408
 */
static uint8_t &rasterizer_fog_plane = console_debug_toggle_689408;

/**
 * Gate of the object shadow pass of the window render.
 *
 * @address 0x69c614
 */
static uint8_t &object_shadow_pass_enabled = console_debug_toggle_69c614;

/**
 * Bump mapping (the script debug table lists it as "rasterizer_bump_mapping"): the environment, model and water paths bind their bump maps only while it is set.
 *
 * @address 0x689409
 */
static uint8_t &bump_mapping_enabled = console_debug_toggle_689409;

/**
 * Gate of the motion sensor draw and update (script debug "rasterizer_hud_motion_sensor").
 *
 * @address 0x689403
 */
static uint8_t &hud_motion_sensor_enabled = console_debug_toggle_689403;

/**
 * Active camouflage (script debug "rasterizer_active_camouflage"): the camouflage render target and the camouflage shader paths are used only while it is set.
 *
 * @address 0x689421
 */
static uint8_t &active_camouflage_enabled = console_debug_toggle_689421;

/**
 * Shadow convolution (script debug "rasterizer_shadows_convolution", by position): the object shadow pass binds the convolved shadow map on texture stage 4 instead of 3.
 *
 * @address 0x68941e
 */
static uint8_t &shadow_convolution_enabled = console_debug_toggle_68941e;

/**
 * Alpha testing of environment surfaces (script debug "rasterizer_environment_alpha_testing"): enables the alpha test render state for shaders with the alpha test bit.
 *
 * @address 0x68941c
 */
static uint8_t &environment_alpha_testing_enabled = console_debug_toggle_68941c;

/**
 * Dynamic diffuse lights on environment surfaces (script debug "rasterizer_environment_diffuse_lights", by position); gates the dynamic light passes of the lighting code.
 *
 * @address 0x6893f3
 */
static uint8_t &environment_diffuse_lights_enabled = console_debug_toggle_6893f3;

/**
 * Debug toggle (cleared by default only on the console): when zero, or when more than one local view is
 * rendered, the detail object begin/draw/end passes return without drawing anything. Set by the video settings
 * apply path.
 *
 * @address 0x689404
 */
static uint8_t &detail_objects_enabled = console_debug_toggle_689404;

}
