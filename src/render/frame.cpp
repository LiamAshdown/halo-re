#include "halo/rasterizer/globals.hpp"
#include "crt.h"
#include "halo/scenario/api.hpp"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "effects.h"
#include "rasterizer.h"
#include "interface.h"
#include "structures.h"
#include "cutscene.h"
#include "shaders.h"
#include "render.h"
#include <stdint.h>
#include "halo/render/render.hpp"
#include "halo/memory/api.hpp"
#include "halo/structures/api.hpp"
#include "halo/cutscene/api.hpp"
#include "halo/render/layout.hpp"
#include "halo/render/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/scenario/scenario.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/cutscene/vars.hpp"
#include "halo/interface/vars.hpp"
#include "halo/main/vars.hpp"
#include "halo/render/vars.hpp"
#include "halo/main/api.hpp"

static auto &render_frame_index = halo::link::ref<int32_t>(halo::render::vars().render_frame_index);
static auto &render_time_since_tick = halo::link::ref<float>(halo::render::vars().render_time_since_tick);
static auto &render_time_since_frame = halo::link::ref<float>(halo::render::vars().render_time_since_frame);
static auto &render_window_index = halo::link::ref<int16_t>(halo::render::vars().render_window_index);
static auto &screenshot_scale = halo::link::ref<int16_t>(halo::main::vars().screenshot_scale);
static auto &rasterizer_model_ambient_reflection_tint = halo::link::ref<ColorARGB *>(halo::cutscene::vars().rasterizer_model_ambient_reflection_tint);
static auto &render_camera_global = halo::link::ref<render_camera>(halo::render::vars().render_camera_global);
static auto &render_frustum_global = halo::link::ref<render_frustum>(halo::render::vars().render_frustum_global);
static auto &render_fog_state = halo::link::ref<render_fog>(halo::render::vars().render_fog_state);
static auto &render_clip_warning = halo::link::ref<uint8_t>(halo::render::vars().render_clip_warning);
static auto &rasterizer_device_version = halo::link::ref<uint32_t>(halo::ui::vars().rasterizer_device_version);
static auto &rasterizer_caps_flag_68a = halo::link::ref<uint8_t>(halo::ui::vars().rasterizer_caps_flag_68a);

namespace halo::render::frame {

/**
 * Top-level per-frame render dispatcher: bumps the frame index and latches the tick/frame time
 * globals, dispatches the cinematic screen effect for the elapsed time, resets the rasterizer
 * device if needed, then draws every split-screen viewport (its 3D scene through
 * render_player_frame, or its placeholder through render_nonplayer_frame), combining the
 * screenshot page and tile indices into the one actually handed to render_player_frame. Finally
 * draws the "trouble brewing" indicator and the rasterizer's own per-frame globals/debug pass.
 *
 * @address 0x0050bea0
 */
void draw(Point2DInt *screenshot_tile, render_view *views, int16_t count, Point2DInt *screenshot_page,
    float time_since_tick, float time_since_frame)
{
    rasterizer_frame_time frame_time;
    int16_t i;

    render_frame_index = render_frame_index + 1;
    render_time_since_tick = time_since_tick;
    render_time_since_frame = time_since_frame;

    frame_time.unknown_08 = 0;
    frame_time.unknown_0c = 0;
    frame_time.time = (double)halo::game::globals().game_time->game_time * (1.0 / 30.0) + (double)time_since_tick;
    halo::render::render_cinematic_screen_effect_update(&frame_time);

    if (!halo::rasterizer::rasterizer_reset_device_if_needed()) {
        return;
    }

    for (i = 0; i < count; i++) {
        render_view *view = &views[i];

        render_window_index = i;

        if (view->nonplayer != 0) {
            halo::render::render_nonplayer_frame(0, view);
        } else if (view->local_player_index == -1) {
            halo::render::render_nonplayer_frame(1, view);
        } else {
            Point2DInt combined_tile;
            Point2DInt *tile_argument = 0;

            if (screenshot_tile != 0 && screenshot_page != 0) {
                combined_tile.x = (int16_t)(screenshot_page->x * screenshot_scale + screenshot_tile->x);
                combined_tile.y = (int16_t)(screenshot_page->y * screenshot_scale + screenshot_tile->y);
            }
            if (screenshot_tile != 0) {
                tile_argument = &combined_tile;
            }
            halo::render::render_player_frame(tile_argument, view);
        }
    }

    halo::interface::ui_draw_trouble_brewing_indicator();
    halo::rasterizer::rasterizer_end_frame();
    halo::rasterizer::rasterizer_unbind_stream_and_textures();
}

/**
 * Carves the 0x10 byte model ambient reflection tint block out of the game state arena and
 * initializes the Direct3D rasterizer device.
 *
 * @address 0x00511da0
 */
uint8_t initialize(void)
{
    int32_t block_size = k_model_ambient_reflection_tint_size;

    rasterizer_model_ambient_reflection_tint =
        (ColorARGB *)(halo::saved_games::globals().game_state_base + halo::saved_games::globals().game_state_cursor);
    halo::saved_games::globals().game_state_cursor = halo::saved_games::globals().game_state_cursor + k_model_ambient_reflection_tint_size;
    halo::memory::crc32_update(&halo::saved_games::globals().game_state_crc, (uint8_t *)&block_size, 4);
    return halo::rasterizer::rasterizer_initialize_direct3d();
}

/**
 * Draws the non-3D placeholder pass for a window whose camera/unit is not valid: latches the
 * view's two cameras into the global render camera/frustum and a bare rasterizer_window_parameters
 * (no fog, no mirror) and begins the frame, then either shows the loading screen / letterbox /
 * console overlay (nonplayer == 0) or runs the alternate non-player path, and finally draws the
 * framerate statistics overlay if the window that ended up current is the special (-1) one.
 *
 * @address 0x0050bdc0
 */
void nonplayer_frame(uint32_t nonplayer, render_view *view)
{
    rasterizer_window_parameters params = {0};

    render_camera_global = view->source_camera;
    halo::render::chimera__render_camera_build_frustum(0, &render_camera_global, &render_frustum_global, 1);

    params.camera = view->rasterizer_camera;
    halo::render::chimera__render_camera_build_frustum(0, &params.camera, &params.frustum, 1);

    params.type = 1;
    params.window_index = -1;
    params.clear_target = (uint8_t)(nonplayer == 0);

    halo::rasterizer::rasterizer_begin_frame(&params);

    if (nonplayer == 0) {
        halo::cutscene::chimera__letterbox();
        halo::interface::ui_error_modal_update();
        halo::interface::hud_timer_draw();
        halo::interface::chimera__do_show_loading_screen();
        halo::interface::console_draw_overlay();
    } else {
        halo::game::game_engine_maybe_render_post_game();
    }

    if (halo::rasterizer::globals().window.window_index == -1) {
        halo::render::rasterizer_frame_statistics_sample(&halo::rasterizer::globals().frame_statistics, 0);
        halo::render::rasterizer_frame_statistics_draw();
    }
}

/**
 * Draws one local player's window: resolves the camera's leaf/cluster and fog, clamps the far
 * clip plane to the fog and to the camera's own near plane, computes the asymmetric projection
 * bounds (subdividing them into a screenshot tile when one was requested), builds the source and
 * rasterizer frustums, then attempts a mirror/portal reflection pass before drawing the main
 * scene.
 *
 * @address 0x0050ba80
 */
void player_frame(Point2DInt *screenshot_tile, render_view *view)
{
    render_camera *source_camera = &view->source_camera;
    uint8_t has_mirror = 0;
    float frustum_bounds[4];
    render_frustum source_frustum;
    render_frustum rasterizer_frustum;
    uint8_t attempt_mirror;

    halo::structures::render_camera_update_leaf_and_cluster(&source_camera->position);

    render_fog_state.unknown_02 = 0;
    halo::scenario::scenario_query::sky_fog_state_update(halo::structures::globals().render_cluster_sky_index, view->local_player_index, &source_camera->position, &render_fog_state);

    halo::structures::structure_bsp_build_fog_environment(halo::structures::globals().render_cluster_index,
                                         (structure_fog_environment *)&render_fog_state);

    if (render_fog_state.atmospheric_maximum_distance != 0.0f && halo::structures::globals().render_cluster_sky_index == -1 &&
        render_fog_state.atmospheric_maximum_distance < render_fog_state.planar_maximum_distance) {
        render_fog_state.planar_maximum_distance = render_fog_state.atmospheric_maximum_distance;
    }
    if (render_fog_state.atmospheric_maximum_density == 1.0f &&
        render_fog_state.atmospheric_maximum_distance != 0.0f) {
        float z_far = render_fog_state.atmospheric_maximum_distance;
        if (source_camera->z_far <= render_fog_state.atmospheric_maximum_distance) {
            z_far = source_camera->z_far;
        }
        source_camera->z_far = z_far;
    }
    if (render_fog_state.planar_mode == 2 && render_fog_state.planar_maximum_distance != 0.0f) {
        float z_far = render_fog_state.planar_maximum_distance;
        if (source_camera->z_far <= render_fog_state.planar_maximum_distance) {
            z_far = source_camera->z_far;
        }
        source_camera->z_far = z_far;
    }
    if (source_camera->z_far <= source_camera->z_near) {
        if (!render_clip_warning) {
            render_clip_warning = 1;
        }
        source_camera->z_far = source_camera->z_near + 0.01f;
    }

    halo::render::render_camera_compute_projection_skew(source_camera, frustum_bounds);

    if (screenshot_tile != 0) {
        int32_t tile_total = (int32_t)screenshot_scale * (int32_t)halo::rasterizer::fields::screenshot_tile_count;
        if (tile_total > 0) {
            float step_x = (frustum_bounds[1] - frustum_bounds[0]) / (float)tile_total;
            float step_y = (frustum_bounds[3] - frustum_bounds[2]) / (float)tile_total;
            float x0 = (float)screenshot_tile->x * step_x + frustum_bounds[0];
            float y0 = (float)(tile_total - screenshot_tile->y - 1) * step_y + frustum_bounds[2];
            frustum_bounds[0] = x0;
            frustum_bounds[2] = y0;
            frustum_bounds[1] = x0 + step_x;
            frustum_bounds[3] = y0 + step_y;
        }
    }

    halo::render::chimera__render_camera_build_frustum(frustum_bounds, source_camera, &source_frustum, 1);
    halo::render::chimera__render_camera_build_frustum(frustum_bounds, &view->rasterizer_camera,
                                          &rasterizer_frustum, 1);

    attempt_mirror = 1;
    if (!(halo::game::globals().current_engine != 0 && halo::game::globals().state >= _game_engine_state_ended &&
          halo::game::globals().state <= _game_engine_state_post_game)) {
        if (halo::cutscene::globals().cinematic_globals->in_progress == 0) {
            int16_t local_player_count = halo::game::globals().local_player_globals->local_player_count;
            if (local_player_count == 1 && local_player_count != 1) {
                attempt_mirror = 0;
            }
        }
    }

    if (attempt_mirror) {
        structure_bsp_mirror_result mirror_result;
        if (halo::structures::structure_bsp_mirror_query(source_camera, &source_frustum, &mirror_result) &&
            rasterizer_device_version > k_device_version_mirror_pass && !rasterizer_caps_flag_68a) {
            render_camera mirror_camera;
            render_frustum mirror_frustum;
            int32_t saved_cluster_index = halo::structures::globals().render_cluster_index;

            halo::render::render_camera_mirror(source_camera, &mirror_result, &mirror_camera);
            halo::render::chimera__render_camera_build_frustum(frustum_bounds, &mirror_camera, &mirror_frustum, 1);
            halo::structures::globals().render_cluster_index = mirror_result.cluster_index;
            halo::render::render_window(-1, &mirror_camera, &mirror_frustum, &mirror_camera, &mirror_frustum,
                          _render_target_mirror, 0);
            halo::structures::globals().render_cluster_index = saved_cluster_index;
            has_mirror = 1;
        }
    }

    halo::render::render_window(view->local_player_index, source_camera, &source_frustum,
                  &view->rasterizer_camera, &rasterizer_frustum, _render_target_main, has_mirror);
}

/**
 * Draws a frame with no scene at all (no active camera, e.g. before a scenario is loaded):
 * latches the view's two cameras into the global render camera/frustum and a bare
 * rasterizer_window_parameters (type 1 only; window_index and clear_target stay 0), begins the
 * frame, shows the loading screen, and draws the framerate statistics overlay if the window that
 * ended up current is the special (-1) one.
 *
 * @address 0x0050c590
 */
void pregame_frame(render_view *view)
{
    rasterizer_window_parameters params = {0};
    rasterizer_frame_time frame_time;

    render_frame_index = render_frame_index + 1;
    halo::render::render_cinematic_screen_effect_update(&frame_time);

    if (!halo::rasterizer::rasterizer_reset_device_if_needed()) {
        return;
    }

    render_camera_global = view->source_camera;
    halo::render::chimera__render_camera_build_frustum(0, &render_camera_global, &render_frustum_global, 1);

    params.camera = view->rasterizer_camera;
    halo::render::chimera__render_camera_build_frustum(0, &params.camera, &params.frustum, 1);

    params.type = 1;
    halo::rasterizer::rasterizer_begin_frame(&params);

    halo::interface::widget_draw_fullscreen_region(0);
    halo::interface::chimera__do_show_loading_screen();

    if (halo::rasterizer::globals().window.window_index == -1) {
        halo::render::rasterizer_frame_statistics_sample(&halo::rasterizer::globals().frame_statistics, 0);
        halo::render::rasterizer_frame_statistics_draw();
    }

    halo::rasterizer::rasterizer_end_frame();
    halo::rasterizer::rasterizer_unbind_stream_and_textures();
}

}  // namespace halo::render::frame
