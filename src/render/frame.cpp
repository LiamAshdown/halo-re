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

extern "C" {
extern int32_t render_frame_index;
extern float render_time_since_tick;
extern float render_time_since_frame;
extern int16_t render_window_index;
extern int16_t screenshot_scale;
extern game_time_globals *game_time;
extern uint8_t rasterizer_reset_device_if_needed(void);
extern void ui_draw_trouble_brewing_indicator(void);
extern void rasterizer_end_frame(void);
extern void rasterizer_unbind_stream_and_textures(void);
extern uint8_t *game_state_base;
extern int32_t game_state_cursor;
extern uint32_t game_state_crc;
extern ColorARGB *rasterizer_model_ambient_reflection_tint;
extern uint8_t rasterizer_initialize_direct3d(void);
extern render_camera render_camera_global;
extern render_frustum render_frustum_global;
extern rasterizer_window_parameters rasterizer_window;
extern void rasterizer_begin_frame(rasterizer_window_parameters *source);
extern void chimera__letterbox(void);
extern void ui_error_modal_update(void);
extern void hud_timer_draw(void);
extern void chimera__do_show_loading_screen(void);
extern void console_draw_overlay(void);
extern void game_engine_maybe_render_post_game(void);
extern rasterizer_frame_statistics rasterizer_frame_statistics_state;
extern render_fog render_fog_state;
extern uint8_t render_clip_warning;
extern uint32_t rasterizer_device_version;
extern uint8_t rasterizer_caps_flag_68a;
extern game_engine_definition *current_game_engine;
extern game_engine_state game_engine_state_value;
extern player_globals *local_player_globals;
extern cinematic_globals *cinematic_globals_ptr;
extern int16_t unknown_00719aac;
extern void halo::scenario::scenario_sky_fog_state_update(int16_t sky_index, int16_t local_player_index,
    real_point3d *camera_position, render_fog *out);
extern void widget_draw_fullscreen_region(int16_t controller_index);
}

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
    frame_time.time = (double)game_time->game_time * (1.0 / 30.0) + (double)time_since_tick;
    render_cinematic_screen_effect_update(&frame_time);

    if (!rasterizer_reset_device_if_needed()) {
        return;
    }

    for (i = 0; i < count; i++) {
        render_view *view = &views[i];

        render_window_index = i;

        if (view->nonplayer != 0) {
            render_nonplayer_frame(0, view);
        } else if (view->local_player_index == -1) {
            render_nonplayer_frame(1, view);
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
            render_player_frame(tile_argument, view);
        }
    }

    ui_draw_trouble_brewing_indicator();
    rasterizer_end_frame();
    rasterizer_unbind_stream_and_textures();
}

/**
 * Carves the 0x10 byte model ambient reflection tint block out of the game state arena and
 * initializes the Direct3D rasterizer device.
 *
 * @address 0x00511da0
 */
uint8_t initialize(void)
{
    int32_t block_size = 0x10;

    rasterizer_model_ambient_reflection_tint =
        (ColorARGB *)(game_state_base + game_state_cursor);
    game_state_cursor = game_state_cursor + 0x10;
    halo::memory::crc32_update(&game_state_crc, (uint8_t *)&block_size, 4);
    return rasterizer_initialize_direct3d();
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
    chimera__render_camera_build_frustum(0, &render_camera_global, &render_frustum_global, 1);

    params.camera = view->rasterizer_camera;
    chimera__render_camera_build_frustum(0, &params.camera, &params.frustum, 1);

    params.type = 1;
    params.window_index = -1;
    params.clear_target = (uint8_t)(nonplayer == 0);

    rasterizer_begin_frame(&params);

    if (nonplayer == 0) {
        chimera__letterbox();
        ui_error_modal_update();
        hud_timer_draw();
        chimera__do_show_loading_screen();
        console_draw_overlay();
    } else {
        game_engine_maybe_render_post_game();
    }

    if (rasterizer_window.window_index == -1) {
        rasterizer_frame_statistics_sample(&rasterizer_frame_statistics_state, 0);
        rasterizer_frame_statistics_draw();
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
    halo::scenario::scenario_sky_fog_state_update(halo::structures::globals().render_cluster_sky_index, view->local_player_index, &source_camera->position,
                                  &render_fog_state);

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

    render_camera_compute_projection_skew(source_camera, frustum_bounds);

    if (screenshot_tile != 0) {
        int32_t tile_total = (int32_t)screenshot_scale * (int32_t)unknown_00719aac;
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

    chimera__render_camera_build_frustum(frustum_bounds, source_camera, &source_frustum, 1);
    chimera__render_camera_build_frustum(frustum_bounds, &view->rasterizer_camera,
                                          &rasterizer_frustum, 1);

    attempt_mirror = 1;
    if (!(current_game_engine != 0 && game_engine_state_value >= _game_engine_state_ended &&
          game_engine_state_value <= _game_engine_state_post_game)) {
        if (cinematic_globals_ptr->in_progress == 0) {
            int16_t local_player_count = local_player_globals->local_player_count;
            if (local_player_count == 1 && local_player_count != 1) {
                attempt_mirror = 0;
            }
        }
    }

    if (attempt_mirror) {
        structure_bsp_mirror_result mirror_result;
        if (halo::structures::structure_bsp_mirror_query(source_camera, &source_frustum, &mirror_result) &&
            rasterizer_device_version > 0xffff0100 && !rasterizer_caps_flag_68a) {
            render_camera mirror_camera;
            render_frustum mirror_frustum;
            int32_t saved_cluster_index = halo::structures::globals().render_cluster_index;

            render_camera_mirror(source_camera, &mirror_result, &mirror_camera);
            chimera__render_camera_build_frustum(frustum_bounds, &mirror_camera, &mirror_frustum, 1);
            halo::structures::globals().render_cluster_index = mirror_result.cluster_index;
            render_window(-1, &mirror_camera, &mirror_frustum, &mirror_camera, &mirror_frustum,
                          _render_target_mirror, 0);
            halo::structures::globals().render_cluster_index = saved_cluster_index;
            has_mirror = 1;
        }
    }

    render_window(view->local_player_index, source_camera, &source_frustum,
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
    render_cinematic_screen_effect_update(&frame_time);

    if (!rasterizer_reset_device_if_needed()) {
        return;
    }

    render_camera_global = view->source_camera;
    chimera__render_camera_build_frustum(0, &render_camera_global, &render_frustum_global, 1);

    params.camera = view->rasterizer_camera;
    chimera__render_camera_build_frustum(0, &params.camera, &params.frustum, 1);

    params.type = 1;
    rasterizer_begin_frame(&params);

    widget_draw_fullscreen_region(0);
    chimera__do_show_loading_screen();

    if (rasterizer_window.window_index == -1) {
        rasterizer_frame_statistics_sample(&rasterizer_frame_statistics_state, 0);
        rasterizer_frame_statistics_draw();
    }

    rasterizer_end_frame();
    rasterizer_unbind_stream_and_textures();
}

}  // namespace halo::render::frame
