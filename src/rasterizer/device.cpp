/**
 * @file src/rasterizer/device.cpp
 * Window, display mode, device creation, reset, frame bracket, capture and shutdown.
 */

#include "halo/interface/chat_gui.hpp"
#include "halo/render/d3d9.hpp"
#include "halo/rasterizer/globals.hpp"
#include "internal/state.hpp"
#include "halo/rasterizer/constants.hpp"
#include "halo/core/bit_cast.hpp"
#include <cstring>
#include "halo/bitmaps/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/render/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/bitmaps/bitmaps.hpp"
#include "halo/interface/api.hpp"
#include "halo/rasterizer/d3dx.hpp"

namespace {

constexpr uint32_t k_game_icon_resource_id = 102;
constexpr uint32_t k_vendor_id_ati = 0x1002;
constexpr uint32_t k_vendor_id_nvidia = 0x10de;
constexpr uint32_t k_device_id_radeon_9200 = 0x514c;
constexpr uint32_t k_device_id_radeon_9200_se = 0x514e;
constexpr uint32_t k_device_id_radeon_9200_pro = 0x514f;
constexpr uint32_t k_device_id_radeon_9200_4242 = 0x4242;
constexpr uint32_t k_chat_bar_color = 0xb0202020u;
constexpr uint32_t k_color_white = 0xffffffffu;
constexpr uint32_t k_fpu_control_word = 0x7e;
constexpr uint32_t k_pixel_shader_none = 0xffffffffu;

}  // namespace
#include "halo/core/win32_constants.hpp"
#include "halo/core/datum.hpp"
#include "halo/platform/time.hpp"
#include "halo/platform/memory.hpp"
#include "halo/platform/system.hpp"
#include "halo/platform/window.hpp"




namespace halo::rasterizer {


/**
 * Configures the framebuffer alpha-blend function (SrcBlend/DestBlend/BlendOp) for the requested blend mode,
 * forcing additive blending for modes 5/6 when the debug toggle is set.
 *
 * Registers: CX -> mode
 *
 * @address 0x5185d0
 */
void chimera__rasterizer_set_framebuffer_blend_function(int16_t mode)
{

    if (halo::shell::globals().min_max_blend_op_is_broken != 0 && (mode == 5 || mode == 6)) {
        render_device().set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::one);
        render_device().set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::one);
        render_device().set_render_state(halo::d3d9::rs::blend_op, 1);
        return;
    }

    render_device().set_render_state(halo::d3d9::rs::src_blend, rasterizer_blend_src_table[mode]);
    render_device().set_render_state(halo::d3d9::rs::dest_blend, rasterizer_blend_dest_table[mode]);
    render_device().set_render_state(halo::d3d9::rs::blend_op, rasterizer_blend_op_table[mode]);
}

namespace chimera__rasterizer_set_frustum_z_func_impl {


/**
 * REWRITTEN (objdump 0x518f40..0x5191ff, 2026-09-25). The Ghidra-shaped draft passed a code address (0x5190f3)
 * where the original builds an identity matrix, uploaded 0 vertex shader constants instead of 6, and turned
 * the raw z bits into floats by value; hooked, the world was not drawn at all. Reads (all inside
 * rasterizer_window, 0x7c1220): view = 4x3 at 0x7c1290 (rows of three floats), projection = 4x4 at 0x7c13c0,
 * camera position 0x7c1228 / forward 0x7c1234, and the two rows at 0x7c12c4 / 0x7c12d0.
 *
 * Registers: stack -> z_near, z_far (raw float bits, forwarded unchanged)
 *
 * @address 0x518f40
 */
void chimera__rasterizer_set_frustum_z_func(float z_near, float z_far)
{
    const float *view = &rasterizer_window.frustum.world_to_view.forward.i;
    const float *projection = &rasterizer_window.frustum.projection[0][0];
    float constants[6][4];
    float rows_1b[2][4];
    int32_t i, j;

    halo::render::render_camera_projection_zrange_push_pop_set(&rasterizer_window.frustum, z_near, z_far);

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            constants[i][j] = view[j * 3 + 0] * projection[0 * 4 + i] + view[j * 3 + 2] * projection[2 * 4 + i] +
                              projection[1 * 4 + i] * view[j * 3 + 1];
        }
        constants[i][3] = projection[3 * 4 + i] + constants[i][3];
    }

    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {

        float identity[16] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                              0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
        float view4[16];
        for (j = 0; j < 4; j++) {
            view4[j * 4 + 0] = view[j * 3 + 0];
            view4[j * 4 + 1] = view[j * 3 + 1];
            view4[j * 4 + 2] = view[j * 3 + 2];
            view4[j * 4 + 3] = j == 3 ? 1.0f : 0.0f;
        }
        render_device().set_transform(halo::d3d9::k_transform_world, identity);
        render_device().set_transform(2, view4);
        render_device().set_transform(3, projection);
    }

    const render_camera &camera = rasterizer_window.camera;
    const real_matrix4x3 &view_to_world = rasterizer_window.frustum.view_to_world;

    constants[4][0] = camera.position.x; constants[4][1] = camera.position.y; constants[4][2] = camera.position.z; constants[4][3] = 2.0f;
    constants[5][0] = camera.forward.i; constants[5][1] = camera.forward.j; constants[5][2] = camera.forward.k; constants[5][3] = 0.5f;
    render_device().set_vertex_shader_constant_f(0, constants, 6);

    rows_1b[0][0] = view_to_world.forward.i; rows_1b[0][1] = view_to_world.forward.j; rows_1b[0][2] = view_to_world.forward.k; rows_1b[0][3] = 1.0f;
    rows_1b[1][0] = view_to_world.left.i; rows_1b[1][1] = view_to_world.left.j; rows_1b[1][2] = view_to_world.left.k; rows_1b[1][3] = 3.0f;
    render_device().set_vertex_shader_constant_f(0x1b, rows_1b, 2);
}

}  // namespace chimera__rasterizer_set_frustum_z_func_impl

/**
 * Builds a display-mode descriptor from the current cached present parameters, applying the same platform-id
 * refresh-rate fallback as rasterizer_get_refresh_rate.c.
 *
 * Registers: unaff_EDI -> out
 *
 * @address 0x515ca0
 */
void display_mode_get_current(rasterizer_display_mode *out)
{
    int32_t refresh_rate;

    out->width = rasterizer_present_parameters.back_buffer_width;
    out->height = rasterizer_present_parameters.back_buffer_height;
    refresh_rate = rasterizer_present_parameters.fullscreen_refresh_rate;

    if (os_platform == 0) {
        halo::shell::os_platform_identify();
    }

    if (os_platform < 3) {
        refresh_rate = 0x3c;
    } else if (refresh_rate == 0) {
        out->refresh_rate = rasterizer_desktop_display_mode.refresh_rate;
        out->vsync = (rasterizer_present_parameters.presentation_interval == 1);
        return;
    }
    out->refresh_rate = refresh_rate;
    out->vsync = (rasterizer_present_parameters.presentation_interval == 1);
}

/**
 * Latches the current frame's window/camera/frustum/fog parameters into rasterizer_window and resets every
 * per-frame subsystem (dynamic geometry slots, transparent groups, lights).
 *
 * @address 0x5175c0
 */
void rasterizer_begin_frame(rasterizer_window_parameters *source)
{
    int i;

    rasterizer_window = *source;

    rasterizer_scratch_memory_used = 0;

    for (i = 0; i < k_rasterizer_vertex_type_count; i++) {
        rasterizer_dynamic_vertex_caches[i].used = 0;
    }
    rasterizer_dynamic_vertex_slot_count = 0;
    rasterizer_dynamic_index_count = 0;
    rasterizer_dynamic_index_slot_count = 0;
    transparent_geometry_group_count = 0;
    halo::rasterizer::fields::frame_reset_cleared_word = 0;
    for (i = 0; i < 12; i++) {
        transparent_geometry_group_drawn_bits[i] = 0;
    }
    transparent_geometry_group_secondary_count = 0;
    rasterizer_light_count = 0;
    rasterizer_light_disable_all();

    halo::rasterizer::fields::transparent_group_created = 0;
    halo::rasterizer::fields::frame_reset_cleared_word_b = 0;
    rasterizer_render_target_capture_done = 0;
    rasterizer_render_target_capture_requested = 0;

    rasterizer_set_shader_stage_config(0);
    rasterizer_set_fog_constants(&source->fog);

    {
        uint32_t clear_color = (halo::rasterizer::fields::rasterizer_debug_mode == 1) ? 0 : halo::interface::color_rgb_float_to_int(&rasterizer_window.fog.atmospheric_color.red);
        if (rasterizer_window.type == 1 || rasterizer_window.type == 2) {
            rasterizer_render_target_set_active(rasterizer_window.type, clear_color, source->clear_target == 0);
        }
    }

    chimera__rasterizer_set_frustum_z_func(-1.0f, -1.0f);

    render_device().set_render_state(halo::d3d9::rs::fill_mode, 3 - (uint32_t)(halo::rasterizer::fields::rasterizer_wireframe != 0));
}

/**
 * Reads or constructs a D3D present-parameters block. With no source display mode, copies the currently cached
 * parameters out; with one, builds fresh parameters from its width/height/ refresh_rate/vsync.
 *
 * Registers: EAX -> source(opt), stack -> dest
 *
 * @address 0x515fc0
 */
void rasterizer_build_present_parameters(d3d_present_parameters *dest, rasterizer_display_mode *source)
{
    if (source == (rasterizer_display_mode *)0) {
        *dest = rasterizer_present_parameters;
        return;
    }

    std::memset(dest, 0, sizeof(*dest));

    dest->flags = (config_disable_buffering == 0 && halo::rasterizer::fields::lockable_back_buffer_requested == 0 && screenshots == 0) ? 0 : 1;
    dest->enable_auto_depth_stencil = 1;
    dest->swap_effect = (rasterizer_fullscreen == 0) ? 3 : 1;
    dest->back_buffer_width = (uint32_t)source->width;
    dest->back_buffer_height = (uint32_t)source->height;
    dest->back_buffer_format = halo::d3d9::k_format_x8r8g8b8;
    dest->back_buffer_count = 1;
    dest->auto_depth_stencil_format = halo::d3d9::k_format_d24s8;
    dest->device_window = (uint32_t)halo::shell::globals().window;

    if (rasterizer_fullscreen == 0) {
        dest->windowed = 1;
        dest->fullscreen_refresh_rate = 0;
    } else {
        if (os_platform == 0) {
            halo::shell::os_platform_identify();
        }
        dest->windowed = 0;
        if (video_force_mode_flag == 0 && source->refresh_rate != 0 && os_platform > 2) {
            dest->fullscreen_refresh_rate = (uint32_t)source->refresh_rate;
        } else {
            dest->fullscreen_refresh_rate = 0;
        }
    }

    if (source->vsync != 0 && game_time_force_single_tick == 0) {
        dest->presentation_interval = halo::d3d9::k_present_interval_one;
    } else {
        dest->presentation_interval = halo::d3d9::k_present_interval_immediate;
    }
}

namespace rasterizer_capture_and_present_impl {


/**
 * Direct3D 9 back end function rasterizer_capture_and_present.
 *
 * @address 0x518180
 */
void rasterizer_capture_and_present(const int16_t *tile, BitmapData *bitmap)
{
    uint8_t ok = 1;
    void *surface;
    d3d_surface_desc desc;
    d3d_locked_rect locked;
    int32_t hr;

    if (rasterizer_device_lost) {
        return;
    }
    if (halo::shell::globals().disable_buffering != 0) {
        surface = NULL;
        if (render_device().get_back_buffer(0, 0, 0, &surface) < 0) {
            ok = 0;
        }
        if (render_device().surface_get_desc(surface, &desc) < 0 || !ok ||
            render_device().surface_lock_rect(surface, &locked, NULL, halo::d3d9::k_lock_read_only) < 0) {
            ok = 0;
        } else if (locked.bits != 0 &&
                   render_device().surface_unlock_rect(surface) < 0) {
            ok = 0;
        }
        render_device().release(surface);
    }
    if (screenshots != 0 && bitmap != NULL && bitmap->pixel_base != nullptr) {
        int16_t top = low_half(game_window_top_left);
        int16_t left = high_half(game_window_top_left);
        int16_t bottom = low_half(game_window_bottom_right);
        int16_t right = high_half(game_window_bottom_right);

        if (tile != NULL) {
            int16_t width = (int16_t)(right - left);
            int16_t height = (int16_t)(bottom - top);

            left = (int16_t)(tile[0] * width);
            top = (int16_t)(tile[1] * height);
            right = (int16_t)(left + width);
            bottom = (int16_t)(top + height);
        }
        if ((bitmap->format == bitmapdataformat_a8r8g8b8 || bitmap->format == bitmapdataformat_x8r8g8b8) && bitmap->mipmap_count == 0 &&
            left >= 0 && top >= 0 && right <= (int16_t)bitmap->width && bottom <= (int16_t)bitmap->height) {
            surface = NULL;
            if (render_device().get_back_buffer(0, 0, 0, &surface) < 0) {
                ok = 0;
            }
            if (render_device().surface_get_desc(surface, &desc) >= 0 && ok &&
                render_device().surface_lock_rect(surface, &locked, NULL, halo::d3d9::k_lock_read_only) >= 0 &&
                locked.bits != 0) {
                int16_t rows = (int16_t)(low_half(game_window_bottom_right) - low_half(game_window_top_left));
                int32_t row_bytes = (int32_t)bitmap_format_bits_per_pixel[bitmap->format] *
                                    (int32_t)(int16_t)bitmap->width / 8;
                int16_t row;

                for (row = 0; row < rows; row++) {
                    const uint8_t *source = locked.bits + (int32_t)row * locked.pitch;
                    void *destination = halo::bitmaps::bitmap_data_view(bitmap).get_row_address(0, left, top + row);

                    memcpy(destination, source, (size_t)row_bytes);
                }
                render_device().surface_unlock_rect(surface);
            }
            render_device().release(surface);
        }
    }

    hr = render_device().present(NULL, NULL, NULL, NULL);
    if (hr == halo::d3d9::k_error_device_lost || hr == halo::d3d9::k_error_driver_internal_error) {
        rasterizer_device_lost = 1;
    } else if (hr == 0) {
        rasterizer_pending_clear = (uint8_t)(rasterizer_pending_clear == 0);
    }
    {
        uint32_t low = (uint32_t)rasterizer_present_counter_low + 1;
        rasterizer_present_counter_high += (low == 0);
        rasterizer_present_counter_low = (int32_t)low;
    }
}

}  // namespace rasterizer_capture_and_present_impl

/**
 * 0x0071d184, HDC Registers the game window class and creates/centers a `width` by `height` window on the
 * primary desktop, reporting a message box and returning 0 on failure. On success, loads the taskbar icon
 * bitmap (best-effort, ignored on failure) and brings the window to the front.
 *
 * Registers: EAX -> height, unaff_EBX -> width
 *
 * @address 0x515930
 */
uint32_t rasterizer_create_game_window(int32_t height, int32_t width)
{
    void *hwnd = halo::platform::window_create(halo::shell::globals().instance, halo::shell::globals().module_handle, shell_window_class_name,
        shell_window_title, width, height, k_game_icon_resource_id, k_loading_screen_resource_id, halo::shell::game_window_events());

    if (hwnd == nullptr) {
        return 0;
    }
    halo::shell::globals().window = hwnd;
    return 1;
}

namespace rasterizer_device_reset_impl {


typedef struct d3d_viewport9 {
    uint32_t x, y, width, height;
    float min_z, max_z;
} d3d_viewport9;

/**
 * stack -> present_parameters (the D3DPRESENT_PARAMETERS to reset with; rasterizer_build_present_parameters
 * fills it) Unbinds textures and shaders, releases every D3D resource the rasterizer created (vertex buffers,
 * render targets, vertex declarations, vertex and pixel shaders, occlusion queries), resets the device, then
 * restores the state: present parameters copied, a full viewport, default render states, the splash drawn,
 * effects / render targets / occlusion queries recreated and the lost vertex buffers rebuilt.
 *
 * @address 0x515d90
 */
uint8_t rasterizer_device_reset(d3d_present_parameters *present_parameters)
{
    uint32_t i;
    int32_t hr;
    uint8_t ok;
    d3d_viewport9 viewport;

    render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
    for (i = 0; i < rasterizer_caps.max_simultaneous_textures; i++) {
        render_device().set_texture(i, 0);
    }
    render_device().set_vertex_shader(0);
    render_device().set_pixel_shader(0);

    for (i = 0; i < (uint32_t)rasterizer_vertex_buffer_slot_high_water; i++) {
        void *buffer = rasterizer_vertex_buffer_slots[i].hardware_buffer;
        if (buffer != 0) {
            render_device().release(buffer);
            rasterizer_vertex_buffer_slots[i].hardware_buffer = 0;
        }
    }
    rasterizer_render_target_dispose();
    for (i = 0; i < k_rasterizer_vertex_type_count; i++) {
        void *declaration = rasterizer_vertex_declarations[i].declaration;
        if (declaration != 0) {
            render_device().release(declaration);
        }
    }
    for (i = 0; i < k_rasterizer_vertex_type_count; i++) {
        rasterizer_vertex_declarations[i].declaration = 0;
        rasterizer_vertex_declarations[i].fvf = 0;
        rasterizer_vertex_declarations[i].usage = 0;
    }
    for (i = 0; i < k_rasterizer_vertex_shaders; i++) {
        void *shader = rasterizer_vertex_shaders[i].shader;
        if (shader != 0) {
            render_device().release(shader);
            rasterizer_vertex_shaders[i].shader = 0;
        }
    }
    rasterizer_dx9_pixel_shaders_release();
    for (i = 0; i < k_lens_flare_occlusion_queries; i++) {
        void *query = lens_flare_occlusion_queries[i];
        if (query != 0) {
            render_device().release(query);
            lens_flare_occlusion_queries[i] = 0;
        }
    }

    hr = render_device().reset(present_parameters);
    if (hr < 0 || rasterizer_device == 0) {
        if (hr == halo::d3d9::k_error_driver_internal_error) {
            halo::shell::shell_display_fatal_error_dialog(0x81, 0x82, 1);
            return 0;
        }
        halo::platform::sleep_milliseconds(50);
        return 0;
    }

    rasterizer_present_parameters = *present_parameters;
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = present_parameters->back_buffer_width;
    viewport.height = present_parameters->back_buffer_height;
    viewport.min_z = 0.0f;
    viewport.max_z = 1.0f;
    ok = render_device().set_viewport(&viewport) >= 0;
    rasterizer_pending_clear = 0;
    rasterizer_set_default_render_states();
    rasterizer_render_loading_screen(1);

    if (ok && rasterizer_dx9_effects_initialize() && rasterizer_render_target_initialize() &&
        rasterizer_lens_flare_occlusion_queries_create()) {
        ok = 1;
    } else {
        ok = 0;
    }
    rasterizer_vertex_buffer_slot_recreate_lost();
    return ok;
}

}  // namespace rasterizer_device_reset_impl

/**
 * Returns nonzero if `requested` differs from the currently active display mode (dimensions always compared;
 * refresh rate compared only while windowed and video_force_mode_flag is clear).
 *
 * Registers: unaff_EDI -> requested
 *
 * @address 0x515d10
 */
uint8_t rasterizer_display_mode_differs(rasterizer_display_mode *requested)
{
    uint8_t differs = (requested->height != (int32_t)rasterizer_present_parameters.back_buffer_height) ||
                       (requested->width != (int32_t)rasterizer_present_parameters.back_buffer_width);

    if (rasterizer_fullscreen != 0) {
        if (video_force_mode_flag == 0) {
            if (requested->height == (int32_t)rasterizer_present_parameters.back_buffer_height &&
                requested->width == (int32_t)rasterizer_present_parameters.back_buffer_width) {
                if (os_platform == 0) {
                    halo::shell::os_platform_identify();
                }
                if (os_platform > 2) {
                    int32_t rate_a = rasterizer_get_refresh_rate(requested->refresh_rate);
                    int32_t rate_b = rasterizer_get_refresh_rate((int32_t)rasterizer_present_parameters.fullscreen_refresh_rate);
                    differs = (rate_a != rate_b);
                }
            }
        }
        {
            uint8_t vsync_differs = (uint8_t)((rasterizer_present_parameters.presentation_interval == 1) ^ requested->vsync);
            differs = differs | vsync_differs;
        }
    }
    return differs;
}

namespace rasterizer_end_frame_impl {


static void rasterizer_set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void rasterizer_set_sampler_state(uint32_t stage, uint32_t type, uint32_t value)
{
    render_device().set_sampler_state(stage, type, value);
}

static void rasterizer_set_texture_stage_state(uint32_t sampler, uint32_t type, uint32_t value)
{
    render_device().set_texture_stage_state(sampler, type, value);
}

static void rasterizer_set_quad_vertex(rasterizer_screen_vertex *vertex, float x, float y, float u, float v)
{
    vertex->x = x;
    vertex->y = y;
    vertex->z = 0.0f;
    vertex->rhw = 1.0f;
    vertex->diffuse = k_color_white;
    vertex->u = u;
    vertex->v = v;
}

/**
 * Direct3D 9 back end function rasterizer_end_frame.
 *
 * @address 0x517b90
 */
void rasterizer_end_frame(void)
{
    uint8_t succeeded = 1;

    if (rasterizer_frame_started == 1 && rasterizer_caps_flag_68a == 0) {
        void *back_buffer = rasterizer_render_targets[0].surface;
        int16_t width = (int16_t)(rasterizer_window.camera.viewport_bounds.right - rasterizer_window.camera.viewport_bounds.left);
        int16_t height = (int16_t)(rasterizer_window.camera.viewport_bounds.bottom - rasterizer_window.camera.viewport_bounds.top);
        d3d_surface_desc desc;
        d3d_viewport viewport;
        rasterizer_screen_vertex *vertices = 0;
        uint32_t stride;

        render_device().set_render_target(0, back_buffer);
        rasterizer_active_render_target = 0;
        render_device().surface_get_desc(back_buffer, &desc);
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = desc.width;
        viewport.height = desc.height;
        viewport.min_z = 0.0f;
        viewport.max_z = 1.0f;
        render_device().set_viewport(&viewport);

        if (halo::rasterizer::fields::rasterizer_wireframe != 0) {
            rasterizer_set_render_state(halo::d3d9::rs::fill_mode, 3);
        }

        stride = halo::rasterizer::d3dx::fvf_vertex_size(halo::d3d9::k_fvf_xyzrhw_diffuse_tex1);
        render_device().set_pixel_shader(0);
        render_device().set_texture(0, rasterizer_render_targets[1].texture);
        rasterizer_set_sampler_state(0, halo::d3d9::ss::address_u, 3);
        rasterizer_set_sampler_state(0, halo::d3d9::ss::address_v, 3);
        rasterizer_set_sampler_state(0, halo::d3d9::ss::mag_filter, 1);
        rasterizer_set_sampler_state(0, halo::d3d9::ss::min_filter, 1);
        rasterizer_set_sampler_state(0, halo::d3d9::ss::mip_filter, 1);
        rasterizer_set_render_state(halo::d3d9::rs::cull_mode, 3);
        rasterizer_set_render_state(halo::d3d9::rs::color_write_enable, 7);
        rasterizer_set_render_state(halo::d3d9::rs::alpha_blend_enable, 0);
        rasterizer_set_render_state(halo::d3d9::rs::alpha_test_enable, 0);
        rasterizer_set_render_state(halo::d3d9::rs::z_enable, 0);
        rasterizer_set_render_state(halo::d3d9::rs::fog_enable, 0);
        rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_op, halo::d3d9::top::select_arg1);
        rasterizer_set_texture_stage_state(0, halo::d3d9::ts::color_arg1, halo::d3d9::ta::texture);
        rasterizer_set_texture_stage_state(0, halo::d3d9::ts::alpha_op, halo::d3d9::top::select_arg1);
        rasterizer_set_texture_stage_state(0, halo::d3d9::ts::alpha_arg1, halo::d3d9::ta::texture);
        rasterizer_set_texture_stage_state(1, halo::d3d9::ts::color_op, halo::d3d9::top::disable);
        rasterizer_set_texture_stage_state(1, halo::d3d9::ts::alpha_op, halo::d3d9::top::disable);

        if (rasterizer_render_target_vertex_buffer != 0) {
            void *buffer = rasterizer_render_target_vertex_buffer;

            render_device().buffer_lock(buffer, 0, stride * 4, &vertices, halo::d3d9::k_lock_discard);
            if (vertices != 0) {
                float right = (float)width - 0.5f;
                float bottom = (float)height - 0.5f;

                rasterizer_set_quad_vertex(&vertices[0], -0.5f, -0.5f, 0.0f, 0.0f);
                rasterizer_set_quad_vertex(&vertices[1], right, -0.5f, 1.0f, 0.0f);
                rasterizer_set_quad_vertex(&vertices[2], right, bottom, 1.0f, 1.0f);
                rasterizer_set_quad_vertex(&vertices[3], -0.5f, bottom, 0.0f, 1.0f);
                if (halo::shell::globals().linear_texture_addressing != 0) {
                    vertices[1].u *= (float)width;
                    vertices[2].u *= (float)width;
                    vertices[2].v *= (float)height;
                    vertices[3].v *= (float)height;
                }

                render_device().set_vertex_shader(0);
                render_device().set_fvf(halo::d3d9::k_fvf_xyzrhw_diffuse_tex1);
                render_device().buffer_unlock(buffer);
                render_device().set_software_vertex_processing(rasterizer_software_vertex_processing);
                render_device().set_stream_source(0, rasterizer_render_target_vertex_buffer, 0, stride);
                render_device().set_indices(rasterizer_render_target_index_buffer);
                render_device().draw_indexed_primitive(6, 0, 0, 4, 0, 2);
                render_device().set_fvf(0);
            }
        }

        if (halo::rasterizer::fields::rasterizer_wireframe != 0) {
            rasterizer_set_render_state(halo::d3d9::rs::fill_mode, 2);
        }
    }

    if (chat_dialog_open != 0) {
        Rectangle2D chat_bar;

        chat_bar.top = 460;
        chat_bar.left = 0;
        chat_bar.bottom = 480;
        chat_bar.right = 640;
        halo::interface::ui_draw_filled_rectangle(k_chat_bar_color, &chat_bar);
    }

    if (render_device().set_software_vertex_processing(rasterizer_software_vertex_processing) < 0) {
        succeeded = 0;
    }
    rasterizer_set_render_state(halo::d3d9::rs::src_blend, halo::d3d9::blend::src_alpha);
    rasterizer_set_render_state(halo::d3d9::rs::dest_blend, halo::d3d9::blend::inv_src_alpha);
    rasterizer_set_render_state(halo::d3d9::rs::blend_op, 1);

    halo::interface::ChatGui::get().draw();

    if (render_device().set_software_vertex_processing(rasterizer_software_vertex_processing) < 0) {
        succeeded = 0;
    }
    if (render_device().end_scene() >= 0 && succeeded != 0) {
        rasterizer_in_scene = 0;
    }
}

}  // namespace rasterizer_end_frame_impl

/**
 * Returns `requested_rate` (or the default enumerated rate when it is 0) once the platform has been identified
 * as beyond the earliest two ids; otherwise falls back to a fixed 60Hz.
 *
 * Registers: unaff_ESI -> requested_rate
 *
 * @address 0x515c70
 */
int32_t rasterizer_get_refresh_rate(int32_t requested_rate)
{
    if (os_platform == 0) {
        halo::shell::os_platform_identify();
    }
    if (os_platform > 2) {
        if (requested_rate == 0) {
            return rasterizer_desktop_display_mode.refresh_rate;
        }
        return requested_rate;
    }
    return 0x3c;
}

namespace rasterizer_initialize_direct3d_impl {


typedef int32_t (__cdecl *nvcpl_get_data_int_fn)(int32_t data_type, int32_t *value);

static int command_line_has_switch(const char *name)
{
    int32_t i;

    for (i = 0; i < halo::shell::globals().argc; i++) {
        const char *argument = halo::shell::globals().argv[i];

        if (argument[0] == '-' && _stricmp(name, argument) == 0) {
            return 1;
        }
    }
    return 0;
}


static void rasterizer_fpu_reset_control_word(uint16_t control_word)
{
#if defined(__GNUC__)
    __asm__ volatile ("finit\n\tfldcw %0" : : "m" (control_word));
#else
    __asm { finit }
    __asm { fldcw control_word }
#endif
}

/**
 * Direct3D 9 back end function rasterizer_initialize_direct3d.
 *
 * @address 0x5169c0
 */
uint8_t rasterizer_initialize_direct3d(void)
{
    rasterizer_display_mode mode;
    d3d_display_mode desktop_mode;
    win32_rect desktop_rect;
    d3d_viewport viewport;
    uint32_t behavior_flags[4];
    uint8_t adapter_usable = 1;
    bool adapter_chosen = false;
    uint32_t requested_adapter = 0;
    int32_t attempt;
    uint32_t adapter_count;
    uint32_t adapter = 0;
    void *hwnd;
    uint8_t succeeded;

    mode.vsync = (uint8_t)(game_time_force_single_tick == 0);
    mode.width = 800;
    mode.height = 600;
    mode.refresh_rate = 60;
    rasterizer_device = 0;
    rasterizer_fullscreen = (uint8_t)(windowed == 0);
    if (rasterizer_window_requested != 0) {
        rasterizer_fullscreen = 0;
    }
    if (width640 != 0 || safe_mode != 0 || cpu_speed <= 1000 || physical_memory <= 0x80) {
        mode.width = 640;
        mode.height = 480;
    }
    rasterizer_parse_vidmode_commandline(&mode.width, &mode.height, (long *)&mode.refresh_rate);
    rasterizer_caps_flag_689 = 0;
    rasterizer_caps_flag_68a = 0;
    rasterizer_caps_flag_688 = 0;

    if (!rasterizer_create_game_window(mode.height, mode.width)) {
        return 0;
    }
    hwnd = halo::shell::globals().window;
    if (shell_direct3d != 0) {
        rasterizer_direct3d = shell_direct3d;
    } else {
        rasterizer_direct3d = direct3d_create9_procedure(halo::d3d9::k_sdk_version);
        if (rasterizer_direct3d == 0) {
            return 0;
        }
    }
    adapter_count = render_device().get_adapter_count(rasterizer_direct3d);
    if (adapter_count < 1) {
        return 0;
    }
    if (rasterizer_fullscreen != 0) {
        const char *argument;

        if (halo::shell::command_line_check_flag("-adapter", &argument)) {
            sscanf(argument, "%d", &requested_adapter);
            if (requested_adapter > adapter_count) {
                return 0;
            }
        }
    }

    for (attempt = -1; ; attempt++) {
        uint32_t shader_version;
        int32_t error;
        uint32_t i;

        if (attempt == -1) {
            if (requested_adapter == 0) {
                continue;
            }
            adapter = requested_adapter - 1;
        } else {
            if ((uint32_t)attempt >= adapter_count) {
                break;
            }
            adapter = (uint32_t)attempt;
        }

        rasterizer_device_type = command_line_has_switch("-useref") ? 2 : 1;
        render_device().get_device_caps(rasterizer_direct3d, adapter, rasterizer_device_type, &rasterizer_caps);
        error = (int32_t)halo::shell::shell_parse_config_txt(adapter, (d3d9_interface *)rasterizer_direct3d);
        if (error != 0) {
            halo::shell::shell_display_fatal_error_dialog(halo::k_dword_none, (uint32_t)error, 0);
        }

        shader_version = k_pixel_shader_none;
        if (halo::shell::globals().force_shader != 0) {
            if (halo::shell::globals().force_shader == k_force_shader_disabled || halo::shell::globals().force_shader == k_force_shader_fallback) {
                shader_version = 0;
            } else if (halo::shell::globals().force_shader == k_force_shader_ps_2_a) {
                shader_version = halo::d3d9::k_pixel_shader_version_2_0;
            } else {

                shader_version = halo::d3d9::pixel_shader_version((uint32_t)halo::shell::globals().force_shader / 10,
                                                                  (uint32_t)halo::shell::globals().force_shader % 10);
            }
        }
        if (command_line_has_switch("-useff") ||
            safe_mode != 0 || halo::shell::globals().safe_mode != 0 || halo::shell::globals().use_fixed_function != 0) {
            halo::shell::globals().force_shader = 0;
            shader_version = 0;
        }
        if (command_line_has_switch("-use00")) {
            halo::shell::globals().force_shader = 0;
            shader_version = 0;
            rasterizer_caps.max_streams = 1;
        }
        if (command_line_has_switch("-use11")) {
            halo::shell::globals().force_shader = 0;
            shader_version = halo::d3d9::k_pixel_shader_version_1_1;
        }
        if (command_line_has_switch("-use14")) {
            halo::shell::globals().force_shader = 0;
            shader_version = halo::d3d9::k_pixel_shader_version_1_4;
        }
        if (command_line_has_switch("-use20")) {
            halo::shell::globals().force_shader = 0;
            shader_version = halo::d3d9::k_pixel_shader_version_2_0;
        }
        if (command_line_has_switch("-use2a")) {
            halo::shell::globals().force_shader = 0;
            shader_version = halo::d3d9::k_pixel_shader_version_2_0;
        }
        if (shader_version != k_pixel_shader_none) {
            if (halo::d3d9::pixel_shader_version_major_minor(rasterizer_caps.pixel_shader_version) > halo::d3d9::pixel_shader_version_major_minor(shader_version)) {
                rasterizer_caps.pixel_shader_version = shader_version;
            }
        }

        if (halo::shell::globals().prototype_card != 0) {
            halo::shell::shell_display_fatal_error_dialog(0x93, 0x70, 0);
        }
        if (halo::shell::globals().unsupported_card != 0) {
            halo::shell::shell_display_fatal_error_dialog(0x67, 0x70, 0);
        }
        if (halo::shell::globals().invalid_driver != 0 && halo::shell::globals().unsupported_card == 0) {
            halo::shell::shell_display_fatal_error_dialog(0x69, 0x72, 0);
        }
        if (halo::shell::globals().old_driver != 0 && halo::shell::globals().invalid_driver == 0 &&
            halo::shell::globals().unsupported_card == 0) {
            halo::shell::shell_display_fatal_error_dialog(0x68, 0x71, 0);
        }
        if (halo::shell::globals().invalid_sound_driver != 0) {
            halo::shell::shell_display_fatal_error_dialog(0x8f, 0x72, 0);
        }
        if (halo::shell::globals().old_sound_driver != 0 && halo::shell::globals().invalid_sound_driver == 0) {
            halo::shell::shell_display_fatal_error_dialog(0x8e, 0x71, 0);
        }
        if (halo::shell::globals().disable_render_targets != 0) {
            rasterizer_caps_flag_689 = 1;
            rasterizer_caps_flag_68a = 1;
        }
        if (halo::shell::globals().disable_alpha_render_targets != 0) {
            rasterizer_caps_flag_68a = 1;
        }

        if (graphics_vendor_id == k_vendor_id_ati) {
            if (graphics_device_id == k_device_id_radeon_9200 || graphics_device_id == k_device_id_radeon_9200_se ||
                graphics_device_id == k_device_id_radeon_9200_pro || graphics_device_id == k_device_id_radeon_9200_4242) {
                rasterizer_caps_flag_688 = 1;
            }
        } else if (graphics_vendor_id == k_vendor_id_nvidia) {
            void *library = halo::platform::library_open("NVCPL.dll");

            if (library != 0) {
                nvcpl_get_data_int_fn get_data_int = (nvcpl_get_data_int_fn)halo::platform::library_symbol(library, "NvCplGetDataInt");

                if (get_data_int != 0) {
                    int32_t value = 0;

                    if (get_data_int(4, &value) != 0 && value != -1 && value != 0) {
                        halo::shell::shell_display_fatal_error_dialog(0x8d, 0x7e, 0);
                    }
                }
                halo::platform::library_close(library);
            }
        }

        if (video_memory < (required_video_memory << 20)) {
            halo::shell::shell_display_fatal_error_dialog(0x6c, 0x75, 0);
        }

        if (rasterizer_fullscreen == 0 && halo::platform::desktop_bits_per_pixel() != 32) {
            halo::shell::shell_display_fatal_error_dialog(0x83, 0x7e, 1);
        }

        if (render_device().get_adapter_display_mode(rasterizer_direct3d, adapter, &desktop_mode) < 0) {
            adapter_usable = 0;
            break;
        }
        if (rasterizer_fullscreen != 0) {
            halo::platform::window_set_fullscreen_style(hwnd);
        }
        if (rasterizer_fullscreen == 0) {

            halo::platform::desktop_bounds(reinterpret_cast<halo::platform::window_rect *>(&desktop_rect));
            if ((uint32_t)mode.height >= (uint32_t)desktop_rect.bottom ||
                (uint32_t)mode.width >= (uint32_t)desktop_rect.right) {
                if (desktop_rect.bottom > 600) {
                    mode.width = 800;
                    mode.height = 600;
                } else {
                    mode.width = 640;
                    mode.height = 480;
                }
            }
            adapter_usable = (uint8_t)(desktop_mode.format >= halo::d3d9::k_format_a8r8g8b8 && desktop_mode.format <= halo::d3d9::k_format_x8r8g8b8);
        }

        rasterizer_build_present_parameters(&rasterizer_present_parameters, &mode);
        behavior_flags[3] = halo::d3d9::k_create_software_vertex_processing;
        bool device_created = false;

        for (;;) {
            uint8_t no_pixel_shaders = (uint8_t)(rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1);

            behavior_flags[0] = no_pixel_shaders ? halo::d3d9::k_create_mixed_vertex_processing : halo::d3d9::k_create_hardware_vertex_processing;
            behavior_flags[1] = no_pixel_shaders ? halo::d3d9::k_create_software_vertex_processing : halo::d3d9::k_create_hardware_vertex_processing;
            behavior_flags[2] = no_pixel_shaders ? halo::d3d9::k_create_software_vertex_processing : halo::d3d9::k_create_mixed_vertex_processing;

            for (i = (~(rasterizer_caps.dev_caps >> 16)) & 1; i < 4; i++) {
                uint32_t flags = behavior_flags[i] +
                                 (halo::shell::globals().disable_driver_management != 0 ? halo::d3d9::k_create_disable_driver_management : 0) +
                                 (checkfpu != 0 ? halo::d3d9::k_create_fpu_preserve : 0) +
                                 (rasterizer_window_requested != 0 ? halo::d3d9::k_create_multithreaded : 0);

                if (render_device().create_device(rasterizer_direct3d, adapter, rasterizer_device_type, hwnd, flags, &rasterizer_present_parameters, &rasterizer_device) >= 0) {
                    rasterizer_software_vertex_processing = (uint8_t)(behavior_flags[i] & halo::d3d9::k_create_software_vertex_processing);
                    device_created = true;
                    break;
                }
            }
            if (device_created) {
                break;
            }
            if (video_force_mode_flag != 0) {
                video_force_mode_flag = (uint8_t)(mode.refresh_rate == 0);
                break;
            }

            video_force_mode_flag = 1;
            rasterizer_build_present_parameters(&rasterizer_present_parameters, &mode);
        }

        if (checkfpu != 0) {
            rasterizer_fpu_reset_control_word(k_fpu_control_word);
        }
        if ((adapter_usable != 0 && rasterizer_device != 0) || adapter_count <= 1) {
            adapter_chosen = true;
            break;
        }
        adapter_usable = 1;
    }

    if (adapter_chosen) {
        rasterizer_desktop_display_mode = desktop_mode;
        rasterizer_resize_game_window(mode.height, mode.width);
    }

    if (rasterizer_device == 0 || adapter_usable == 0) {
        rasterizer_device = 0;
        halo::shell::shell_display_fatal_error_dialog(0x81, 0x82, 1);
        return 0;
    }

    rasterizer_texture_stage_count = (int16_t)(rasterizer_caps.max_simultaneous_textures < 4 ? 2 : 4);
    d3d_adapter = adapter;
    rasterizer_maximum_skinning_nodes = 0x3f;

    if (render_device().check_device_format(rasterizer_direct3d, adapter, rasterizer_device_type, 0x16, 1, 1, 0x15) < 0) {
        rasterizer_caps_flag_68a = 1;
    }
    if (rasterizer_fullscreen != 0 && rasterizer_device != 0) {
        halo::platform::cursor_show(false);
    }
    rasterizer_device_lost = 0;
    rasterizer_pending_clear = 0;
    rasterizer_set_default_render_states();
    if (rasterizer_window_requested == 0) {
        rasterizer_render_loading_screen(1);
    }
    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
        if (rasterizer_texture_stage_count >= 2) {
            rasterizer_texture_stage_count = 2;
        }
        rasterizer_caps_flag_688 = 1;
    }
    if (command_line_has_switch("-usefxfile")) {
        rasterizer_use_fx_file = 1;
    }
    rasterizer_select_hardware_codepaths();

    succeeded = 0;
    if ((uint8_t)rasterizer_dx9_effects_initialize() &&
        (rasterizer_scratch_memory = halo::platform::heap_allocate(0, k_rasterizer_scratch_memory_size)) != 0 &&
        (uint8_t)rasterizer_decal_index_buffer_initialize() &&
        (uint8_t)transparent_geometry_pool_initialize() &&
        (uint8_t)text_font_system_initialize() &&
        rasterizer_detail_object_vertex_buffer_create() &&
        rasterizer_render_target_initialize() &&
        (uint8_t)rasterizer_lens_flare_occlusion_queries_create()) {
        succeeded = 1;
    }
    chimera__registry_check_4();

    {
        uint32_t size = 0x78;
        const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&size);
        uint8_t *block = game_state_base + game_state_cursor;
        int32_t n;

        game_state_cursor += 0x78;
        if (halo::memory::globals().crc32_lookup_table_initialized == 0) {
            halo::memory::crc32_build_table(&halo::memory::globals().crc32_lookup_table);
            halo::memory::globals().crc32_lookup_table_initialized = 1;
        }
        for (n = 0; n < 4; n++) {
            game_state_crc = (game_state_crc >> 8) ^ halo::memory::globals().crc32_lookup_table.entries[(bytes[n] ^ game_state_crc) & 0xff];
        }
        cinematic_screen_effect_state = (cinematic_screen_effect_globals *)block;
    }
    halo::cache::texture_cache_new();
    if (rasterizer_reset_device_if_needed()) {
        rasterizer_end_frame();
    }

    if (succeeded == 0) {
        rasterizer_shutdown();
        return 0;
    }
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (uint32_t)mode.width;
    viewport.height = (uint32_t)mode.height;
    viewport.min_z = 0.0f;
    viewport.max_z = 1.0f;
    if (render_device().set_viewport(&viewport) < 0) {
        succeeded = 0;
    }
    rasterizer_frame_started = 1;
    return succeeded;
}

}  // namespace rasterizer_initialize_direct3d_impl

/**
 * Parses -vidmode width,height[,refresh] and -refresh refresh from the command line into
 * *width_out/*height_out/*refresh_out (any of which may be NULL). Returns 1 if either switch was present.
 *
 * Registers: unaff_ESI -> height_out, stack -> (width_out, refresh_out)
 *
 * @address 0x5168c0
 */
uint8_t rasterizer_parse_vidmode_commandline(int32_t *width_out, int32_t *height_out, long *refresh_out)
{
    uint8_t found = 0;
    const char *value;
    int32_t width = 800;
    int32_t height = 600;
    long refresh = 0x3c;

    if (halo::shell::command_line_check_flag("-vidmode", &value) != 0 && value != nullptr) {
        int32_t parsed = sscanf(value, "%d,%d,%d", &width, &height, &refresh);
        if (parsed == 2 || parsed == 3) {
            if (parsed == 3 && refresh_out != (long *)0) {
                *refresh_out = refresh;
            }
            if (width_out != nullptr) {
                *width_out = width;
            }
            if (height_out != nullptr) {
                *height_out = height;
            }
            if (halo::rasterizer::fields::video_mode_command_line_parsed == 0) {
                rasterizer_needs_reset = 1;
            }
            found = 1;
        }
    }

    if (halo::shell::command_line_check_flag("-refresh", &value) != 0) {
        refresh = (value == nullptr) ? 0 : atol(value);
        if (refresh_out != (long *)0) {
            *refresh_out = refresh;
        }
        found = 1;
    }

    if (halo::rasterizer::fields::video_mode_command_line_parsed == 0) {
        if (refresh == 0) {
            video_force_mode_flag = 1;
        }
        halo::rasterizer::fields::video_mode_command_line_parsed = 1;
    }
    return found;
}

namespace rasterizer_reset_device_if_needed_impl {


/**
 * Resets pre-ps_1_1 debug toggles, resets the D3D device if it was flagged lost, and reports whether the
 * device is usable afterward (also flagging a deferred update when it is).
 *
 * @address 0x517500
 */
uint8_t rasterizer_reset_device_if_needed(void)
{
    uint8_t usable = 1;
    int32_t hr;

    if (rasterizer_caps.pixel_shader_version < halo::d3d9::k_pixel_shader_version_1_1) {
        halo::rasterizer::fields::specular_projected_light_enabled = 0;
        halo::rasterizer::fields::specular_lightmap_enabled = 0;
        halo::rasterizer::fields::environment_multipurpose_enabled = 0;
        console_debug_toggle_6893f9 = 0;
        halo::rasterizer::fields::device_reset_cleared_flag = 0;
        halo::rasterizer::fields::fog_screen_overlay_enabled = 0;
        halo::rasterizer::fields::object_shadows_enabled = 0;
    }

    if (rasterizer_device_lost != 0) {
        d3d_present_parameters present_params_copy = rasterizer_present_parameters;

        rasterizer_device_lost = (uint8_t)(1 - (rasterizer_device_reset(&present_params_copy) != 0));
        usable = (rasterizer_device_lost == 0);
        if (rasterizer_device_lost != 0) {
            return usable;
        }
    }

    hr = render_device().begin_scene();
    if (hr < 0) {
        return 0;
    }
    if (usable == 0) {
        return 0;
    }
    rasterizer_in_scene = 1;
    return usable;
}

}  // namespace rasterizer_reset_device_if_needed_impl

/**
 * Repositions/resizes the game window to a centered `width` by `height` client area if that
 * differs from its current adjusted rect, then updates the cached client-area/mouse-bound globals.
 *
 * Registers: EAX -> height, ECX -> width
 *
 * @address 0x515b20
 */
void rasterizer_resize_game_window(int32_t height, int32_t width)
{
    halo::platform::window_center(halo::shell::globals().window, width, height);

    game_window_bottom_right = (uint16_t)(int16_t)height | ((uint32_t)(uint16_t)(int16_t)width << 16);
    halo::rasterizer::fields::game_screen_rect_bottom = (int16_t)height - 8;
    halo::rasterizer::fields::game_screen_rect_right = (int16_t)width - 8;
    game_window_top_left = 0;
    halo::rasterizer::fields::game_screen_rect_left = 8;
    game_screen_rect = 8;
    rasterizer_present_counter_low = 1;
    rasterizer_present_counter_high = 0;
}

/**
 * Rounds an input vertical screen resolution up to the nearest value in a table of common display heights.
 *
 * Registers: EAX -> height
 *
 * @address 0x5195f0
 */
int32_t rasterizer_round_up_resolution_height(int32_t height)
{
    if (height < 576) return 480;
    if (height < 577) return 576;
    if (height < 601) return 600;
    if (height < 721) return 720;
    if (height < 769) return 768;
    if (height < 865) return 864;
    if (height < 901) return 900;
    if (height < 961) return 960;
    if (height < 1025) return 1024;
    if (height < 1051) return 1050;
    if (height < 1081) return 1080;
    if (height < 1201) return 1200;
    if (height < 1441) return 1440;
    if (height <= 1600) return 1600;
    return 2160;
}

/**
 * The light cone draw installed on hardware without pixel shaders: draws nothing.
 *
 * @address 0x44ad80
 */
static void light_cone_draw_nothing(const ShaderEnvironment *, int16_t, int32_t, int32_t, int32_t, rasterizer_vertex_buffer *)
{
}

/**
 * Selects vendor/driver-specific rendering code path function pointers based on the detected GPU capability
 * caps (max_streams, pixel_shader_version).
 *
 * @address 0x516810
 */
void rasterizer_select_hardware_codepaths(void)
{
    if (rasterizer_caps.max_streams < 2) {
        halo::rasterizer::fields::environment_self_illumination_draw = rasterizer_shader_environment_self_illumination_draw_single_stream;
    } else {
        halo::rasterizer::fields::environment_self_illumination_draw = rasterizer_shader_environment_self_illumination_draw_two_stream;
        if (rasterizer_caps.pixel_shader_version > halo::d3d9::k_pixel_shader_version_1_0) {
            halo::rasterizer::fields::environment_self_illumination_draw = rasterizer_shader_environment_self_illumination_draw;
        }
    }

    rasterizer_glass_draw_procedures_select();
    rasterizer_shader_environment_select_draw_functions();

    if (rasterizer_caps.max_streams < 2) {
        halo::rasterizer::fields::environment_lightmap_draw = rasterizer_shader_environment_lightmap_draw_single_stream;
    } else {
        halo::rasterizer::fields::environment_lightmap_draw = rasterizer_shader_environment_lightmap_draw_two_stream;
        if (rasterizer_caps.pixel_shader_version > halo::d3d9::k_pixel_shader_version_1_0) {
            halo::rasterizer::fields::environment_lightmap_draw = rasterizer_shader_environment_lightmap_draw;
        }
    }
    if (rasterizer_caps.pixel_shader_version > halo::d3d9::k_pixel_shader_version_1_0) {
        halo::rasterizer::fields::light_cone_draw = rasterizer_light_cone_draw;
    } else {
        halo::rasterizer::fields::light_cone_draw = light_cone_draw_nothing;
    }

    rasterizer_water_draw_procedure = rasterizer_water_draw_fixed_function;
    if (rasterizer_caps.pixel_shader_version > halo::d3d9::k_pixel_shader_version_1_0) {
        rasterizer_water_draw_procedure = rasterizer_water_draw_pixel_shader;
    }
}

namespace rasterizer_service_deferred_windowed_ops_impl {


/**
 * While windowed and the device exists: performs a deferred present/update (+0xa8) if flagged, and a deferred
 * clear (+0x44, all zero args) if flagged, clearing each flag afterward.
 *
 * @address 0x5180d0
 */
void rasterizer_service_deferred_windowed_ops(void)
{

    if (rasterizer_fullscreen == 0 || rasterizer_device == nullptr) {
        return;
    }
    if (rasterizer_in_scene != 0) {
        render_device().end_scene();
        rasterizer_in_scene = 0;
    }
    if (rasterizer_pending_clear != 0) {
        render_device().present(0, 0, 0, 0);
        rasterizer_pending_clear = (rasterizer_pending_clear == 0);
    }
}

}  // namespace rasterizer_service_deferred_windowed_ops_impl

namespace rasterizer_set_default_render_states_impl {


static void set_render_state(uint32_t state, uint32_t value)
{
    render_device().set_render_state(state, value);
}

static void set_render_states(const uint32_t pairs[][2], int count)
{
    int i;
    for (i = 0; i < count; i++) {
        set_render_state(pairs[i][0], pairs[i][1]);
    }
}

/**
 * Initializes the D3D device's render states and texture stage states to their default values after device
 * creation/reset.
 *
 * @address 0x5160d0
 */
void rasterizer_set_default_render_states(void)
{
    static const uint32_t table1[][2] = {
        {7, 1}, {0xe, 1}, {0x17, 4},
    };
    static const uint32_t table2[][2] = {
        {0x34, 0}, {0x35, 1}, {0x36, 1}, {0x37, 1}, {0x38, 8}, {0x39, 0}, {0x3a, 0}, {0x3b, 0},
        {0xf, 0}, {0x19, 5}, {0x18, 0}, {0x1b, 0}, {0x13, 2}, {0x14, 1}, {0xab, 1}, {0xce, 0},
        {0xcf, 2}, {0xd0, 1}, {0xd1, 1}, {0x1c, 0},
    };
    static const uint32_t table3[][2] = {
        {0x22, 0}, {0x23, 0}, {0x8c, 0}, {0x24, 0}, {0x25, 0}, {0x26, 0}, {0x9c, 1}, {0x9d, 0},
        {0x9a, 0}, {0x9b, 0}, {0xa6, 0}, {0x9e, 0}, {0x9f, 0}, {0xa0, 0}, {0xa8, 0xf}, {9, 2},
        {0x16, 3}, {8, 3}, {0x10, 0}, {0x1a, 0}, {0xa1, 1}, {0xa2, 0}, {0xa3, 0}, {0xa5, 0},
        {0x88, 1}, {0x98, 0}, {0x80, 0}, {0x81, 0}, {0x82, 0}, {0x83, 0}, {0x84, 0}, {0x85, 0},
        {0x86, 0}, {0x87, 0}, {0xc6, 0}, {199, 0}, {200, 0}, {0xc9, 0}, {0xca, 0}, {0xcb, 0},
        {0xcc, 0}, {0xcd, 0}, {0x3c, 0}, {0xc2, 0}, {0x97, 0}, {0xa7, 0}, {0xaa, 0}, {0x89, 0},
        {0x8b, 0}, {0x1d, 0}, {0x8d, 0}, {0x8e, 0}, {0x8f, 0}, {0x93, 0}, {0x91, 0}, {0x92, 0},
        {0x94, 0},
    };

    set_render_states(table1, sizeof(table1) / sizeof(table1[0]));

    rasterizer_clear_decal_zbias();

    set_render_states(table2, sizeof(table2) / sizeof(table2[0]));

    set_render_state(0x30, (rasterizer_caps.raster_caps >> 0x10) & 1);

    set_render_states(table3, sizeof(table3) / sizeof(table3[0]));

    {
        render_device().set_texture_stage_state(0, halo::d3d9::ts::texcoord_index, 0);
        render_device().set_texture_stage_state(1, halo::d3d9::ts::texcoord_index, 1);
        render_device().set_texture_stage_state(2, halo::d3d9::ts::texcoord_index, 2);
        render_device().set_texture_stage_state(3, halo::d3d9::ts::texcoord_index, 3);
        render_device().set_texture_stage_state(0, halo::d3d9::ts::texture_transform_flags, 0);
        render_device().set_texture_stage_state(1, halo::d3d9::ts::texture_transform_flags, 0);
        render_device().set_texture_stage_state(2, halo::d3d9::ts::texture_transform_flags, 0);
        render_device().set_texture_stage_state(3, halo::d3d9::ts::texture_transform_flags, 0);
    }
}

}  // namespace rasterizer_set_default_render_states_impl

/**
 * Full rasterizer teardown: releases every cached D3D resource, destroys the window, and releases the Direct3D
 * device and Direct3D9 object.
 *
 * @address 0x518450
 */
void rasterizer_shutdown(void)
{
    int32_t i;

    if (rasterizer_scratch_memory != nullptr) {
        halo::platform::heap_free(rasterizer_scratch_memory);
    }
    rasterizer_scratch_memory = nullptr;
    rasterizer_scratch_memory_used = 0;

    rasterizer_dynamic_geometry_dispose();
    chimera__rasterizer_dispose_free_memory();

    if (g_font_glyph_cache.initialized != 0) {
        font_glyph_cache_clear_all();
        halo::bitmaps::bitmap_data_view((BitmapData *)g_font_glyph_cache.atlas).free();
        g_font_glyph_cache.initialized = 0;
    }

    if (rasterizer_device != nullptr && rasterizer_detail_object_vertex_buffer != nullptr) {
        render_device().release(rasterizer_detail_object_vertex_buffer);
        rasterizer_detail_object_vertex_buffer = nullptr;
    }

    rasterizer_dx9_shaders_release_all();
    rasterizer_render_target_dispose();

    for (i = 0; i < k_lens_flare_occlusion_queries; i++) {
        if (lens_flare_occlusion_queries[i] != nullptr) {
            render_device().release(lens_flare_occlusion_queries[i]);
            lens_flare_occlusion_queries[i] = nullptr;
        }
    }

    chimera__registry_check_3();

    halo::platform::window_destroy(halo::shell::globals().window);
    halo::shell::globals().window = nullptr;

    for (i = 0; i < 4; i++) {
        if (rasterizer_capture_surfaces[i] != nullptr) {
            render_device().release(rasterizer_capture_surfaces[i]);
            rasterizer_capture_surfaces[i] = nullptr;
        }
    }

    if (rasterizer_device != nullptr) {
        render_device().release(rasterizer_device);
    }
    rasterizer_device = nullptr;

    if (rasterizer_direct3d != nullptr) {
        render_device().release(rasterizer_direct3d);
    }
    rasterizer_direct3d = nullptr;
}

}  // namespace halo::rasterizer
