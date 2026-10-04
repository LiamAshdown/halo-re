/**
 * @file src/rasterizer/screen_effects.cpp
 * Screen space effects, sun glow, motion sensor, UI quads and gamma.
 */

#include "halo/render/d3d9.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/core/datum.hpp"
#include "halo/rasterizer/globals.hpp"
#include "internal/state.hpp"
#include "halo/core/lcg.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/render/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/libm.hpp"
#include <string.h>




namespace {

constexpr const char *k_halo_registry_key = "Software\\Microsoft\\Microsoft Games\\Halo";
constexpr uint32_t k_motion_sensor_background_color = halo::d3d9::color_argb(0xff, 0x74, 0xb9, 0xff);
constexpr uint32_t k_motion_sensor_blip_color = halo::d3d9::color_argb(0xff, 0x66, 0xcc, 0x66);
constexpr uint32_t k_filter_cap_min_anisotropic = 0x400;
constexpr uint32_t k_filter_cap_mag_anisotropic = 0x4000000;
constexpr uint32_t k_blend_cap_blend_factor = 0x2000;

}  // namespace

namespace halo::rasterizer {

/**
 * Per-frame update of the default near clip distance (cinematic override), latches the current frame time into
 * rasterizer_time, and drives the lens flare visibility smoothing pass plus two external frame counters (the
 * second only when a debug toggle is set).
 *
 * Registers: ECX -> time_source
 *
 * @address 0x517470
 */
void chimera__cinematic_screen_effect(rasterizer_frame_time *time_source)
{
    rasterizer_default_z_near = 0.0625f;

    if (cinematic_screen_effect_state != (cinematic_screen_effect_globals *)0) {
        float near_clip = cinematic_screen_effect_state->near_clip_distance;
        if (near_clip > 0.0f) {
            rasterizer_default_z_near = near_clip;
        }
    }

    rasterizer_time = *time_source;

    halo::rasterizer::fields::water_ripple_update_pending = (uint8_t)(1 - (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1));
    halo::rasterizer::fields::transparent_group_created = 0;

    lens_flare_update_visibility();

    halo::cache::globals().texture_cache->age = halo::cache::globals().texture_cache->age + 1;
    if (decals_for_all_responses != 0) {
        rasterizer_decal_vertex_cache_handle->age = rasterizer_decal_vertex_cache_handle->age + 1;
        halo::effects::decals_update_fade();
    }
}


/**
 * Direct3D 9 back end function chimera__gamma.
 *
 * @address 0x5227a0
 */
void chimera__gamma(void)
{
    uint32_t i;
    int16_t ramp_value;
    float exponent;
    HDC dc;

    if (rasterizer_gamma_disabled != 0 && rasterizer_gamma_captured != 0) {
        return;
    }

    exponent = (float)(halo::libm::log((double)rasterizer_gamma_exponent * 0.003921568859368563) / halo::libm::log(0.5));
    for (i = 0; i < k_gamma_ramp_entries; i++) {
        ramp_value = (int16_t)(int32_t)(halo::libm::pow((double)i * 0.003921568859368563, exponent) * 65535.0);
        rasterizer_game_gamma_ramp.red[i] = ramp_value;
        rasterizer_game_gamma_ramp.green[i] = ramp_value;
        rasterizer_game_gamma_ramp.blue[i] = ramp_value;
    }

    if (rasterizer_gamma_high_bit_17 == 1 && rasterizer_fullscreen != 0 && rasterizer_device != 0) {
        render_device().set_gamma_ramp(0, 0, &rasterizer_game_gamma_ramp);
        return;
    }

    dc = GetDC(rasterizer_window_handle);
    SetDeviceGammaRamp(dc, &rasterizer_game_gamma_ramp);
    ReleaseDC(rasterizer_window_handle, dc);
}

/**
 * Direct3D 9 back end function chimera__registry_check_3.
 *
 * @address 0x5226c0
 */
void chimera__registry_check_3(void)
{
    HDC dc;
    uint8_t zero_value[4] = {0, 0, 0, 0};
    HKEY key;

    if (rasterizer_gamma_disabled == 0 && rasterizer_gamma_captured != 0) {
        if (rasterizer_gamma_high_bit_17 == 1 && rasterizer_fullscreen != 0 && rasterizer_device != 0) {
            render_device().set_gamma_ramp(0, 0, &rasterizer_desktop_gamma_ramp);
            return;
        }

        dc = GetDC(rasterizer_window_handle);
        SetDeviceGammaRamp(dc, &rasterizer_desktop_gamma_ramp);
        ReleaseDC(rasterizer_window_handle, dc);

        RegCreateKeyExA(HKEY_CURRENT_USER, k_halo_registry_key, 0,
                         (LPSTR)0, 0, KEY_WRITE, (LPSECURITY_ATTRIBUTES)0, (PHKEY)&key, (LPDWORD)0);
        RegSetValueExA(key, "gamma", 0, REG_DWORD, zero_value, 4);
        RegCloseKey(key);
    }
}
/**
 * 0x522890, blam-cc: EAX
 *
 * @address 0x522520
 */
void chimera__registry_check_4(void)
{
    int32_t i;
    HKEY key;
    DWORD value_size;
    int32_t gamma_flag;
    HDC dc;

    int32_t nogamma_argument = 0;
    for (i = 0; i < halo::shell::globals().argc; i++) {
        char *arg = halo::shell::globals().argv[i];
        if (*arg == '-' && _stricmp("-nogamma", arg) == 0) {
            nogamma_argument = 1;
            break;
        }
    }
    if (nogamma_argument || safe_mode != 0) {
        rasterizer_gamma_disabled = 1;
    } else {
        rasterizer_gamma_disabled = 0;
        if (halo::shell::globals().safe_mode != 0) {
            rasterizer_gamma_disabled = 1;
        }
    }
    rasterizer_gamma_high_bit_17 = (uint8_t)((rasterizer_caps.caps2 >> 0x11) & 1);

    gamma_flag = 0;
    value_size = 4;
    RegOpenKeyExA(HKEY_CURRENT_USER, k_halo_registry_key, 0, KEY_READ, (PHKEY)&key);
    RegQueryValueExA(key, "gamma", (LPDWORD)0, (LPDWORD)0, (LPBYTE)&gamma_flag, &value_size);
    RegCloseKey(key);

    if (gamma_flag == 0) {
        dc = GetDC(rasterizer_window_handle);
        GetDeviceGammaRamp(dc, &rasterizer_desktop_gamma_ramp);
        ReleaseDC(rasterizer_window_handle, dc);
    } else {
        int32_t j;
        for (j = 0; j < k_gamma_ramp_entries; j++) {
            uint16_t value = (uint16_t)(j << 8);
            rasterizer_desktop_gamma_ramp.red[j] = value;
            rasterizer_desktop_gamma_ramp.green[j] = value;
            rasterizer_desktop_gamma_ramp.blue[j] = value;
        }
    }

    rasterizer_gamma_brightness_to_exponent(&rasterizer_desktop_gamma_ramp);

    gamma_flag = 1;
    RegCreateKeyExA(HKEY_CURRENT_USER, k_halo_registry_key, 0, (LPSTR)0,
                     0, KEY_WRITE, (LPSECURITY_ATTRIBUTES)0, (PHKEY)&key, (LPDWORD)0);
    RegSetValueExA(key, "gamma", 0, REG_DWORD, (const BYTE *)&gamma_flag, 4);
    RegCloseKey(key);

    rasterizer_gamma_captured = 1;
}
namespace rasterizer_fog_screen_overlay_set_states_impl {


/**
 * Direct3D 9 back end function rasterizer_fog_screen_overlay_set_states.
 *
 * @address 0x51def0
 */
void rasterizer_fog_screen_overlay_set_states(void)
{
    uint32_t stage5_filter, stage6_filter;
    uint32_t max_anisotropy;
    uint32_t fog_color;

    if (halo::rasterizer::fields::rasterizer_environment_diffuse_textures == 0) {
        return;
    }

    render_device().set_render_state(halo::d3d9::rs::cull_mode, 3);
    render_device().set_render_state(halo::d3d9::rs::color_write_enable, (console_debug_toggle_68941d != 0) * 8 + 7);
    render_device().set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    render_device().set_render_state(halo::d3d9::rs::src_blend, (-(uint32_t)(halo::rasterizer::fields::rasterizer_debug_mode != 1) & 7) + 2);
    render_device().set_render_state(halo::d3d9::rs::dest_blend, (halo::rasterizer::fields::rasterizer_debug_mode == 1) + 1);
    render_device().set_render_state(halo::d3d9::rs::blend_op, 1);
    render_device().set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
    render_device().set_render_state(halo::d3d9::rs::z_enable, halo::rasterizer::fields::rasterizer_debug_mode != 1);
    render_device().set_render_state(halo::d3d9::rs::z_func, 3);
    render_device().set_render_state(halo::d3d9::rs::z_write_enable, 0);

    stage5_filter = 2;
    stage6_filter = 2;
    if (halo::shell::globals().use_anisotropic_filter != 0 && halo::d3d9::has_raster_cap(rasterizer_caps.raster_caps, halo::d3d9::raster_cap::anisotropy) &&
        1 < rasterizer_caps.max_anisotropy) {
        max_anisotropy = 8;
        if (rasterizer_caps.max_anisotropy < 8) {
            max_anisotropy = rasterizer_caps.max_anisotropy;
        }
        render_device().set_sampler_state(0, halo::d3d9::ss::max_anisotropy, max_anisotropy);
        render_device().set_sampler_state(1, halo::d3d9::ss::max_anisotropy, max_anisotropy);
        if (halo::d3d9::k_pixel_shader_version_1_0 < rasterizer_caps.pixel_shader_version) {
            render_device().set_sampler_state(2, halo::d3d9::ss::max_anisotropy, max_anisotropy);
            render_device().set_sampler_state(3, halo::d3d9::ss::max_anisotropy, max_anisotropy);
        }
        if ((rasterizer_caps.texture_filter_caps & k_filter_cap_min_anisotropic) != 0) {
            stage6_filter = 3;
        }
        if ((rasterizer_caps.texture_filter_caps & k_filter_cap_mag_anisotropic) != 0) {
            stage5_filter = 3;
        }
    }

    render_device().set_sampler_state(0, halo::d3d9::ss::address_u, 1);
    render_device().set_sampler_state(0, halo::d3d9::ss::address_v, 1);
    render_device().set_sampler_state(0, halo::d3d9::ss::mag_filter, stage5_filter);
    render_device().set_sampler_state(0, halo::d3d9::ss::min_filter, stage6_filter);
    render_device().set_sampler_state(0, halo::d3d9::ss::mip_filter, 2);
    render_device().set_sampler_state(1, halo::d3d9::ss::address_u, 1);
    render_device().set_sampler_state(1, halo::d3d9::ss::address_v, 1);
    render_device().set_sampler_state(1, halo::d3d9::ss::mag_filter, stage5_filter);
    render_device().set_sampler_state(1, halo::d3d9::ss::min_filter, stage6_filter);
    render_device().set_sampler_state(1, halo::d3d9::ss::mip_filter, 2);

    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
        render_device().set_render_state(halo::d3d9::rs::fog_enable, rasterizer_fog_enabled);
        fog_color = halo::interface::color_rgb_float_to_int(&rasterizer_window.fog.atmospheric_color.red);
        render_device().set_render_state(halo::d3d9::rs::fog_color, fog_color);
        rasterizer_set_shader_stage_config(5);
        return;
    }

    render_device().set_render_state(halo::d3d9::rs::fog_enable, 0);

    render_device().set_sampler_state(2, halo::d3d9::ss::address_u, 1);
    render_device().set_sampler_state(2, halo::d3d9::ss::address_v, 1);
    render_device().set_sampler_state(2, halo::d3d9::ss::mag_filter, stage5_filter);
    render_device().set_sampler_state(2, halo::d3d9::ss::min_filter, stage6_filter);
    render_device().set_sampler_state(2, halo::d3d9::ss::mip_filter, 2);
    render_device().set_sampler_state(3, halo::d3d9::ss::address_u, 1);
    render_device().set_sampler_state(3, halo::d3d9::ss::address_v, 1);
    render_device().set_sampler_state(3, halo::d3d9::ss::mag_filter, stage5_filter);
    render_device().set_sampler_state(3, halo::d3d9::ss::min_filter, stage6_filter);
    render_device().set_sampler_state(3, halo::d3d9::ss::mip_filter, 2);

    rasterizer_set_shader_stage_config(5);
}

}  // namespace rasterizer_fog_screen_overlay_set_states_impl

/** Byte offset of red[128] in the d3d gamma ramp, the sample the brightness exponent is derived from (settings is a byte pointer). */


/**
 * inline fldl2e / f2xm1 / fscale
 *
 * Registers: EAX = settings
 *
 * @address 0x522890
 */
void rasterizer_gamma_brightness_to_exponent(d3d_gamma_ramp *settings)
{
    uint16_t brightness;
    double brightness_norm;
    double ratio;
    double exponent;
    double scaled;

    brightness = settings->red[k_gamma_ramp_entries / 2];
    brightness_norm = (double)brightness * 1.5259021896696422e-05;
    ratio = halo::libm::log(0.5) / halo::libm::log(0.5019607843137255);
    exponent = halo::libm::log(brightness_norm) * ratio;
    scaled = halo::libm::exp(exponent) * 255.0;

    rasterizer_gamma_exponent = (int32_t)scaled;
    if (halo::d3d9::k_pixel_shader_version_1_0 < rasterizer_caps.pixel_shader_version) {
        rasterizer_gamma_exponent = rasterizer_gamma_exponent + 10;
    }
    if (rasterizer_gamma_exponent < 1) {
        rasterizer_gamma_exponent = 1;
        video_gamma_current = 1;
        return;
    }
    if (0xfe < rasterizer_gamma_exponent) {
        rasterizer_gamma_exponent = 0xfe;
    }
    video_gamma_current = rasterizer_gamma_exponent;
}

namespace rasterizer_motion_sensor_begin_impl {


static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

static void set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(sampler, type, value);
}

static BitmapData *first_bitmap_data(const TagDependency &bitmap_reference)
{
    const Bitmap *bitmap = (const Bitmap *)halo::cache::globals().tag_instances[halo::tag_id_bits(bitmap_reference.tag_id) & halo::k_slot_mask].data;

    if (bitmap != NULL && (int32_t)bitmap->bitmap_data.count > 0) {
        return (BitmapData *)(uintptr_t)bitmap->bitmap_data.pointer;
    }
    return NULL;
}

/**
 * Direct3D 9 back end function rasterizer_motion_sensor_begin.
 *
 * @address 0x52b690
 */
void rasterizer_motion_sensor_begin(void)
{
    GlobalsInterfaceBitmaps *interface_bitmaps;
    BitmapData *blip_bitmap;
    BitmapData *goo_bitmap;
    void *surface;
    d3d_surface_desc desc;
    d3d_viewport viewport;
    float constants[5][4];
    int i, j;

    interface_bitmaps = global_globals->interface_bitmaps.count != 0
                            ? (GlobalsInterfaceBitmaps *)(uintptr_t)global_globals->interface_bitmaps.pointer
                            : NULL;
    blip_bitmap = first_bitmap_data(interface_bitmaps->motion_sensor_blip_bitmap);
    goo_bitmap = first_bitmap_data(interface_bitmaps->interface_goo_map1);

    rasterizer_motion_sensor_ready = 0;
    if (rasterizer_caps_flag_689 || !halo::rasterizer::fields::hud_motion_sensor_enabled) {
        return;
    }
    if (halo::cache::texture_cache_get(blip_bitmap, 0, 1) == NULL) {
        return;
    }
    if (halo::cache::texture_cache_get(goo_bitmap, 0, 1) == NULL) {
        return;
    }

    surface = rasterizer_render_targets[5].surface;
    rasterizer_motion_sensor_ready = 1;
    render_device().set_render_target(0, (uint32_t)(uintptr_t)surface);
    rasterizer_active_render_target = 5;
    render_device().surface_get_desc(surface, &desc);
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = desc.width;
    viewport.height = desc.height;
    viewport.min_z = 0.0f;
    viewport.max_z = 1.0f;
    render_device().set_viewport(&viewport);
    render_device().clear(0, NULL, 1, 0, 1.0f, 0);

    rasterizer_bind_texture_d3d9(0, blip_bitmap);
    set_sampler_state(0, halo::d3d9::ss::address_u, 3);
    set_sampler_state(0, halo::d3d9::ss::address_v, 3);
    set_sampler_state(0, halo::d3d9::ss::mag_filter, 2);
    set_sampler_state(0, halo::d3d9::ss::min_filter, 2);
    set_sampler_state(0, halo::d3d9::ss::mip_filter, 1);
    set_render_state(halo::d3d9::rs::cull_mode, 3);
    set_render_state(halo::d3d9::rs::color_write_enable, 7);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::one);
    set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
    set_render_state(halo::d3d9::rs::blend_op, 1);
    set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
    set_render_state(halo::d3d9::rs::z_enable, 0);
    set_render_state(halo::d3d9::rs::fog_enable, 0);
    render_device().set_vertex_declaration(rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].declaration);

    render_device().set_software_vertex_processing(((rasterizer_software_vertex_processing ? 0x10 : 0) |
                                                rasterizer_vertex_declarations[6].usage) & 0x10);
    render_device().set_vertex_shader(rasterizer_vertex_shaders[35].shader);
    render_device().set_pixel_shader(0);

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            constants[i][j] = (i == j) ? 1.0f : 0.0f;
        }
    }
    constants[4][0] = 1.0f;
    constants[4][1] = 1.0f;
    constants[4][2] = 0.0f;
    constants[4][3] = 1.0f;
    render_device().set_vertex_shader_constant_f(0xd, &constants[0][0], 5);
    set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
    set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
    set_texture_stage_state(0, halo::d3d9::ts::color_arg2, halo::d3d9::ta::diffuse);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::modulate);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_arg2, halo::d3d9::ta::diffuse);
    set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
    set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
}

}  // namespace rasterizer_motion_sensor_begin_impl

namespace rasterizer_motion_sensor_blip_draw_impl {


/**
 * Draws one blip as a two triangle fan quad of half size size/16 around (x, y) * -1/32, with the color scaled
 * by brightness and forced opaque.
 *
 * @address 0x52bad0
 */
void rasterizer_motion_sensor_blip_draw(const float *position, const float *color, float brightness, float size)
{
    rasterizer_dynamic_screen_vertex vertices[4];
    float half_size;
    float x, y;
    uint32_t packed;
    int i;

    if (!halo::rasterizer::fields::hud_motion_sensor_enabled || !rasterizer_motion_sensor_ready) {
        return;
    }
    half_size = size * 0.0625f;
    x = position[0] * -0.03125f;
    y = position[1] * -0.03125f;
    packed = (uint32_t)(int32_t)(brightness * color[0] * 255.0f);
    packed = (packed | ~0xffu) << 8;
    packed = (packed | ((uint32_t)(int32_t)(brightness * color[1] * 255.0f) & 0xff)) << 8;
    packed = packed | ((uint32_t)(int32_t)(brightness * color[2] * 255.0f) & 0xff);

    vertices[0].x = x - half_size;
    vertices[0].y = y + half_size;
    vertices[0].u = 0.0f;
    vertices[0].v = 0.0f;
    vertices[1].x = x + half_size;
    vertices[1].y = y + half_size;
    vertices[1].u = 1.0f;
    vertices[1].v = 0.0f;
    vertices[2].x = x + half_size;
    vertices[2].y = y - half_size;
    vertices[2].u = 1.0f;
    vertices[2].v = 1.0f;
    vertices[3].x = x - half_size;
    vertices[3].y = y - half_size;
    vertices[3].u = 0.0f;
    vertices[3].v = 1.0f;
    for (i = 0; i < 4; i++) {
        vertices[i].z = 0.0f;
        vertices[i].color = packed;
    }
    render_device().draw_primitive_up(6, 2, vertices, sizeof(rasterizer_dynamic_screen_vertex));
}

}  // namespace rasterizer_motion_sensor_blip_draw_impl

namespace rasterizer_motion_sensor_end_impl {


static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

static void set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(sampler, type, value);
}

static void draw_fan(const rasterizer_dynamic_screen_vertex *vertices)
{
    render_device().draw_primitive_up(6, 2, vertices, sizeof(rasterizer_dynamic_screen_vertex));
}

static void set_vertex(rasterizer_dynamic_screen_vertex *vertex, float x, float y, uint32_t color, float u, float v)
{
    vertex->x = x;
    vertex->y = y;
    vertex->z = 0.0f;
    vertex->color = color;
    vertex->u = u;
    vertex->v = v;
}

static BitmapData *first_bitmap_data(const TagDependency &bitmap_reference)
{
    const Bitmap *bitmap = (const Bitmap *)halo::cache::globals().tag_instances[halo::tag_id_bits(bitmap_reference.tag_id) & halo::k_slot_mask].data;

    if (bitmap != NULL && (int32_t)bitmap->bitmap_data.count > 0) {
        return (BitmapData *)(uintptr_t)bitmap->bitmap_data.pointer;
    }
    return NULL;
}

/**
 * Direct3D 9 back end function rasterizer_motion_sensor_end.
 *
 * @address 0x52bc40
 */
void rasterizer_motion_sensor_end(const float *position, float sweep)
{
    GlobalsInterfaceBitmaps *interface_bitmaps;
    BitmapData *sweep_bitmap;
    BitmapData *mask_bitmap;
    rasterizer_dynamic_screen_vertex vertices[4];
    float constants[5][4];
    float half_size;

    interface_bitmaps = global_globals->interface_bitmaps.count != 0
                            ? (GlobalsInterfaceBitmaps *)(uintptr_t)global_globals->interface_bitmaps.pointer
                            : NULL;
    sweep_bitmap = first_bitmap_data(interface_bitmaps->motion_sensor_sweep_bitmap);
    mask_bitmap = first_bitmap_data(interface_bitmaps->motion_sensor_sweep_bitmap_mask);

    if (!halo::rasterizer::fields::hud_motion_sensor_enabled) {
        return;
    }
    if (!rasterizer_motion_sensor_ready ||
        halo::cache::texture_cache_get(sweep_bitmap, 0, 1) == NULL ||
        halo::cache::texture_cache_get(mask_bitmap, 0, 1) == NULL) {

        if (halo::rasterizer::fields::hud_motion_sensor_enabled && rasterizer_motion_sensor_ready) {
            rasterizer_render_target_set_active(rasterizer_window.type, 0, 0);
        }
        return;
    }

    rasterizer_bind_texture_d3d9(0, sweep_bitmap);
    set_sampler_state(0, halo::d3d9::ss::address_u, 3);
    set_sampler_state(0, halo::d3d9::ss::address_v, 3);
    set_sampler_state(0, halo::d3d9::ss::mag_filter, 2);
    set_sampler_state(0, halo::d3d9::ss::min_filter, 2);
    set_sampler_state(0, halo::d3d9::ss::mip_filter, 0);
    set_render_state(halo::d3d9::rs::cull_mode, 3);
    set_render_state(halo::d3d9::rs::color_write_enable, 7);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::one);
    set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::src_alpha);
    set_render_state(halo::d3d9::rs::blend_op, 1);
    set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
    set_render_state(halo::d3d9::rs::z_enable, 0);
    set_render_state(halo::d3d9::rs::fog_enable, 0);
    render_device().set_vertex_shader_constant_f(0xd, &rasterizer_identity_vertex_constants[0][0], 5);
    set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
    set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
    set_texture_stage_state(0, halo::d3d9::ts::color_arg2, halo::d3d9::ta::diffuse);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::diffuse);
    set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
    set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
    if (sweep <= 2.75f) {
        float t = sweep * 0.5f;
        float near_u = t + 0.5f;
        float far_u = 0.5f - t;

        set_vertex(&vertices[0], -1.015625f, 1.046875f, k_motion_sensor_background_color, near_u, far_u);
        set_vertex(&vertices[1], 1.046875f, 1.046875f, k_motion_sensor_background_color, far_u, far_u);
        set_vertex(&vertices[2], 1.046875f, -1.015625f, k_motion_sensor_background_color, far_u, near_u);
        set_vertex(&vertices[3], -1.015625f, -1.015625f, k_motion_sensor_background_color, near_u, near_u);
        draw_fan(vertices);
    }

    set_sampler_state(0, halo::d3d9::ss::mip_filter, 1);
    rasterizer_bind_texture_d3d9(0, mask_bitmap);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::zero);
    set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::src_alpha);
    set_render_state(halo::d3d9::rs::fog_enable, 0);
    set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
    set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
    set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
    set_vertex(&vertices[0], -1.015625f, 1.046875f, k_motion_sensor_blip_color, 1.0f, 0.0f);
    set_vertex(&vertices[1], 1.046875f, 1.046875f, k_motion_sensor_blip_color, 0.0f, 0.0f);
    set_vertex(&vertices[2], 1.046875f, -1.015625f, k_motion_sensor_blip_color, 0.0f, 1.0f);
    set_vertex(&vertices[3], -1.015625f, -1.015625f, k_motion_sensor_blip_color, 1.0f, 1.0f);
    draw_fan(vertices);

    rasterizer_render_target_set_active(rasterizer_window.type, 0, 0);
    render_device().set_texture(0, rasterizer_render_targets[5].texture);
    set_sampler_state(0, halo::d3d9::ss::address_u, 3);
    set_sampler_state(0, halo::d3d9::ss::address_v, 3);
    set_sampler_state(0, halo::d3d9::ss::mag_filter, 2);
    set_sampler_state(0, halo::d3d9::ss::min_filter, 2);
    set_sampler_state(0, halo::d3d9::ss::mip_filter, 1);
    set_render_state(halo::d3d9::rs::cull_mode, 3);
    set_render_state(halo::d3d9::rs::color_write_enable, 7);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::one);
    set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
    set_render_state(halo::d3d9::rs::blend_op, 1);
    set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
    set_render_state(halo::d3d9::rs::z_enable, 0);
    set_render_state(halo::d3d9::rs::fog_enable, 0);

    constants[0][0] = 0.003125f;      constants[0][1] = 0.0f;           constants[0][2] = 0.0f; constants[0][3] = -1.0015625f;
    constants[1][0] = 0.0f;           constants[1][1] = -0.004166667f;  constants[1][2] = 0.0f; constants[1][3] = 1.0020833f;
    constants[2][0] = 0.0f;           constants[2][1] = 0.0f;           constants[2][2] = 0.0f; constants[2][3] = 0.5f;
    constants[3][0] = 0.0f;           constants[3][1] = 0.0f;           constants[3][2] = 0.0f; constants[3][3] = 1.0f;
    constants[4][0] = 1.0f;           constants[4][1] = 1.0f;           constants[4][2] = 0.0f; constants[4][3] = 1.0f;
    render_device().set_vertex_shader_constant_f(0xd, &constants[0][0], 5);
    half_size = (local_player_globals->local_player_count > 1) ? 32.0f : 42.0f;
    set_vertex(&vertices[0], position[0] - half_size, position[1] - half_size, halo::d3d9::k_color_white, 0.0f, 0.0f);
    set_vertex(&vertices[1], position[0] + half_size, position[1] - half_size, halo::d3d9::k_color_white, 1.0f, 0.0f);
    set_vertex(&vertices[2], position[0] + half_size, position[1] + half_size, halo::d3d9::k_color_white, 1.0f, 1.0f);
    set_vertex(&vertices[3], position[0] - half_size, position[1] + half_size, halo::d3d9::k_color_white, 0.0f, 1.0f);
    draw_fan(vertices);
    render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
}

}  // namespace rasterizer_motion_sensor_end_impl

namespace rasterizer_screen_effect_compute_uv_transform_impl {


static void texel_size(const BitmapData *bitmap, float *u, float *v)
{
    if (bitmap->flags & 0x10) {
        *u = 1.0f;
        *v = 1.0f;
    } else {
        *u = 1.0f / (float)(int16_t)bitmap->width;
        *v = 1.0f / (float)(int16_t)bitmap->height;
    }
}

/**
 * Direct3D 9 back end function rasterizer_screen_effect_compute_uv_transform.
 *
 * @address 0x52ce50
 */
void rasterizer_screen_effect_compute_uv_transform(uint32_t width, uint32_t height, weapon_screen_effect_parameters *params, int16_t pass, int16_t pass_count, uint8_t shift_down)
{
    BitmapData frame;
    const BitmapData *mask;
    const BitmapData *map_b;
    const BitmapData *map_c;
    BitmapData *mask_bitmap = (BitmapData *)(uintptr_t)params->mask_bitmap_data;
    uint8_t has_extra_maps;
    uint8_t mask_used;
    float target_width = (float)width;
    float target_height = (float)height;
    float mask_u, mask_v, b_u, b_v, c_u, c_v, frame_u, frame_v;
    float mask_w, mask_h, extra_w, extra_h;
    float frame_width, frame_height;
    float inverse_width, inverse_height;
    float m[8][4];
    int i;

    frame.bitmap_class = halo::fourcc('b', 'i', 't', 'm');
    frame.width = (uint16_t)width;
    frame.height = (uint16_t)height;
    frame.depth = 1;
    frame.type = 0;
    frame.format = static_cast<BitmapDataFormat_t>(-1);
    frame.flags = halo::shell::globals().linear_texture_addressing_zoom ? 0x10 : 0;
    frame.registration_point.x = 0;
    frame.registration_point.y = 0;
    frame.mipmap_count = 0;
    frame._pad_16[0] = 0;
    frame._pad_16[1] = 0;
    frame.pixel_data_offset = 0;
    frame.pixel_data_size = 0;
    frame.bitmap_tag_id.index = 0;
    frame.bitmap_tag_id.id = 0;
    frame.pointer = 0;
    frame.hardware_texture = 0;
    frame.pixel_base = nullptr;

    mask_used = mask_bitmap != NULL && (pass > 0 || pass_count == 1 || params->convolution_type != 0);
    mask = mask_used ? mask_bitmap : &frame;
    has_extra_maps = params->has_extra_maps;
    map_b = has_extra_maps ? (const BitmapData *)(uintptr_t)params->extra_map_b : &frame;
    map_c = has_extra_maps ? (const BitmapData *)(uintptr_t)params->extra_map_c : &frame;

    texel_size(mask, &mask_u, &mask_v);
    texel_size(map_b, &b_u, &b_v);
    texel_size(map_c, &c_u, &c_v);
    texel_size(&frame, &frame_u, &frame_v);
    frame_width = (float)(int16_t)width;
    frame_height = (float)(int16_t)height;

    mask_w = mask_used ? 640.0f : target_width;
    mask_h = mask_used ? 480.0f : target_height;
    extra_w = has_extra_maps ? 640.0f : target_width;
    extra_h = has_extra_maps ? 480.0f : target_height;

    inverse_width = 1.0f / frame_width;
    inverse_height = 1.0f / frame_height;
    for (i = 0; i < 8; i++) {
        m[i][0] = 0.0f;
        m[i][1] = 0.0f;
        m[i][2] = 0.0f;
        m[i][3] = 0.0f;
    }
    m[0][0] = mask_u * inverse_width * mask_w;
    m[0][3] = ((float)(int16_t)mask->width + 1.0f - mask_w) * mask_u * 0.5f;
    m[1][1] = mask_v * inverse_height * mask_h;
    m[1][3] = ((float)(int16_t)mask->height + 1.0f - mask_h) * mask_v * 0.5f;
    m[2][0] = inverse_width * b_u * extra_w;
    m[2][3] = ((float)(int16_t)map_b->width + 1.0f - extra_w) * b_u * 0.5f;
    m[3][1] = b_v * inverse_height * extra_h;
    m[3][3] = ((float)(int16_t)map_b->height + 1.0f - extra_h) * b_v * 0.5f;
    m[4][0] = inverse_width * c_u * extra_w;
    m[4][3] = ((float)(int16_t)map_c->width + 1.0f - extra_w) * c_u * 0.5f;
    m[5][1] = inverse_height * c_v * extra_h;
    m[5][3] = ((float)(int16_t)map_c->height + 1.0f - extra_h) * c_v * 0.5f;
    m[6][0] = frame_u;
    m[6][3] = (frame_width + 1.0f - frame_width) * frame_u * 0.5f;
    m[7][1] = frame_v;
    m[7][3] = (frame_height + 1.0f - frame_height) * frame_v * 0.5f;

    if (params->convolution_type == 1) {
        float amount = params->convolution_amount;

        m[0][3] += mask_bitmap != NULL ? 0.0f : mask_u * amount;
        m[1][3] += mask_bitmap != NULL ? 0.0f : mask_v * amount;
        m[2][3] = m[2][3] - b_u * amount;
        m[3][3] = m[3][3] - b_v * amount;
        m[4][3] += c_u * amount;
        m[5][3] = m[5][3] - c_v * amount;
        m[6][3] = m[6][3] - frame_u * amount;
        m[7][3] += amount * frame_v;
    } else if (params->convolution_type == 2) {
        float amount = params->convolution_amount;
        float mask_shift = mask_bitmap != NULL ? 0.0f : -amount;
        float frame_shift = mask_bitmap != NULL ? -amount : amount + amount;

        m[0][0] = (1.0f - mask_shift / (float)(int16_t)mask->width) * m[0][0];
        m[1][1] = (1.0f - mask_shift / (float)(int16_t)mask->height) * m[1][1];
        m[2][0] = (1.0f - 0.0f / (float)(int16_t)map_b->width) * m[2][0];
        m[3][1] = (1.0f - 0.0f / (float)(int16_t)map_b->height) * m[3][1];
        m[4][0] = (1.0f - amount / (float)(int16_t)map_c->width) * m[4][0];
        m[5][1] = (1.0f - amount / (float)(int16_t)map_c->height) * m[5][1];
        m[6][0] = (1.0f - frame_shift / frame_width) * frame_u;
        m[7][1] = (1.0f - frame_shift / frame_height) * frame_v;
        m[0][3] += mask_shift * mask_u * 0.5f;
        m[1][3] += mask_shift * mask_v * 0.5f;
        m[2][3] += b_u * 0.0f;
        m[3][3] += b_v * 0.0f;
        m[4][3] += amount * c_u * 0.5f;
        m[5][3] += amount * c_v * 0.5f;
        m[6][3] += frame_u * frame_shift * 0.5f;
        m[7][3] += frame_shift * frame_v * 0.5f;
    } else if (pass == 1 && has_extra_maps) {
        m[4][3] += halo::effects::effect_random_fraction() * c_u * (float)(int16_t)map_c->width;
        m[5][3] += halo::effects::effect_random_fraction() * c_v * (float)(int16_t)map_c->height;
    }

    if (shift_down) {

        float b_scale_u = m[2][0], b_offset_u = m[2][3], b_scale_v = m[3][1], b_offset_v = m[3][3];
        float c_scale_u = m[4][0], c_offset_u = m[4][3], c_scale_v = m[5][1], c_offset_v = m[5][3];

        for (i = 0; i < 4; i++) {
            m[i][0] = 0.0f;
            m[i][1] = 0.0f;
            m[i][2] = 0.0f;
        }
        m[0][0] = b_scale_u;
        m[0][3] = b_offset_u;
        m[1][1] = b_scale_v;
        m[1][3] = b_offset_v;
        m[2][0] = c_scale_u;
        m[2][3] = c_offset_u;
        m[3][1] = c_scale_v;
        m[3][3] = c_offset_v;
    }
    render_device().set_vertex_shader_constant_f(0xd, &m[0][0], 8);
}

}  // namespace rasterizer_screen_effect_compute_uv_transform_impl

static const char *const k_video_technique_names[k_rasterizer_screen_effect_techniques] = {
    "VideoOn",
    "VideoOffNonConvolved",
    "VideoOffConvolvedMask",
    "VideoOffConvolvedMaskThreeStage",
    "VideoOffConvolvedMaskFilterLightAndDesaturation",
    "VideoOffConvolvedMaskFilterLight",
    "VideoOffConvolvedMaskFilterDesaturation",
    "VideoOffConvolved",
    "VideoOffConvolvedFilterLightAndDesaturation",
    "VideoOffConvolvedFilterLight",
    "VideoOffConvolvedFilterDesaturation",
};

/**
 * Looks up and caches the effect-technique handles for all variants of the 'video' (old-TV/ convolution)
 * screen effect shader, returning success only if every technique was found.
 *
 * @address 0x52d740
 */
uint8_t rasterizer_screen_effect_init_shaders(void)
{
    int i;

    for (i = 0; i < k_rasterizer_screen_effect_techniques; i++) {
        screen_effect_techniques[i] = (int32_t)(uintptr_t)rasterizer_shader_technique_for_name(
            rasterizer_effects[114].effect, k_video_technique_names[i]);
        if (screen_effect_techniques[i] == 0) {
            return 0;
        }
    }
    return 1;
}

namespace rasterizer_screen_effect_render_impl {


static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(sampler, type, value);
}

static void set_sampler_states(uint32_t sampler, uint32_t address, uint32_t filter, uint32_t mip_filter)
{
    set_sampler_state(sampler, halo::d3d9::ss::address_u, address);
    set_sampler_state(sampler, halo::d3d9::ss::address_v, address);
    set_sampler_state(sampler, halo::d3d9::ss::mag_filter, filter);
    set_sampler_state(sampler, halo::d3d9::ss::min_filter, filter);
    set_sampler_state(sampler, halo::d3d9::ss::mip_filter, mip_filter);
}

static void *screen_effect(void) { return rasterizer_effects[114].effect; }

static void draw_screen_quad(void)
{
    render_device().draw_primitive_up(6, 2, rasterizer_screen_effect_quad, sizeof(rasterizer_dynamic_screen_vertex));
}

static void set_technique(uint32_t technique)
{
    render_device().effect_set_technique(screen_effect(), technique);
}

static void set_vector(int handle_index, const float *vector)
{
    void **handles = rasterizer_effects[114].constant_handles;

    render_device().effect_set_vector(screen_effect(), handles[handle_index], vector);
}

static void set_blend(uint32_t source, uint32_t destination)
{
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    set_render_state(halo::d3d9::rs::src_blend, source);
    set_render_state(halo::d3d9::rs::dest_blend, destination);
    set_render_state(halo::d3d9::rs::blend_op, 1);
}

static uint32_t select_filter_technique(const weapon_screen_effect_parameters *p, int first)
{
    uint32_t technique = 0;

    if (p->night_vision_masked && p->desaturation_masked &&
        p->night_vision_intensity > 0.0f && p->desaturation_intensity > 0.0f) {
        technique = screen_effect_techniques[first];
    } else if (p->night_vision_masked && p->night_vision_intensity > 0.0f) {
        technique = screen_effect_techniques[first + 1];
    } else if (p->desaturation_masked && p->desaturation_intensity > 0.0) {
        technique = screen_effect_techniques[first + 2];
    }
    return technique;
}

/**
 * Direct3D 9 back end function rasterizer_screen_effect_render.
 *
 * @address 0x52d8a0
 */
void rasterizer_screen_effect_render(weapon_screen_effect_parameters *input)
{
    weapon_screen_effect_parameters *p;
    int16_t pass_count;
    int16_t pass;
    int16_t source;
    int16_t destination;
    uint32_t source_width, source_height;
    uint32_t passes;
    uint32_t effect_pass;

    p = (weapon_screen_effect_parameters *)halo::render::cinematic_screen_effect_update((cinematic_screen_effect_globals *)input);
    if (p == NULL) {
        return;
    }
    if (p->convolution_type == 0 && p->mask_bitmap_data == 0 && !(p->night_vision_intensity > 0.0f) &&
        !(p->desaturation_intensity > 0.0f) && p->has_extra_maps == 0) {
        return;
    }
    if (!console_debug_toggle_689428 || rasterizer_window.type != 1) {
        return;
    }
    pass_count = (int16_t)((uint16_t)(p->convolution_extra_passes + 1) << 1);

    rasterizer_screen_effect_quad[0].color = halo::d3d9::k_color_white;
    rasterizer_screen_effect_quad[1].color = halo::d3d9::k_color_white;
    rasterizer_screen_effect_quad[2].color = halo::d3d9::k_color_white;
    rasterizer_screen_effect_quad[3].color = halo::d3d9::k_color_white;
    rasterizer_screen_effect_quad[0].x = -1.0f;
    rasterizer_screen_effect_quad[0].y = -1.0f;
    rasterizer_screen_effect_quad[1].x = 1.0f;
    rasterizer_screen_effect_quad[1].y = -1.0f;
    rasterizer_screen_effect_quad[2].x = 1.0f;
    rasterizer_screen_effect_quad[2].y = 1.0f;
    rasterizer_screen_effect_quad[3].x = -1.0f;
    rasterizer_screen_effect_quad[3].y = 1.0f;
    rasterizer_screen_effect_quad[3].z = 0.0f;
    rasterizer_screen_effect_quad[2].z = 0.0f;
    rasterizer_screen_effect_quad[1].z = 0.0f;
    rasterizer_screen_effect_quad[0].z = 0.0f;
    if (screen_effect() == NULL) {
        render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
        return;
    }
    render_device().set_vertex_declaration(rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].declaration);
    render_device().set_software_vertex_processing(((rasterizer_software_vertex_processing ? 0x10 : 0) |
                                                rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].usage) & 0x10);
    render_device().set_vertex_shader(rasterizer_vertex_shaders[0].shader);

    for (pass = 0; pass < pass_count; pass++) {
        if (pass_count == 1) {
            source = -1;
            destination = -1;
        } else if ((pass & 1) == 0) {
            source = 1;
            destination = 2;
        } else {
            source = 2;
            destination = 1;
        }
        source_width = rasterizer_render_targets[source].width;
        source_height = rasterizer_render_targets[source].height;
        rasterizer_screen_effect_quad[0].u = 0.0f;
        rasterizer_screen_effect_quad[0].v = (float)source_height;
        rasterizer_screen_effect_quad[1].u = (float)source_width;
        rasterizer_screen_effect_quad[1].v = (float)source_height;
        rasterizer_screen_effect_quad[2].u = (float)source_width;
        rasterizer_screen_effect_quad[2].v = 0.0f;
        rasterizer_screen_effect_quad[3].u = 0.0f;
        rasterizer_screen_effect_quad[3].v = 0.0f;

        if ((pass & 1) && p->has_extra_maps) {

            rasterizer_render_target_bind_effect_texture(source, &rasterizer_effects[114], 0);
            set_sampler_states(0, 3, 1, 1);
            rasterizer_bind_texture_d3dx(1, (BitmapData *)(uintptr_t)p->extra_map_b, &rasterizer_effects[114]);
            set_sampler_states(1, 3, 1, 1);
            rasterizer_bind_texture_d3dx(2, (BitmapData *)(uintptr_t)p->extra_map_c, &rasterizer_effects[114]);
            set_sampler_states(2, 1, 2, 1);
        } else {
            int16_t stage;
            BitmapData *mask = (BitmapData *)(uintptr_t)p->mask_bitmap_data;

            for (stage = 0; stage < 4; stage++) {
                if (p->convolution_type == 0) {
                    if (pass_count == 1) {
                        if (stage != 0) {
                            continue;
                        }
                        rasterizer_bind_texture_d3dx(0, mask, &rasterizer_effects[114]);
                    } else if (pass == 0) {
                        if (stage != 0) {
                            continue;
                        }
                        rasterizer_render_target_bind_effect_texture(source, &rasterizer_effects[114], 0);
                    } else if (pass == 1) {
                        if (mask == NULL) {
                            if (stage != 0) {
                                continue;
                            }
                            rasterizer_render_target_bind_effect_texture(source, &rasterizer_effects[114], 0);
                        } else if (stage == 0) {
                            rasterizer_bind_texture_d3dx(0, mask, &rasterizer_effects[114]);
                        } else {
                            rasterizer_render_target_bind_effect_texture(source, &rasterizer_effects[114], stage);
                        }
                    }

                } else if (mask != NULL) {
                    if (stage == 0) {
                        rasterizer_bind_texture_d3dx(0, mask, &rasterizer_effects[114]);
                    } else {
                        rasterizer_render_target_bind_effect_texture(source, &rasterizer_effects[114], stage);
                    }
                } else {
                    if (stage != 0) {
                        break;
                    }
                    rasterizer_render_target_bind_effect_texture(source, &rasterizer_effects[114], 0);
                }
                set_sampler_states((uint32_t)stage, 3, 2, 1);
            }
        }

        set_render_state(halo::d3d9::rs::cull_mode, 1);
        set_render_state(halo::d3d9::rs::color_write_enable, 7);
        set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
        set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
        set_render_state(halo::d3d9::rs::z_enable, 0);
        set_render_state(halo::d3d9::rs::fog_enable, 0);
        rasterizer_screen_effect_compute_uv_transform(source_width, source_height, p, pass, pass_count, 0);
        if (destination != -1) {
            rasterizer_render_target_set_active(destination, 0, 0);
        }

        if (p->has_extra_maps) {
            uint32_t technique = screen_effect_techniques[0];

            if (pass == 1) {
                static const int32_t k_noise_scales[3] = { 1, 2, 4 };
                float noise[4];
                float amount = p->noise_amount;

                noise[0] = (float)k_noise_scales[p->noise_type];
                noise[1] = noise[0];
                noise[2] = noise[0];
                noise[3] = amount < 0.0f ? 0.0f : (amount > 1.0f ? 1.0f : amount);
                if (rasterizer_effects[114].constant_handles != 0) {
                    set_vector(0, noise);
                }
                set_blend(6, 1);
            }
            set_technique(technique);
            render_device().effect_begin(screen_effect(), &passes, 3);
            render_device().effect_pass(screen_effect(), (uint32_t)(int32_t)pass);
            draw_screen_quad();
            render_device().effect_end(screen_effect());
            continue;
        }

        if (p->convolution_type != 0) {
            float tint[4];
            float night_vision[4];
            uint32_t technique;

            tint[0] = p->desaturation_tint[0];
            tint[1] = p->desaturation_tint[1];
            tint[2] = p->desaturation_tint[2];
            tint[3] = p->desaturation_intensity;
            night_vision[0] = p->night_vision_intensity;
            night_vision[1] = p->night_vision_intensity;
            night_vision[2] = p->night_vision_intensity;
            night_vision[3] = p->night_vision_intensity;
            if (p->mask_bitmap_data != 0) {
                technique = (pass == pass_count - 1) ? select_filter_technique(p, 4) : 0;
                if (technique == 0) {
                    technique = halo::shell::globals().use_alternate_convolve_mask ? screen_effect_techniques[3]
                                                                   : screen_effect_techniques[2];
                }
                set_technique(technique);
                if (rasterizer_effects[114].constant_handles != 0) {
                    set_vector(0, tint);
                    set_vector(1, night_vision);
                }
            } else {
                technique = (pass == pass_count - 1) ? select_filter_technique(p, 8) : 0;
                if (technique == 0) {
                    technique = screen_effect_techniques[7];
                }
                set_technique(technique);
            }
        }
        if (pass_count == 1) {
            set_blend(1, 6);
        } else if (pass == pass_count - 1) {
            set_blend(6, 1);
        }
        render_device().effect_begin(screen_effect(), &passes, 3);
        for (effect_pass = 0; effect_pass < passes; effect_pass++) {
            render_device().effect_pass(screen_effect(), effect_pass);
            draw_screen_quad();
        }
        render_device().effect_end(screen_effect());
    }
    rasterizer_render_target_set_active(rasterizer_window.type, 0, 0);
    render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
}

}  // namespace rasterizer_screen_effect_render_impl

namespace rasterizer_screen_effect_render_fixed_function_impl {


static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

static void set_sampler_states(uint32_t sampler, uint32_t address, uint32_t filter, uint32_t mip_filter)
{
    render_device().set_sampler_state(sampler, halo::d3d9::ss::address_u, address);
    render_device().set_sampler_state(sampler, halo::d3d9::ss::address_v, address);
    render_device().set_sampler_state(sampler, halo::d3d9::ss::mag_filter, filter);
    render_device().set_sampler_state(sampler, halo::d3d9::ss::min_filter, filter);
    render_device().set_sampler_state(sampler, halo::d3d9::ss::mip_filter, mip_filter);
}

static void set_blend(uint32_t source, uint32_t destination)
{
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    set_render_state(halo::d3d9::rs::src_blend, source);
    set_render_state(halo::d3d9::rs::dest_blend, destination);
    set_render_state(halo::d3d9::rs::blend_op, 1);
}

static void draw_screen_quad(void)
{
    render_device().draw_primitive_up(6, 2, rasterizer_screen_effect_quad, sizeof(rasterizer_dynamic_screen_vertex));
}

/**
 * Direct3D 9 back end function rasterizer_screen_effect_render_fixed_function.
 *
 * @address 0x52e2d0
 */
void rasterizer_screen_effect_render_fixed_function(weapon_screen_effect_parameters *input)
{
    weapon_screen_effect_parameters *p;
    int32_t width, height;
    int i, j;

    p = (weapon_screen_effect_parameters *)halo::render::cinematic_screen_effect_update((cinematic_screen_effect_globals *)input);
    if (p == NULL) {
        return;
    }
    if (p->convolution_type == 0 && p->mask_bitmap_data == 0 && !(p->night_vision_intensity > 0.0f) &&
        !(p->desaturation_intensity > 0.0f) && p->has_extra_maps == 0) {
        return;
    }
    if (!console_debug_toggle_689428 || rasterizer_window.type != 1) {
        return;
    }
    width = rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left;
    height = rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top;

    render_device().set_pixel_shader(0);
    set_render_state(halo::d3d9::rs::cull_mode, 1);
    set_render_state(halo::d3d9::rs::color_write_enable, 7);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
    set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
    set_render_state(halo::d3d9::rs::z_enable, 0);
    set_render_state(halo::d3d9::rs::fog_enable, 0);
    render_device().set_vertex_declaration(rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].declaration);
    render_device().set_software_vertex_processing(((rasterizer_software_vertex_processing ? 0x10 : 0) |
                                                rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].usage) & 0x10);
    render_device().set_vertex_shader(rasterizer_vertex_shaders[0].shader);

    for (i = 0; i < 4; i++) {
        rasterizer_screen_effect_quad[i].z = 0.0f;
        rasterizer_screen_effect_quad[i].color = halo::d3d9::k_color_white;
    }
    rasterizer_screen_effect_quad[0].x = -1.0f;
    rasterizer_screen_effect_quad[0].y = -1.0f;
    rasterizer_screen_effect_quad[0].u = 0.0f;
    rasterizer_screen_effect_quad[0].v = (float)(uint32_t)height;
    rasterizer_screen_effect_quad[1].x = 1.0f;
    rasterizer_screen_effect_quad[1].y = -1.0f;
    rasterizer_screen_effect_quad[1].u = (float)(uint32_t)width;
    rasterizer_screen_effect_quad[1].v = (float)(uint32_t)height;
    rasterizer_screen_effect_quad[2].x = 1.0f;
    rasterizer_screen_effect_quad[2].y = 1.0f;
    rasterizer_screen_effect_quad[2].u = (float)(uint32_t)width;
    rasterizer_screen_effect_quad[2].v = 0.0f;
    rasterizer_screen_effect_quad[3].x = -1.0f;
    rasterizer_screen_effect_quad[3].y = 1.0f;
    rasterizer_screen_effect_quad[3].u = 0.0f;
    rasterizer_screen_effect_quad[3].v = 0.0f;

    if (p->has_extra_maps) {
        int16_t pass_count = (int16_t)((uint16_t)(p->convolution_extra_passes + 1) << 1);
        int16_t pass;
        float identity[4][4];

        for (pass = 0; pass < pass_count; pass++) {
            if (pass != 1) {
                continue;
            }
            for (i = 0; i < 4; i++) {
                for (j = 0; j < 4; j++) {
                    identity[i][j] = (i == j) ? 1.0f : 0.0f;
                }
            }
            rasterizer_screen_effect_compute_uv_transform((uint32_t)width, (uint32_t)height, p, 1, pass_count, 1);
            rasterizer_bind_texture_d3d9(0, (BitmapData *)(uintptr_t)p->extra_map_b);
            set_sampler_states(0, 3, 1, 1);
            rasterizer_bind_texture_d3d9(1, (BitmapData *)(uintptr_t)p->extra_map_c);
            set_sampler_states(1, 1, 2, 1);
            set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::select_arg1);
            set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
            set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
            set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
            set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
            set_texture_stage_state(1, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
            set_texture_stage_state(1, halo::d3d9::ts::color_arg2, halo::d3d9::ta::current);
            set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
            set_texture_stage_state(1, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
            set_texture_stage_state(2, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
            set_texture_stage_state(2, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
            set_blend(9, 1);
            draw_screen_quad();
            set_texture_stage_state(0, halo::d3d9::ts::texture_transform_flags, 0);
            render_device().set_transform(0x10, &identity[0][0]);
            set_texture_stage_state(1, halo::d3d9::ts::texture_transform_flags, 0);
            render_device().set_transform(0x11, &identity[0][0]);
        }
    } else if (p->mask_bitmap_data != 0) {
        BitmapData *mask = (BitmapData *)(uintptr_t)p->mask_bitmap_data;

        set_sampler_states(0, 3, 2, 1);
        set_sampler_states(1, 3, 2, 1);
        rasterizer_screen_effect_compute_uv_transform((uint32_t)width, (uint32_t)height, p, 0, 1, 0);
        if (p->convolution_type != 0) {
            if (p->desaturation_tint[0] == 0.0f && p->desaturation_tint[1] == 0.0f && p->desaturation_tint[2] == 0.0f) {
                set_blend(9, 1);
                rasterizer_bind_texture_d3d9(0, mask);
                set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::select_arg1);
                set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture | halo::d3d9::ta::complement);
                set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
                set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
                set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
                set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
            } else {
                float amount = p->desaturation_intensity;
                float base = (1.0f - amount) * 0.5f;
                uint32_t color;

                set_blend(9, 3);
                color = (uint32_t)(int32_t)((p->desaturation_tint[0] * amount + base) * 255.0f) & 0xff;
                color |= (uint32_t)(int32_t)(amount * 255.0f) << 8;
                color <<= 8;
                color |= (uint32_t)(int32_t)((p->desaturation_tint[1] * amount + base) * 255.0f) & 0xff;
                color <<= 8;
                color |= (uint32_t)(int32_t)((p->desaturation_tint[2] * amount + base) * 255.0f) & 0xff;
                set_render_state(halo::d3d9::rs::texture_factor, color);
                rasterizer_bind_texture_d3d9(0, mask);
                set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
                set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::tfactor);
                set_texture_stage_state(0, halo::d3d9::ts::color_arg2, halo::d3d9::ta::texture | halo::d3d9::ta::complement);
                set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
                set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
                set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
                set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
                set_texture_stage_state(2, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
                set_texture_stage_state(2, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
            }
            draw_screen_quad();
        }
    }
    render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
}

}  // namespace rasterizer_screen_effect_render_fixed_function_impl

/**
 * Looks up and caches the effect-technique handle for each of the six full-screen flash blend modes (Lighten,
 * Darken, Max, Min, Invert, Tint). Stops at the first failure, so a lookup failure partway through leaves the
 * later slots unset from a previous call, if any.
 *
 * @address 0x52ec40
 */
int rasterizer_screen_flash_init_shaders(void)
{
    screen_flash_techniques[0] = rasterizer_shader_technique_for_name(rasterizer_screen_flash_effect, "FlashLighten");
    if (screen_flash_techniques[0] == 0) {
        return 0;
    }
    screen_flash_techniques[1] = rasterizer_shader_technique_for_name(rasterizer_screen_flash_effect, "FlashDarken");
    if (screen_flash_techniques[1] == 0) {
        return 0;
    }
    screen_flash_techniques[2] = rasterizer_shader_technique_for_name(rasterizer_screen_flash_effect, "FlashMax");
    if (screen_flash_techniques[2] == 0) {
        return 0;
    }
    screen_flash_techniques[3] = rasterizer_shader_technique_for_name(rasterizer_screen_flash_effect, "FlashMin");
    if (screen_flash_techniques[3] == 0) {
        return 0;
    }
    screen_flash_techniques[4] = rasterizer_shader_technique_for_name(rasterizer_screen_flash_effect, "FlashInvert");
    if (screen_flash_techniques[4] == 0) {
        return 0;
    }
    screen_flash_techniques[5] = rasterizer_shader_technique_for_name(rasterizer_screen_flash_effect, "FlashTint");
    if (screen_flash_techniques[5] == 0) {
        return 0;
    }
    return 1;
}

namespace rasterizer_screen_flash_render_impl {


static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static uint32_t pack_argb_bytes(float alpha, float red, float green, float blue)
{
    uint32_t a = (uint32_t)(int32_t)(alpha * 255.0f) & 0xff;
    uint32_t r = (uint32_t)(int32_t)(red * 255.0f) & 0xff;
    uint32_t g = (uint32_t)(int32_t)(green * 255.0f) & 0xff;
    uint32_t b = (uint32_t)(int32_t)(blue * 255.0f) & 0xff;
    return (a << 24) | (r << 16) | (g << 8) | b;
}

static uint32_t pack_argb_bytes_clamped_alpha(ColorARGB color, float intensity)
{
    float alpha_i = color.alpha * intensity;
    if (0.25f < alpha_i) {
        alpha_i = 0.25f;
    }
    return pack_argb_bytes(alpha_i, color.red * intensity, color.green * intensity, color.blue * intensity);
}

/**
 * Renders the full-screen flash post-process effect (Lighten/Darken/Max/Min/Invert/Tint) for the active
 * render_screen_flash entry of the current window parameters.
 *
 * @address 0x52ed00
 */
void rasterizer_screen_flash_render(void)
{
    render_screen_flash *flash;
    ColorARGB color;
    float intensity;
    uint32_t current_color;
    uint32_t inverted_color;
    void *effect;
    void *technique;
    int16_t width, height;
    float constants[5][4];
    float vs_quad[4][6];
    float ps_constants[2][4];
    union { uint32_t bits; float f; } neg_one_bits;
    uint32_t pass_count, pass;

    if (console_debug_toggle_689427 == 0) {
        return;
    }
    flash = &rasterizer_window.screen_flash;
    if (flash->type == 0) {
        return;
    }

    color = flash->color;
    intensity = flash->intensity;
    current_color = pack_argb_bytes(color.alpha * intensity, color.red * intensity, color.green * intensity,
                                    color.blue * intensity);
    inverted_color = pack_argb_bytes(color.alpha * intensity, (1.0f - color.red) * intensity,
                                     (1.0f - color.green) * intensity, (1.0f - color.blue) * intensity);

    effect = rasterizer_screen_flash_effect;
    if (effect == 0) {

        render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
        return;
    }
    {
        set_render_state(halo::d3d9::rs::cull_mode, 3);
        set_render_state(halo::d3d9::rs::color_write_enable, 7);
        set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
        set_render_state(halo::d3d9::rs::fog_enable, 0);

        switch (flash->type) {
        case 1:
            set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::one);
            set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::inv_src_alpha);
            set_render_state(halo::d3d9::rs::blend_op, 1);
            set_render_state(halo::d3d9::rs::texture_factor, current_color);
            technique = screen_flash_techniques[0];
            render_device().effect_set_technique(effect, technique);
            break;

        case 2:
            set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::one);
            set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
            set_render_state(halo::d3d9::rs::blend_op, 3);
            set_render_state(halo::d3d9::rs::texture_factor, current_color);
            technique = screen_flash_techniques[1];
            render_device().effect_set_technique(effect, technique);
            break;

        case 3:
            if (halo::shell::globals().min_max_blend_op_is_broken != 0) {
                uint32_t clamped = pack_argb_bytes_clamped_alpha(color, intensity);
                set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::src_alpha);
                set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::inv_src_alpha);
                set_render_state(halo::d3d9::rs::blend_op, 1);
                set_render_state(halo::d3d9::rs::texture_factor, clamped);
                technique = screen_flash_techniques[0];
            } else if ((rasterizer_caps.src_blend_caps & k_blend_cap_blend_factor) != 0) {
                set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::inv_dest_color);
                set_render_state(halo::d3d9::rs::dest_blend, 0xf);
                set_render_state(halo::d3d9::rs::blend_op, 5);
                set_render_state(0xc1, current_color);
                set_render_state(halo::d3d9::rs::texture_factor, current_color);
                technique = screen_flash_techniques[2];
            } else {
                set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::inv_dest_color);
                set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
                set_render_state(halo::d3d9::rs::blend_op, 5);
                set_render_state(halo::d3d9::rs::texture_factor, current_color);
                technique = screen_flash_techniques[2];
            }
            render_device().effect_set_technique(effect, technique);
            break;

        case 4:
            if (halo::shell::globals().min_max_blend_op_is_broken != 0) {
                uint32_t clamped = pack_argb_bytes_clamped_alpha(color, intensity);
                set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::src_alpha);
                set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::inv_src_alpha);
                set_render_state(halo::d3d9::rs::blend_op, 1);
                set_render_state(halo::d3d9::rs::texture_factor, clamped);
                technique = screen_flash_techniques[0];
            } else if ((rasterizer_caps.src_blend_caps & k_blend_cap_blend_factor) != 0) {
                set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::inv_dest_color);
                set_render_state(halo::d3d9::rs::dest_blend, 0xf);
                set_render_state(halo::d3d9::rs::blend_op, 4);
                set_render_state(0xc1, current_color);
                set_render_state(halo::d3d9::rs::texture_factor, current_color);

            } else {
                set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::inv_dest_color);
                set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
                set_render_state(halo::d3d9::rs::blend_op, 4);
            }
            set_render_state(halo::d3d9::rs::texture_factor, current_color);
            technique = screen_flash_techniques[3];
            render_device().effect_set_technique(effect, technique);
            break;

        case 5:
            if ((rasterizer_caps.src_blend_caps & k_blend_cap_blend_factor) != 0) {
                set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::inv_dest_color);
                set_render_state(halo::d3d9::rs::dest_blend, 0xf);
                set_render_state(halo::d3d9::rs::blend_op, 1);
                set_render_state(0xc1, current_color);
            } else {
                set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::inv_dest_color);
                set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
                set_render_state(halo::d3d9::rs::blend_op, 1);
            }
            set_render_state(halo::d3d9::rs::texture_factor, current_color);
            technique = screen_flash_techniques[4];
            render_device().effect_set_technique(effect, technique);
            break;

        case 6:
        default:
            if (flash->type == 6) {
                if ((rasterizer_caps.src_blend_caps & k_blend_cap_blend_factor) != 0) {
                    set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::one);
                    set_render_state(halo::d3d9::rs::dest_blend, 0xf);
                    set_render_state(halo::d3d9::rs::blend_op, 1);
                    set_render_state(0xc1, inverted_color);
                } else {
                    set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::one);
                    set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
                    set_render_state(halo::d3d9::rs::blend_op, 1);
                }
                set_render_state(halo::d3d9::rs::texture_factor, inverted_color);
                technique = screen_flash_techniques[5];
                render_device().effect_set_technique(effect, technique);
            }
            break;
        }
    }

    set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
    set_render_state(halo::d3d9::rs::z_enable, 0);

    render_device().set_vertex_declaration(rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].declaration);

    {
        uint32_t usage = rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].usage;
        render_device().set_software_vertex_processing((rasterizer_software_vertex_processing != 0 ? 0x10u : 0u) | (usage & 0x10));
    }

    render_device().set_vertex_shader(rasterizer_vertex_shaders[35].shader);

    width = (int16_t)(rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left);
    height = (int16_t)(rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top);
    {
        float inv_w = 1.0f / (float)width;
        float inv_h = 1.0f / (float)height;
        constants[0][0] = inv_w + inv_w;
        constants[0][1] = 0.0f;
        constants[0][2] = 0.0f;
        constants[0][3] = -1.0f - inv_w;
        constants[1][0] = 0.0f;
        constants[1][1] = -2.0f * inv_h;
        constants[1][2] = 0.0f;
        constants[1][3] = 1.0f + inv_h;
        constants[2][0] = 0.0f;
        constants[2][1] = 0.0f;
        constants[2][2] = 0.0f;
        constants[2][3] = 0.5f;
        constants[3][0] = 0.0f;
        constants[3][1] = 0.0f;
        constants[3][2] = 0.0f;
        constants[3][3] = 1.0f;
        constants[4][0] = 0.0f;
        constants[4][1] = 0.0f;
        constants[4][2] = 0.0f;
        constants[4][3] = 1.0f;
    }
    render_device().set_vertex_shader_constant_f(0xd, &constants[0][0], 5);

    width = (int16_t)(rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left);
    height = (int16_t)(rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top);
    neg_one_bits.bits = halo::k_dword_none;
    {
        float w = (float)width;
        float h = (float)height;
        float n1 = neg_one_bits.f;
        vs_quad[0][0] = 0.0f; vs_quad[0][1] = 0.0f; vs_quad[0][2] = 0.0f; vs_quad[0][3] = n1; vs_quad[0][4] = 0.0f; vs_quad[0][5] = 0.0f;
        vs_quad[1][0] = w;    vs_quad[1][1] = 0.0f; vs_quad[1][2] = 0.0f; vs_quad[1][3] = n1; vs_quad[1][4] = 1.0f; vs_quad[1][5] = 0.0f;
        vs_quad[2][0] = w;    vs_quad[2][1] = h;    vs_quad[2][2] = 0.0f; vs_quad[2][3] = n1; vs_quad[2][4] = 1.0f; vs_quad[2][5] = 1.0f;
        vs_quad[3][0] = 0.0f; vs_quad[3][1] = h;    vs_quad[3][2] = 0.0f; vs_quad[3][3] = n1; vs_quad[3][4] = 0.0f; vs_quad[3][5] = 1.0f;
    }

    ps_constants[0][0] = color.red * intensity;
    ps_constants[0][1] = color.green * intensity;
    ps_constants[0][2] = color.blue * intensity;
    ps_constants[0][3] = color.alpha * intensity;
    ps_constants[1][0] = (1.0f - color.red) * intensity;
    ps_constants[1][1] = (1.0f - color.green) * intensity;
    ps_constants[1][2] = (1.0f - color.blue) * intensity;
    ps_constants[1][3] = color.alpha * intensity;
    if (rasterizer_caps.pixel_shader_version > halo::d3d9::k_pixel_shader_version_1_0) {
        render_device().set_pixel_shader_constant_f(0, &ps_constants[0][0], 2);
    }

    effect = rasterizer_screen_flash_effect;
    pass_count = 0;
    render_device().effect_begin(effect, &pass_count, 3);
    for (pass = 0; pass < pass_count; pass++) {
        render_device().effect_pass(effect, 0);

        render_device().draw_primitive_up(6, 2, &vs_quad[0][0], 0x18);
    }
    render_device().effect_end(effect);

    render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
}

}  // namespace rasterizer_screen_flash_render_impl

namespace rasterizer_sun_glow_blur_impl {


static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_quad_vertex(int i, float x, float y, float u, float v)
{
    rasterizer_shadow_screen_quad[i].x = x;
    rasterizer_shadow_screen_quad[i].y = y;
    rasterizer_shadow_screen_quad[i].z = 0.0f;
    rasterizer_shadow_screen_quad[i].color = halo::d3d9::k_color_white;
    rasterizer_shadow_screen_quad[i].u = u;
    rasterizer_shadow_screen_quad[i].v = v;
}

/**
 * Direct3D 9 back end function rasterizer_sun_glow_blur.
 *
 * @address 0x525720
 */
int16_t rasterizer_sun_glow_blur(int16_t first, int16_t second, int16_t passes)
{
    int16_t pass;
    void *effect = rasterizer_effects[76].effect;

    if (effect == NULL || passes <= 0) {
        return (passes & 1) ? second : first;
    }
    set_render_state(halo::d3d9::rs::cull_mode, 3);
    set_render_state(halo::d3d9::rs::color_write_enable, 7);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::dest_alpha);
    set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::zero);
    set_render_state(halo::d3d9::rs::blend_op, 1);
    set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
    set_render_state(halo::d3d9::rs::z_enable, 0);
    set_render_state(halo::d3d9::rs::fog_enable, 0);
    render_device().set_vertex_declaration(rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].declaration);
    render_device().set_software_vertex_processing(((rasterizer_software_vertex_processing ? 0x10 : 0) |
                                                rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].usage) & 0x10);
    render_device().set_vertex_shader(rasterizer_vertex_shaders[0].shader);
    render_device().set_vertex_shader_constant_f(0xd, &rasterizer_sun_glow_blur_offsets[0][0], 8);

    for (pass = 0; pass < passes; pass++) {
        int16_t source = (pass & 1) ? second : first;
        int16_t destination = (pass & 1) ? first : second;
        float weight[4];
        uint32_t stage;
        uint32_t effect_passes;

        for (stage = 0; stage < 4; stage++) {
            void *texture = (source < 9 && source >= 0) ? rasterizer_render_targets[source].texture : NULL;

            render_device().set_texture(stage, texture);
            render_device().set_sampler_state(stage, halo::d3d9::ss::mip_filter, 1);
        }
        rasterizer_render_target_set_active(destination, 0, 0);
        set_quad_vertex(0, -1.015625f, 1.015625f, 0.0f, 0.0f);
        set_quad_vertex(1, 0.984375f, 1.015625f, 1.0f, 0.0f);
        set_quad_vertex(2, 0.984375f, -0.984375f, 1.0f, 1.0f);
        set_quad_vertex(3, -1.015625f, -0.984375f, 0.0f, 1.0f);
        weight[0] = weight[1] = weight[2] = weight[3] = (pass > 0) ? 0.5f : 1.0f;
        render_device().set_pixel_shader_constant_f(0, weight, 1);
        effect = rasterizer_effects[76].effect;
        render_device().effect_begin(effect, &effect_passes, 3);
        effect = rasterizer_effects[76].effect;
        render_device().effect_pass(effect, 1);
        render_device().draw_primitive_up(6, 2, rasterizer_shadow_screen_quad, sizeof(rasterizer_dynamic_screen_vertex));
        effect = rasterizer_effects[76].effect;
        render_device().effect_end(effect);
    }
    rasterizer_render_target_set_active(rasterizer_window.type, 0, 0);
    render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
    return (passes & 1) ? second : first;
}

}  // namespace rasterizer_sun_glow_blur_impl

namespace rasterizer_sun_glow_capture_impl {


static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_quad_vertex(int i, float x, float y, float u, float v)
{
    rasterizer_shadow_screen_quad[i].x = x;
    rasterizer_shadow_screen_quad[i].y = y;
    rasterizer_shadow_screen_quad[i].z = 0.0f;
    rasterizer_shadow_screen_quad[i].color = halo::d3d9::k_color_white;
    rasterizer_shadow_screen_quad[i].u = u;
    rasterizer_shadow_screen_quad[i].v = v;
}

/**
 * Direct3D 9 back end function rasterizer_sun_glow_capture.
 *
 * @address 0x525320
 */
void rasterizer_sun_glow_capture(const float *rect, int16_t target_index)
{
    float constants[8][4];
    float width, height;
    void *effect;
    int i, j;

    render_device().set_texture(0, rasterizer_render_targets[1].texture);
    render_device().set_sampler_state(0, halo::d3d9::ss::mip_filter, 1);
    chimera__rasterizer_set_texture_direct_d3d9(halo::tag_id_bits(rasterizer_globals_data->glow.tag_id), 1, 0);
    set_render_state(halo::d3d9::rs::cull_mode, 3);
    set_render_state(halo::d3d9::rs::color_write_enable, 0xf);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
    set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
    set_render_state(halo::d3d9::rs::z_enable, 0);
    set_render_state(halo::d3d9::rs::fog_enable, 0);
    render_device().set_vertex_declaration(rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].declaration);
    render_device().set_software_vertex_processing(((rasterizer_software_vertex_processing ? 0x10 : 0) |
                                                rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].usage) & 0x10);
    render_device().set_vertex_shader(rasterizer_vertex_shaders[0].shader);

    width = (float)(int16_t)(rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left);
    height = (float)(int16_t)(rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top);
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 4; j++) {
            constants[i][j] = 0.0f;
        }
    }
    for (i = 2; i < 8; i++) {
        constants[i][i & 1] = 1.0f;
    }
    constants[0][0] = (rect[1] - rect[0]) / width;
    constants[0][3] = rect[0] / width;
    constants[1][1] = (rect[3] - rect[2]) / height;
    constants[1][3] = rect[2] / height;
    if (halo::shell::globals().linear_texture_addressing_sun) {
        constants[0][0] *= width;
        constants[0][3] = width * constants[0][3];
        constants[1][1] = height * constants[1][1];
        constants[1][3] = height * constants[1][3];
    }
    render_device().set_vertex_shader_constant_f(0xd, &constants[0][0], 8);
    rasterizer_render_target_set_active(target_index, 0, 0);

    effect = rasterizer_effects[76].effect;
    if (effect != NULL) {
        uint32_t passes;

        set_quad_vertex(0, -1.015625f, 1.015625f, 0.0f, 0.0f);
        set_quad_vertex(1, 0.984375f, 1.015625f, 1.0f, 0.0f);
        set_quad_vertex(2, 0.984375f, -0.984375f, 1.0f, 1.0f);
        set_quad_vertex(3, -1.015625f, -0.984375f, 0.0f, 1.0f);
        render_device().effect_begin(effect, &passes, 3);
        effect = rasterizer_effects[76].effect;
        render_device().effect_pass(effect, 0);
        render_device().draw_primitive_up(6, 2, rasterizer_shadow_screen_quad, sizeof(rasterizer_dynamic_screen_vertex));
        effect = rasterizer_effects[76].effect;
        render_device().effect_end(effect);
    }
    rasterizer_render_target_set_active(rasterizer_window.type, 0, 0);
    render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
}

}  // namespace rasterizer_sun_glow_capture_impl

/**
 * Projects a world-space point and radius into screen space for the projected dynamic-light/ shadow renderer.
 * Returns 0 (without writing the outputs) when the radius is non-positive or the point lies behind the light's
 * near plane (projected w <= 0).
 *
 * @address 0x525130
 */
uint8_t rasterizer_sun_glow_project_point(real_point3d *point, float radius, float *out_screen, float *out_scale)
{
    int16_t viewport_width;
    int16_t viewport_height;
    real_point3d view;
    float proj_y, proj_w, proj_x0, proj_scale_x0, proj_scale_y0;
    float inv_w;

    if (radius <= 0.0f) {
        return 0;
    }

    viewport_width = rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left;
    viewport_height = rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top;

    halo::math::matrix4x3_transform_point(view, *point, rasterizer_window.frustum.world_to_view);

    proj_y = rasterizer_window.frustum.projection[0][1] * view.x +
             rasterizer_window.frustum.projection[1][1] * view.y +
             rasterizer_window.frustum.projection[2][1] * view.z +
             rasterizer_window.frustum.projection[3][1];
    proj_w = rasterizer_window.frustum.projection[0][2] * view.x +
             rasterizer_window.frustum.projection[1][2] * view.y +
             rasterizer_window.frustum.projection[2][2] * view.z +
             rasterizer_window.frustum.projection[3][2];
    proj_scale_x0 = rasterizer_window.frustum.projection[0][0] * radius;
    proj_scale_y0 = rasterizer_window.frustum.projection[1][1] * radius;

    if (0.0f < proj_w) {
        float proj_depth;
        float scale_y_radius;

        inv_w = 1.0f / (rasterizer_window.frustum.projection[0][3] * view.x +
                          rasterizer_window.frustum.projection[1][3] * view.y +
                          rasterizer_window.frustum.projection[2][3] * view.z +
                          rasterizer_window.frustum.projection[3][3]);

        proj_x0 = rasterizer_window.frustum.projection[0][0] * view.x +
                   rasterizer_window.frustum.projection[1][0] * view.y +
                   rasterizer_window.frustum.projection[2][0] * view.z +
                   rasterizer_window.frustum.projection[3][0];
        out_screen[0] = ((proj_x0 * inv_w + 1.0f) * (float)viewport_width - 1.0f) * 0.5f;

        out_screen[1] = ((1.0f - inv_w * proj_y) * (float)viewport_height - 1.0f) * 0.5f;

        proj_depth = inv_w * proj_w;
        if (1.0f <= proj_depth) {
            proj_depth = 1.0f;
        }
        out_screen[2] = proj_depth;

        scale_y_radius = proj_scale_y0;
        out_scale[0] = (float)viewport_width * inv_w * proj_scale_x0 * 0.5f;
        out_scale[1] = (float)viewport_height * inv_w * scale_y_radius * 0.5f;
        return 1;
    }
    return 0;
}

namespace rasterizer_sun_glow_render_impl {


static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

static void set_sampler_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(sampler, type, value);
}

static void set_clamped_linear_sampler(uint32_t sampler)
{
    set_sampler_state(sampler, halo::d3d9::ss::address_u, 3);
    set_sampler_state(sampler, halo::d3d9::ss::address_v, 3);
    set_sampler_state(sampler, halo::d3d9::ss::mag_filter, 2);
    set_sampler_state(sampler, halo::d3d9::ss::min_filter, 2);
    set_sampler_state(sampler, halo::d3d9::ss::mip_filter, 2);
}

static void draw_quad(void)
{
    render_device().draw_primitive_up(6, 2, rasterizer_shadow_screen_quad, sizeof(rasterizer_dynamic_screen_vertex));
}

static void set_quad_position(float left, float top, float right, float bottom, float z)
{
    rasterizer_shadow_screen_quad[0].x = left;
    rasterizer_shadow_screen_quad[0].y = top;
    rasterizer_shadow_screen_quad[1].x = right;
    rasterizer_shadow_screen_quad[1].y = top;
    rasterizer_shadow_screen_quad[2].x = right;
    rasterizer_shadow_screen_quad[2].y = bottom;
    rasterizer_shadow_screen_quad[3].x = left;
    rasterizer_shadow_screen_quad[3].y = bottom;
    rasterizer_shadow_screen_quad[0].z = z;
    rasterizer_shadow_screen_quad[1].z = z;
    rasterizer_shadow_screen_quad[2].z = z;
    rasterizer_shadow_screen_quad[3].z = z;
}

static void set_quad_color_and_uv(uint32_t color)
{
    rasterizer_shadow_screen_quad[0].color = color;
    rasterizer_shadow_screen_quad[0].u = 0.0f;
    rasterizer_shadow_screen_quad[0].v = 0.0f;
    rasterizer_shadow_screen_quad[1].color = color;
    rasterizer_shadow_screen_quad[1].u = 1.0f;
    rasterizer_shadow_screen_quad[1].v = 0.0f;
    rasterizer_shadow_screen_quad[2].color = color;
    rasterizer_shadow_screen_quad[2].u = 1.0f;
    rasterizer_shadow_screen_quad[2].v = 1.0f;
    rasterizer_shadow_screen_quad[3].color = color;
    rasterizer_shadow_screen_quad[3].u = 0.0f;
    rasterizer_shadow_screen_quad[3].v = 1.0f;
}

static void set_screen_constants(float c15_z, float c15_w, float c16_w, float c17_x, float c17_y)
{
    float constants[5][4];
    float inverse_width = 1.0f / (float)(int16_t)(rasterizer_window.camera.viewport_bounds.right -
                                                  rasterizer_window.camera.viewport_bounds.left);
    float inverse_height = 1.0f / (float)(int16_t)(rasterizer_window.camera.viewport_bounds.bottom -
                                                   rasterizer_window.camera.viewport_bounds.top);

    constants[0][0] = inverse_width + inverse_width;
    constants[0][1] = 0.0f;
    constants[0][2] = 0.0f;
    constants[0][3] = -1.0f - inverse_width;
    constants[1][0] = 0.0f;
    constants[1][1] = -2.0f * inverse_height;
    constants[1][2] = 0.0f;
    constants[1][3] = inverse_height + 1.0f;
    constants[2][0] = 0.0f;
    constants[2][1] = 0.0f;
    constants[2][2] = c15_z;
    constants[2][3] = c15_w;
    constants[3][0] = 0.0f;
    constants[3][1] = 0.0f;
    constants[3][2] = 0.0f;
    constants[3][3] = c16_w;
    constants[4][0] = c17_x;
    constants[4][1] = c17_y;
    constants[4][2] = 0.0f;
    constants[4][3] = 1.0f;
    render_device().set_vertex_shader_constant_f(0xd, &constants[0][0], 5);
}

static void set_screen_vertex_states(void)
{
    render_device().set_vertex_declaration(rasterizer_vertex_declarations[6].declaration);
    render_device().set_software_vertex_processing(((rasterizer_software_vertex_processing ? 0x10 : 0) |
                                                rasterizer_vertex_declarations[6].usage) & 0x10);
    render_device().set_vertex_shader(rasterizer_vertex_shaders[24].shader);
}

/**
 * Direct3D 9 back end function rasterizer_sun_glow_render.
 *
 * @address 0x525ab0
 */
void rasterizer_sun_glow_render(lens_flare_instance *instance)
{
    real_vector3d to_flare;
    real_vector3d direction;
    real_point3d point;
    float screen[3];
    float scale;
    float rect[4];
    float falloff;
    float cone_cosine;
    float radius;
    void *effect;
    int32_t target;
    int32_t quad;

    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
        return;
    }
    set_clamped_linear_sampler(0);

    to_flare.i = instance->position.x - rasterizer_window.camera.position.x;
    to_flare.j = instance->position.y - rasterizer_window.camera.position.y;
    to_flare.k = instance->position.z - rasterizer_window.camera.position.z;
    halo::math::vector3d_normalize_with_length(to_flare);
    cone_cosine = (float)halo::libm::cos(0.7853981852531433);
    falloff = (to_flare.k * rasterizer_window.camera.forward.k + to_flare.j * rasterizer_window.camera.forward.j +
               to_flare.i * rasterizer_window.camera.forward.i - cone_cosine) / (1.0f - cone_cosine);
    if (falloff < 0.0f) {
        falloff = 0.0f;
    } else if (falloff > 1.0f) {
        falloff = 1.0f;
    }
    set_screen_constants(1.0f, 0.0f, 1.0f, 0.0f, 0.0f);

    vector3d_unpack_normal_11_11_10(&direction, instance->packed_direction);
    radius = reinterpret_cast<const LensFlare *>(static_cast<uintptr_t>(instance->definition))->occlusion_radius;
    point.x = direction.i * radius + instance->position.x;
    point.y = direction.j * radius + instance->position.y;
    point.z = direction.k * radius + instance->position.z;
    if (!rasterizer_sun_glow_project_point(&point, radius, screen, &scale)) {
        render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
        return;
    }
    screen[0] = (float)halo::libm::floor(screen[0] + 0.5f);
    screen[1] = (float)halo::libm::floor(screen[1] + 0.5f);
    rect[0] = screen[0] - 32.0f;
    rect[2] = screen[1] - 32.0f;
    rect[1] = screen[0] + 32.0f;
    rect[3] = screen[1] + 32.0f;

    set_clamped_linear_sampler(1);
    set_clamped_linear_sampler(2);
    set_clamped_linear_sampler(3);
    set_screen_vertex_states();
    render_device().set_pixel_shader(0);

    set_render_state(halo::d3d9::rs::cull_mode, 3);
    set_render_state(halo::d3d9::rs::color_write_enable, 8);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
    set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
    set_render_state(halo::d3d9::rs::z_enable, 0);
    set_render_state(halo::d3d9::rs::fog_enable, 0);
    set_quad_position(rect[0], rect[2], rect[1], rect[3], 0.0f);
    set_render_state(halo::d3d9::rs::texture_factor, 0);
    set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::tfactor);
    set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
    set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
    draw_quad();

    chimera__rasterizer_set_texture_direct_d3d9(halo::tag_id_bits(rasterizer_globals_data->glow.tag_id), 0, 0);
    set_render_state(halo::d3d9::rs::cull_mode, 3);
    set_render_state(halo::d3d9::rs::color_write_enable, 8);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
    set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
    set_render_state(halo::d3d9::rs::z_enable, 1);
    set_render_state(halo::d3d9::rs::z_func, 4);
    set_render_state(halo::d3d9::rs::z_write_enable, 0);
    set_render_state(halo::d3d9::rs::fog_enable, 0);
    set_quad_position(rect[0], rect[2], rect[1], rect[3], screen[2]);
    set_quad_color_and_uv(halo::d3d9::k_color_white);
    set_render_state(halo::d3d9::rs::texture_factor, halo::d3d9::k_color_white);
    set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::select_arg1);
    set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture | halo::d3d9::ta::alpha_replicate);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::modulate4x);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
    set_texture_stage_state(0, halo::d3d9::ts::alpha_arg2, halo::d3d9::ta::tfactor);
    set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
    set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
    draw_quad();

    effect = rasterizer_effects[77].effect;
    if (effect != NULL) {
        uint32_t passes;
        uint32_t pass;

        rasterizer_sun_glow_capture(rect, 6);
        rasterizer_sun_glow_capture(rect, 7);
        target = rasterizer_sun_glow_blur(6, 7, 4);
        set_screen_constants(0.0f, 0.5f, 1.0f, 1.0f, 1.0f);
        set_screen_vertex_states();
        rasterizer_render_target_bind_texture_stage((int16_t)target, 0);
        set_sampler_state(0, halo::d3d9::ss::mip_filter, 2);
        set_render_state(halo::d3d9::rs::cull_mode, 3);
        set_render_state(halo::d3d9::rs::color_write_enable, 7);
        set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
        set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::src_alpha);
        set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
        set_render_state(halo::d3d9::rs::blend_op, 1);
        set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
        set_render_state(halo::d3d9::rs::z_enable, 0);
        set_render_state(halo::d3d9::rs::fog_enable, 0);

        for (quad = 0; quad < 16; quad++) {
            float grow = (float)quad * 0.0625f * 80.0f - 4.0f;
            uint32_t alpha = (uint32_t)(int32_t)(falloff / (float)(quad + 1) * 255.0f);

            set_quad_color_and_uv(halo::d3d9::color_argb(alpha, 0xff, 0xff, 0xff));
            set_quad_position(rect[0] - grow, rect[2] - grow, rect[1] + grow, rect[3] + grow, 0.0f);
            effect = rasterizer_effects[77].effect;
            render_device().effect_begin(effect, &passes, 3);
            for (pass = 0; pass < passes; pass++) {
                effect = rasterizer_effects[77].effect;
                render_device().effect_pass(effect, pass);
                draw_quad();
            }
            effect = rasterizer_effects[77].effect;
            render_device().effect_end(effect);
        }
    }
    render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
}

}  // namespace rasterizer_sun_glow_render_impl

namespace rasterizer_ui_quad_draw_impl {


static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_texture_stage_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(stage, type, value);
}

static void set_sampler_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(stage, type, value);
}

static void set_texture(uint32_t stage, uint32_t texture)
{
    render_device().set_texture(stage, texture);
}

static void draw_quad(const hud_quad_vertex *vertices)
{
    render_device().draw_primitive_up(6, 2, vertices, sizeof(hud_quad_vertex));
}

static uint32_t pack_argb(float alpha, float red, float green, float blue)
{
    uint32_t value = (uint32_t)(int32_t)(red * 255.0f) & 0xff;

    value = (value | ((uint32_t)(int32_t)(alpha * 255.0f) << 8)) << 8;
    value = (value | ((uint32_t)(int32_t)(green * 255.0f) & 0xff)) << 8;
    return value | ((uint32_t)(int32_t)(blue * 255.0f) & 0xff);
}

/**
 * Direct3D 9 back end function rasterizer_ui_quad_draw.
 *
 * @address 0x51c9a0
 */
void rasterizer_ui_quad_draw(ui_quad_render_state *state, hud_quad_vertex *vertices)
{
    const uint8_t *axis_flags = (const uint8_t *)&state->unknown_08;
    rasterizer_effect_slot *slot = NULL;
    uint8_t ok = 1;
    float screen[5][4];
    float maps[6][4];
    float pixel[6][4];
    float offset_x, offset_y;
    int16_t width, height;
    int16_t stage;

    if (text_rendering_enabled == 0 || rasterizer_window.type != 1) {
        return;
    }
    set_render_state(halo::d3d9::rs::cull_mode, 1);
    set_render_state(halo::d3d9::rs::color_write_enable, 7);
    set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
    set_render_state(halo::d3d9::rs::alpha_test_enable, 1);
    set_render_state(halo::d3d9::rs::alpha_ref, 1);
    set_render_state(halo::d3d9::rs::z_enable, 0);
    set_render_state(halo::d3d9::rs::fog_enable, 0);
    chimera__rasterizer_set_framebuffer_blend_function(state->framebuffer_blend_function);
    if (render_device().set_vertex_declaration(rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].declaration) < 0) {
        ok = 0;
    }
    if (render_device().set_software_vertex_processing(((rasterizer_software_vertex_processing ? 0x10 : 0) |
             rasterizer_vertex_declarations[_rasterizer_vertex_type_dynamic_screen].usage) & 0x10) < 0) {
        ok = 0;
    }
    if (render_device().set_vertex_shader(rasterizer_vertex_shaders[36].shader) < 0) {
        ok = 0;
    }

    width = (int16_t)(rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left);
    height = (int16_t)(rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top);
    offset_x = (state->geometry_offset != NULL) ? state->geometry_offset[0] * 0.003125f : 0.0f;
    offset_y = (state->geometry_offset != NULL) ? state->geometry_offset[1] * -0.004166667f : 0.0f;

    screen[0][0] = 0.003125f; screen[0][1] = 0.0f; screen[0][2] = 0.0f;
    screen[0][3] = offset_x - (1.0f / (float)width + 1.0f);
    screen[1][0] = 0.0f; screen[1][1] = -0.004166667f; screen[1][2] = 0.0f;
    screen[1][3] = 1.0f / (float)height + offset_y + 1.0f;
    screen[2][0] = 0.0f; screen[2][1] = 0.0f; screen[2][2] = 0.0f; screen[2][3] = 0.5f;
    screen[3][0] = 0.0f; screen[3][1] = 0.0f; screen[3][2] = 0.0f; screen[3][3] = 1.0f;
    screen[4][0] = state->map_texel_scales[0].x;
    screen[4][1] = state->map_texel_scales[0].y;
    screen[4][2] = 0.0f;
    screen[4][3] = 1.0f;

    maps[0][0] = state->map_texel_scales[1].x;
    maps[0][1] = state->map_texel_scales[1].y;
    maps[0][2] = state->map_texel_scales[2].x;
    maps[0][3] = state->map_texel_scales[2].y;
    maps[1][0] = axis_flags[0] ? 1.0f : 0.0f;
    maps[1][1] = axis_flags[0] ? 0.0f : 1.0f;
    maps[1][2] = axis_flags[1] ? 1.0f : 0.0f;
    maps[1][3] = axis_flags[1] ? 0.0f : 1.0f;
    maps[2][0] = axis_flags[2] ? 1.0f : 0.0f;
    maps[2][1] = axis_flags[2] ? 0.0f : 1.0f;
    maps[2][2] = (state->map_offsets[0] != NULL) ? state->map_offsets[0]->x : 0.0f;
    maps[2][3] = (state->map_offsets[0] != NULL) ? state->map_offsets[0]->y : 0.0f;
    maps[3][0] = (state->map_offsets[1] != NULL) ? state->map_offsets[1]->x : 0.0f;
    maps[3][1] = (state->map_offsets[1] != NULL) ? state->map_offsets[1]->y : 0.0f;
    maps[3][2] = (state->map_offsets[2] != NULL) ? state->map_offsets[2]->x : 0.0f;
    maps[3][3] = (state->map_offsets[2] != NULL) ? state->map_offsets[2]->y : 0.0f;
    maps[4][0] = state->map_scales[0].x;
    maps[4][1] = state->map_scales[0].y;
    maps[4][2] = state->map_scales[1].x;
    maps[4][3] = state->map_scales[1].y;
    maps[5][0] = state->map_scales[2].x;
    maps[5][1] = state->map_scales[2].y;
    maps[5][2] = 0.0f;
    maps[5][3] = 0.0f;
    if (render_device().set_vertex_shader_constant_f(0xd, &screen[0][0], 5) < 0) {
        ok = 0;
    }
    if (render_device().set_vertex_shader_constant_f(0x12, &maps[0][0], 6) < 0) {
        ok = 0;
    }

    for (stage = 0; stage < 3 && state->maps[stage] != NULL; stage++) {
        uint32_t address = state->wrap_modes[stage] ? 1 : 3;
        uint32_t filter = state->single_local_player ? 1 : 2;

        set_sampler_state(stage, halo::d3d9::ss::address_u, address);
        set_sampler_state(stage, halo::d3d9::ss::address_v, address);
        set_sampler_state(stage, halo::d3d9::ss::mag_filter, filter);
        set_sampler_state(stage, halo::d3d9::ss::min_filter, filter);
        set_sampler_state(stage, halo::d3d9::ss::mip_filter, filter);
    }

    if (state->meter_parameters != NULL) {
        const hud_meter_color_block *meter = static_cast<const hud_meter_color_block *>(state->meter_parameters);

        for (stage = 0; stage < 3 && state->maps[stage] != NULL; stage++) {
            halo::cache::texture_cache_get(state->maps[stage], 1, 1);
            set_texture(stage, state->maps[stage]->hardware_texture);
        }
        render_device().set_pixel_shader(0);
        set_render_state(halo::d3d9::rs::alpha_test_enable, 1);
        set_render_state(halo::d3d9::rs::alpha_func, 4);
        set_render_state(halo::d3d9::rs::alpha_ref, static_cast<uint8_t>(meter->primary >> 24));
        set_render_state(halo::d3d9::rs::color_write_enable, 0xf);
        set_render_state(halo::d3d9::rs::alpha_blend_enable, 1);
        set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::one);
        set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
        set_render_state(halo::d3d9::rs::blend_op, 1);
        set_render_state(halo::d3d9::rs::texture_factor, meter->primary);
        set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
        set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
        set_texture_stage_state(0, halo::d3d9::ts::color_arg2, halo::d3d9::ta::tfactor);
        set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
        set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
        set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
        set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
        draw_quad(vertices);
        set_render_state(halo::d3d9::rs::alpha_func, 5);
        set_render_state(halo::d3d9::rs::texture_factor, meter->empty);
        set_render_state(halo::d3d9::rs::color_write_enable, 0xf);
        set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::modulate2x);
        set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
        set_texture_stage_state(0, halo::d3d9::ts::color_arg2, halo::d3d9::ta::tfactor);
        set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
        set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
        set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
        set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
        draw_quad(vertices);
        render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
        return;
    }

    if (state->maps[0] != NULL) {
        const ColorRGB *tint0 = (state->map_tints[0] != NULL) ? state->map_tints[0] : global_white_color;
        float fade0 = (state->map_fades[0] != NULL) ? *state->map_fades[0] : 1.0f;
        int16_t effect_index = 0x31;

        set_sampler_state(0, halo::d3d9::ss::address_u, 3);
        set_sampler_state(0, halo::d3d9::ss::address_v, 3);
        if (state->maps[1] == NULL && state->maps[2] == NULL) {
            set_render_state(halo::d3d9::rs::texture_factor, pack_argb(fade0, tint0->red, tint0->green, tint0->blue));
            rasterizer_bind_texture_d3d9(0, state->maps[0]);
            set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
            set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
            set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
            set_texture_stage_state(0, halo::d3d9::ts::color_arg2, halo::d3d9::ta::tfactor);
            set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::modulate);
            set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
            set_texture_stage_state(0, halo::d3d9::ts::alpha_arg2, halo::d3d9::ta::diffuse);
            set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::modulate);
            set_texture_stage_state(1, halo::d3d9::ts::color_arg1, halo::d3d9::ta::current);
            set_texture_stage_state(1, halo::d3d9::ts::color_arg2, halo::d3d9::ta::diffuse);
            set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
            set_texture_stage_state(1, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::current);
            set_texture_stage_state(2, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
            set_texture_stage_state(2, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);
            render_device().set_pixel_shader(0);
            draw_quad(vertices);
            render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
            return;
        }
        if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
            render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
            return;
        }
        {
            const ColorRGB *tint1 = (state->map_tints[1] != NULL) ? state->map_tints[1] : global_white_color;
            const ColorRGB *tint2 = (state->map_tints[2] != NULL) ? state->map_tints[2] : global_white_color;
            const float *extra = reinterpret_cast<const float *>(state->unknown_64);

            pixel[0][0] = tint0->red; pixel[0][1] = tint0->green; pixel[0][2] = tint0->blue; pixel[0][3] = fade0;
            pixel[1][0] = tint1->red; pixel[1][1] = tint1->green; pixel[1][2] = tint1->blue;
            pixel[1][3] = (state->map_fades[1] != NULL) ? *state->map_fades[1] : 1.0f;
            pixel[2][0] = tint2->red; pixel[2][1] = tint2->green; pixel[2][2] = tint2->blue;
            pixel[2][3] = (state->map_fades[2] != NULL) ? *state->map_fades[2] : 1.0f;
            pixel[3][0] = extra[1]; pixel[3][1] = extra[2]; pixel[3][2] = extra[3]; pixel[3][3] = extra[0];
            pixel[4][0] = pixel[4][1] = pixel[4][2] = pixel[4][3] = 0.0f;
            pixel[5][0] = pixel[5][1] = pixel[5][2] = pixel[5][3] = 0.0f;
        }
        if (state->maps[1] != NULL) {
            switch (state->zero_to_one_blend) {
            case 1: effect_index = 0x3b; break;
            case 2: effect_index = 0x45; break;
            case 3: effect_index = 0x40; break;
            case 4: effect_index = 0x36; break;
            case 5: effect_index = 0; break;
            default: break;
            }
        }
        if (state->maps[2] != NULL) {
            switch (state->one_to_two_blend) {
            case 1: effect_index += 2; break;
            case 2: effect_index += 4; break;
            case 3: effect_index += 3; break;
            case 4: effect_index += 1; break;
            default: break;
            }
        }
        slot = &rasterizer_effects[effect_index];
    }

    if (ok && slot != NULL && slot->effect != 0) {
        void *effect = slot->effect;
        uint32_t passes;
        uint32_t pass;

        for (stage = 0; stage < 3 && state->maps[stage] != NULL; stage++) {
            halo::cache::texture_cache_get(state->maps[stage], 1, 1);
            render_device().effect_set_texture(effect, slot->texture_handles[stage], state->maps[stage]->hardware_texture);
        }
        if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
            set_render_state(halo::d3d9::rs::texture_factor, pack_argb(pixel[0][3], pixel[0][0], pixel[0][1], pixel[0][2]));
        } else {
            render_device().set_pixel_shader_constant_f(0, &pixel[0][0], 6);
        }
        render_device().effect_begin(effect, &passes, 3);
        for (pass = 0; pass < passes; pass++) {
            render_device().effect_pass(effect, pass);
            draw_quad(vertices);
        }
        render_device().effect_end(effect);
    }
    render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
}

}  // namespace rasterizer_ui_quad_draw_impl

/**
 * Direct3D 9 back end function rasterizer_underwater_tint_jitter_update.
 *
 * Registers: EAX = lightmap
 *
 * @address 0x51f310
 */
void rasterizer_underwater_tint_jitter_update(BitmapData *lightmap)
{
    uint32_t state;

    if (console_debug_toggle_6893f1 == 0) {
        return;
    }
    rasterizer_environment_lightmap = lightmap;
    if (render_force_flag <= 0) {
        return;
    }

    if (render_force_flag == 2) {
        rasterizer_underwater_tint_jitter_b = halo::rasterizer::fields::underwater_tint_jitter_forced_value;
        rasterizer_underwater_tint_jitter_g = halo::rasterizer::fields::underwater_tint_jitter_forced_value;
        rasterizer_underwater_tint_jitter_r = halo::rasterizer::fields::underwater_tint_jitter_forced_value;
        return;
    }

    state = halo::advance_random_seed(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(lightmap)));
    rasterizer_underwater_tint_jitter_r = (float)(state >> halo::k_random_high_shift) * halo::k_unit_word_scale;
    state = halo::advance_random_seed(state);
    rasterizer_underwater_tint_jitter_g = (float)(state >> halo::k_random_high_shift) * halo::k_unit_word_scale;
    state = halo::advance_random_seed(state);
    rasterizer_underwater_tint_jitter_b = (float)(state >> halo::k_random_high_shift) * halo::k_unit_word_scale;
}

namespace rasterizer_underwater_tint_set_states_impl {


/**
 * Direct3D 9 back end function rasterizer_underwater_tint_set_states.
 *
 * @address 0x51f030
 */
void rasterizer_underwater_tint_set_states(void)
{
    uint32_t stage7_index;

    if (console_debug_toggle_6893f1 == 0) {
        return;
    }

    render_device().set_render_state(halo::d3d9::rs::cull_mode, 3);
    render_device().set_render_state(halo::d3d9::rs::color_write_enable, 0xf);
    render_device().set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
    render_device().set_render_state(halo::d3d9::rs::alpha_ref, 0x7f);
    render_device().set_render_state(halo::d3d9::rs::z_enable, 1);
    render_device().set_render_state(halo::d3d9::rs::z_func, 4);
    render_device().set_render_state(halo::d3d9::rs::z_write_enable, 1);


    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
        render_device().set_material(rasterizer_underwater_material);

        render_device().set_render_state(halo::d3d9::rs::lighting, 1);
        render_device().set_render_state(halo::d3d9::rs::fog_enable, rasterizer_fog_enabled);
        render_device().set_render_state(halo::d3d9::rs::fog_color, halo::d3d9::k_color_white);
        render_device().set_render_state(halo::d3d9::rs::ambient, halo::rasterizer::fields::fixed_function_ambient_color);

        render_device().set_sampler_state(1, halo::d3d9::ss::address_u, 3);
        render_device().set_sampler_state(1, halo::d3d9::ss::address_v, 3);
        render_device().set_sampler_state(1, halo::d3d9::ss::mag_filter, 2);
        render_device().set_sampler_state(1, halo::d3d9::ss::min_filter, 2);
        stage7_index = 1;
    } else {
        render_device().set_render_state(halo::d3d9::rs::fog_enable, 0);
        render_device().set_sampler_state(1, halo::d3d9::ss::address_u, 1);
        render_device().set_sampler_state(1, halo::d3d9::ss::address_v, 1);
        render_device().set_sampler_state(2, halo::d3d9::ss::address_u, 3);
        render_device().set_sampler_state(2, halo::d3d9::ss::address_v, 3);
        render_device().set_sampler_state(2, halo::d3d9::ss::mag_filter, 2);
        render_device().set_sampler_state(2, halo::d3d9::ss::min_filter, 2);
        render_device().set_sampler_state(2, halo::d3d9::ss::mip_filter, 2);
        render_device().set_sampler_state(3, halo::d3d9::ss::address_u, 3);
        render_device().set_sampler_state(3, halo::d3d9::ss::address_v, 3);
        render_device().set_sampler_state(3, halo::d3d9::ss::address_w, 3);
        render_device().set_sampler_state(3, halo::d3d9::ss::mag_filter, 2);
        render_device().set_sampler_state(3, halo::d3d9::ss::min_filter, 2);
        stage7_index = 3;
    }

    render_device().set_sampler_state(stage7_index, halo::d3d9::ss::mip_filter, 2);
    render_device().set_sampler_state(0, halo::d3d9::ss::address_u, 1);
    render_device().set_sampler_state(0, halo::d3d9::ss::address_v, 1);
    render_device().set_sampler_state(0, halo::d3d9::ss::mag_filter, 2);
    render_device().set_sampler_state(0, halo::d3d9::ss::min_filter, 2);
    render_device().set_sampler_state(0, halo::d3d9::ss::mip_filter, 2);
}

}  // namespace rasterizer_underwater_tint_set_states_impl

}  // namespace halo::rasterizer
