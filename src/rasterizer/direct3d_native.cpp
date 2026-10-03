/**
 * @file src/rasterizer/direct3d_native.cpp
 * Functions that need the real Direct3D 9 and D3DX headers.
 */

#include "halo/render/d3d9.hpp"
#include "internal/state.hpp"
#include "d3d.h"
#include "halo/shell/api.hpp"
#include "halo/rasterizer/constants.hpp"
#include "halo/rasterizer/d3dx.hpp"
#include "halo/rasterizer/api.hpp"




namespace halo::rasterizer {


static void *get_param(void *effect, const char *name)
{
    return d3d_handle(render_device().effect_get_parameter_by_name(effect, 0, name));
}

/**
 * Allocates and looks up the named vertex/pixel-shader-constant handle tables used by every material shader,
 * after compiling all 122 pixel-shader effects from shaders\fx.bin.
 *
 * @address 0x52fab0
 */
uint8_t rasterizer_dx9_shaders_initialize(void)
{
    uint32_t saved_locale;
    int32_t hr;
    uint8_t success;
    int i;
    void **handles;

    saved_locale = GetThreadLocale();
    SetThreadLocale(k_locale_english_us);

    rasterizer_effect_defines[0].name = "PS_2_0_TARGET";
    rasterizer_effect_defines[0].definition = "ps_2_a";
    if (halo::shell::globals().force_shader != k_force_shader_ps_2_a) {
        rasterizer_effect_defines[0].definition = "ps_2_0";
    }
    rasterizer_effect_defines[1].name = 0;
    rasterizer_effect_defines[1].definition = 0;

    hr = D3DXCreateEffectPool((LPD3DXEFFECTPOOL *)&rasterizer_effect_pool);
    if (hr < 0) {
        success = 0;
    } else {
        success = rasterizer_dx9_pixel_shaders_load_all();
        if (success == 0) {
            rasterizer_shader_file_name = "shaders\\fx.bin";
            halo::shell::shell_display_fatal_error_dialog(0x89, 0x7e, 1);
        }
    }
    SetThreadLocale(saved_locale);
    if (success == 0) {
        halo::shell::shell_display_fatal_error_dialog(0x69, 0x7e, 1);
        return success;
    }

    for (i = 116; i <= 121; i++) {
        void *effect = rasterizer_effects[i].effect;
        handles = static_cast<void **>(GlobalAlloc(0, 0x14));
        rasterizer_effects[i].constant_handles = handles;
        handles[0] = get_param(effect, "c_primary_change_color");
        handles[1] = get_param(effect, "c_fog_color_correction_0");
        handles[2] = get_param(effect, "c_fog_color_correction_E");
        handles[3] = get_param(effect, "c_fog_color_correction_1");
        handles[4] = get_param(effect, "c_self_illumination_color");
    }

    for (i = 32; i <= 34; i++) {
        void *effect = rasterizer_effects[i].effect;
        handles = static_cast<void **>(GlobalAlloc(0, 0xc));
        rasterizer_effects[i].constant_handles = handles;
        handles[0] = get_param(effect, "c_eye_forward");
        handles[1] = get_param(effect, "c_view_perpendicular_color");
        handles[2] = get_param(effect, "c_view_parallel_color");
    }

    for (i = 37; i <= 39; i++) {
        void *effect = rasterizer_effects[i].effect;
        handles = static_cast<void **>(GlobalAlloc(0, 0xc));
        rasterizer_effects[i].constant_handles = handles;
        handles[0] = get_param(effect, "c_eye_forward");
        handles[1] = get_param(effect, "c_view_perpendicular_color");
        handles[2] = get_param(effect, "c_view_parallel_color");
    }

    {
        void *effect = rasterizer_effects[106].effect;
        handles = static_cast<void **>(GlobalAlloc(0, 0x10));
        rasterizer_effects[106].constant_handles = handles;
        handles[0] = get_param(effect, "c_eye_forward");
        handles[1] = get_param(effect, "c_view_perpendicular_color");
        handles[2] = get_param(effect, "c_view_parallel_color");
        handles[3] = get_param(effect, "c_group_intensity");
    }

    {
        void *effect = rasterizer_effects[107].effect;
        handles = static_cast<void **>(GlobalAlloc(0, 0xc));
        rasterizer_effects[107].constant_handles = handles;
        handles[0] = get_param(effect, "c_eye_forward");
        handles[1] = get_param(effect, "c_view_perpendicular_color");
        handles[2] = get_param(effect, "c_view_parallel_color");
    }

    {
        void *effect = rasterizer_effects[108].effect;
        handles = static_cast<void **>(GlobalAlloc(0, 0xc));
        rasterizer_effects[108].constant_handles = handles;
        handles[0] = get_param(effect, "c_eye_forward");
        handles[1] = get_param(effect, "c_view_perpendicular_color");
        handles[2] = get_param(effect, "c_view_parallel_color");
    }

    {
        void *effect = rasterizer_effects[0].effect;
        handles = static_cast<void **>(GlobalAlloc(0, 0x18));
        rasterizer_effects[0].constant_handles = handles;
        handles[0] = get_param(effect, "c_material_color");
        handles[1] = get_param(effect, "c_plasma_animation");
        handles[2] = get_param(effect, "c_primary_color");
        handles[3] = get_param(effect, "c_secondary_color");
        handles[4] = get_param(effect, "c_plasma_on_color");
        handles[5] = get_param(effect, "c_plasma_off_color");
    }

    for (i = 1; i <= 3; i++) {
        void *effect = rasterizer_effects[i].effect;
        handles = static_cast<void **>(GlobalAlloc(0, 4));
        rasterizer_effects[i].constant_handles = handles;
        handles[0] = get_param(effect, "c_material_color");
    }

    {
        void *effect = rasterizer_effects[114].effect;
        handles = static_cast<void **>(GlobalAlloc(0, 8));
        rasterizer_effects[114].constant_handles = handles;
        handles[0] = get_param(effect, "c_desaturation_tint");
        handles[1] = get_param(effect, "c_light_enhancement");
    }

    for (i = 40; i <= 43; i++) {
        void *effect = rasterizer_effects[i].effect;
        handles = static_cast<void **>(GlobalAlloc(0, 0x10));
        rasterizer_effects[i].constant_handles = handles;
        handles[0] = get_param(effect, "c_specular_brightness");
        handles[1] = get_param(effect, "c_view_perpendicular_color");
        handles[2] = get_param(effect, "c_view_parallel_color");
        handles[3] = get_param(effect, "c_multiplier");
    }
    return success;
}

namespace rasterizer_dx9_vertex_declarations_create_impl {


/**
 * Creates the full set of Direct3D vertex declarations (and their per-format stride/usage constants) used by
 * every geometry draw path in the rasterizer, returning false if any creation call fails.
 *
 * @address 0x5301b0
 */
uint8_t rasterizer_dx9_vertex_declarations_create(void)
{
    constexpr uint32_t k_usage_software_write_only = halo::d3d9::k_usage_software_processing | halo::d3d9::k_usage_write_only;
    constexpr uint32_t k_usage_dynamic_write_only = halo::d3d9::k_usage_dynamic | halo::d3d9::k_usage_write_only;
    constexpr uint32_t k_usage_dynamic_software = k_usage_dynamic_write_only | halo::d3d9::k_usage_software_processing;

    int32_t hr[19];
    uint8_t ok;
    int i;

    memset(rasterizer_vertex_declarations, 0, sizeof(rasterizer_vertex_declarations));


    hr[0] = render_device().create_vertex_declaration(vertex_elements_environment_uncompressed, &rasterizer_vertex_declarations[0].declaration);
    hr[1] = render_device().create_vertex_declaration(vertex_elements_environment_uncompressed, &rasterizer_vertex_declarations[1].declaration);
    hr[2] = render_device().create_vertex_declaration(vertex_elements_environment_lightmap, &rasterizer_vertex_declarations[2].declaration);
    hr[3] = render_device().create_vertex_declaration(vertex_elements_environment_lightmap, &rasterizer_vertex_declarations[3].declaration);
    hr[4] = render_device().create_vertex_declaration(vertex_elements_model, &rasterizer_vertex_declarations[4].declaration);
    hr[5] = render_device().create_vertex_declaration(vertex_elements_model, &rasterizer_vertex_declarations[5].declaration);
    hr[6] = render_device().create_vertex_declaration(vertex_elements_dynamic, &rasterizer_vertex_declarations[6].declaration);
    hr[7] = render_device().create_vertex_declaration(vertex_elements_dynamic, &rasterizer_vertex_declarations[7].declaration);
    hr[8] = render_device().create_vertex_declaration(vertex_elements_dynamic_screen, &rasterizer_vertex_declarations[8].declaration);
    hr[9] = render_device().create_vertex_declaration(vertex_elements_debug, &rasterizer_vertex_declarations[9].declaration);
    hr[10] = render_device().create_vertex_declaration(vertex_elements_decal, &rasterizer_vertex_declarations[10].declaration);
    hr[11] = render_device().create_vertex_declaration(vertex_elements_detail_object, &rasterizer_vertex_declarations[11].declaration);
    hr[12] = render_device().create_vertex_declaration(vertex_elements_environment_uncompressed_ff, &rasterizer_vertex_declarations[12].declaration);
    hr[13] = render_device().create_vertex_declaration(vertex_elements_environment_lightmap_ff, &rasterizer_vertex_declarations[13].declaration);
    hr[14] = render_device().create_vertex_declaration(vertex_elements_model_ff, &rasterizer_vertex_declarations[14].declaration);
    hr[15] = render_device().create_vertex_declaration(vertex_elements_model_processed, &rasterizer_vertex_declarations[15].declaration);
    hr[16] = render_device().create_vertex_declaration(vertex_elements_unlit_zsprite, &rasterizer_vertex_declarations[16].declaration);
    hr[17] = render_device().create_vertex_declaration(vertex_elements_screen_transformed_lit, &rasterizer_vertex_declarations[17].declaration);
    hr[18] = render_device().create_vertex_declaration(vertex_elements_screen_transformed_lit_specular, &rasterizer_vertex_declarations[18].declaration);

    ok = 1;
    for (i = 0; i < 19; i++) {
        if (hr[i] < 0) {
            ok = 0;
        }
    }

    rasterizer_vertex_declarations[14].usage = halo::d3d9::k_usage_write_only;
    rasterizer_vertex_declarations[13].usage = halo::d3d9::k_usage_write_only;
    rasterizer_vertex_declarations[12].usage = halo::d3d9::k_usage_write_only;
    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
        rasterizer_vertex_declarations[6].usage = k_usage_dynamic_software;
        rasterizer_vertex_declarations[7].usage = k_usage_dynamic_software;
        rasterizer_vertex_declarations[8].usage = k_usage_dynamic_software;
        rasterizer_vertex_declarations[9].usage = k_usage_dynamic_software;
        rasterizer_vertex_declarations[0].usage = k_usage_software_write_only;
        rasterizer_vertex_declarations[1].usage = k_usage_software_write_only;
        rasterizer_vertex_declarations[2].usage = k_usage_software_write_only;
        rasterizer_vertex_declarations[3].usage = k_usage_software_write_only;
        rasterizer_vertex_declarations[4].usage = k_usage_software_write_only;
        rasterizer_vertex_declarations[5].usage = k_usage_software_write_only;
        rasterizer_vertex_declarations[10].usage = k_usage_software_write_only;
        rasterizer_vertex_declarations[11].usage = k_usage_software_write_only;
        rasterizer_vertex_declarations[16].usage = k_usage_software_write_only;
    } else {
        rasterizer_vertex_declarations[0].usage = halo::d3d9::k_usage_write_only;
        rasterizer_vertex_declarations[1].usage = halo::d3d9::k_usage_write_only;
        rasterizer_vertex_declarations[2].usage = halo::d3d9::k_usage_write_only;
        rasterizer_vertex_declarations[3].usage = halo::d3d9::k_usage_write_only;
        rasterizer_vertex_declarations[4].usage = halo::d3d9::k_usage_write_only;
        rasterizer_vertex_declarations[5].usage = halo::d3d9::k_usage_write_only;
        rasterizer_vertex_declarations[6].usage = k_usage_dynamic_write_only;
        rasterizer_vertex_declarations[7].usage = k_usage_dynamic_write_only;
        rasterizer_vertex_declarations[8].usage = k_usage_dynamic_write_only;
        rasterizer_vertex_declarations[9].usage = k_usage_dynamic_write_only;
        rasterizer_vertex_declarations[10].usage = halo::d3d9::k_usage_write_only;
        rasterizer_vertex_declarations[11].usage = halo::d3d9::k_usage_write_only;
        rasterizer_vertex_declarations[16].usage = halo::d3d9::k_usage_write_only;
    }
    rasterizer_vertex_declarations[15].usage = k_usage_dynamic_write_only;
    rasterizer_vertex_declarations[17].usage = k_usage_dynamic_write_only;
    rasterizer_vertex_declarations[18].usage = k_usage_dynamic_write_only;

    D3DXFVFFromDeclarator((const D3DVERTEXELEMENT9 *)vertex_elements_model_processed,
        (DWORD *)&rasterizer_vertex_declarations[15].fvf);

    rasterizer_vertex_declarations[17].fvf = halo::d3d9::k_fvf_xyzrhw_diffuse_tex1;
    rasterizer_vertex_declarations[18].fvf = halo::d3d9::k_fvf_xyzrhw_diffuse_specular_tex1;

    if (rasterizer_caps.max_streams < 2) {
        int32_t hr19 = render_device().create_vertex_declaration(vertex_elements_environment_single_stream_ff, &rasterizer_vertex_declarations[19].declaration);
        ok = (hr19 >= 0) && ok;
        rasterizer_vertex_declarations[19].usage = halo::d3d9::k_usage_write_only;
    }

    return ok;
}

}  // namespace rasterizer_dx9_vertex_declarations_create_impl

namespace rasterizer_render_loading_screen_impl {




/**
 * Loads and presents a loading/splash screen resource (mode == 1), falling back to clearing the screen white
 * for any other mode that reaches the tail path (mode == 0, or a failed load).
 *
 * Registers: EAX -> mode
 *
 * @address 0x5157e0
 */
void rasterizer_render_loading_screen(int32_t mode)
{
    if (mode != 0) {
        void *splash = 0;
        void *render_target = 0;
        int32_t hr;

        if (mode != 1 || rasterizer_device == nullptr) {
            return;
        }
        hr = render_device().create_offscreen_plain_surface(k_loading_screen_width, k_loading_screen_height, halo::d3d9::k_format_x8r8g8b8, halo::d3d9::k_pool_default, &splash, 0);
        if (hr >= 0) {
            hr = D3DXLoadSurfaceFromResourceA((LPDIRECT3DSURFACE9)splash, 0, 0, (HMODULE)halo::shell::globals().module_handle, MAKEINTRESOURCEA(k_loading_screen_resource_id), 0,
                                              halo::rasterizer::d3dx::k_default, 0, 0);
            if (hr >= 0) {
                render_device().get_render_target(0, &render_target);

                render_device().stretch_rect(splash, 0, render_target, 0, 0);
                rasterizer_capture_and_present(nullptr, (BitmapData *)0);

                render_device().stretch_rect(splash, 0, render_target, 0, 0);

                render_device().release(render_target);
            }
            render_device().release(splash);
            if (hr >= 0) {
                return;
            }
        }
    }

    if (rasterizer_device != nullptr) {
        render_device().clear(0, 0, halo::d3d9::k_clear_target | halo::d3d9::k_clear_zbuffer | halo::d3d9::k_clear_stencil, 0, 1.0f, 0);
        rasterizer_capture_and_present(nullptr, (BitmapData *)0);
        render_device().clear(0, 0, halo::d3d9::k_clear_target | halo::d3d9::k_clear_zbuffer | halo::d3d9::k_clear_stencil, 0, 1.0f, 0);
    }
}

}  // namespace rasterizer_render_loading_screen_impl

}  // namespace halo::rasterizer
