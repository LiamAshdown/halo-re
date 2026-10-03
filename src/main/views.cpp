/**
 * Render view setup, split-screen rectangles and screenshots.
 */

#include "crt.h"
#include "halo/bitmaps/api.hpp"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "cache.h"
#include "game.h"
#include "camera.h"
#include "rasterizer.h"
#include "render.h"
#include "networking.h"
#include "saved_games.h"
#include "input.h"
#include "main.h"
#include <string.h>
#include "units.h"
#include "cutscene.h"
#include "win32.h"
#include "hs.h"
#include <stdio.h>

#include "halo/main/views.hpp"
#include "halo/math/api.hpp"
#include "halo/sound/api.hpp"
#include "halo/effects/api.hpp"
#include "halo/cutscene/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/main/layout.hpp"
#include "halo/render/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/main/api.hpp"
#include "halo/rasterizer/api.hpp"
#include "halo/bitmaps/bitmaps.hpp"


extern "C" { extern main_globals main_globals_data; }
extern "C" { extern render_view render_views[2]; }
extern "C" { extern game_engine_definition *current_game_engine; }
extern "C" { extern game_engine_state game_engine_state_value; }
extern "C" { extern player_globals *local_player_globals; }
extern "C" { extern uint8_t widget_memory_pool_valid; }
extern "C" { extern widget_instance *ui_root_widget[1]; }
extern "C" { extern uint8_t render_view_local_player_sticky; }
extern "C" { extern int32_t screenshots; }
extern "C" { extern input_abstraction_globals input_globals; }
extern "C" { extern const real_point3d *global_zero_vector3d_pointer; }
extern "C" { extern uint8_t unknown_00873d30; }
extern "C" { extern double tan(double x); }
extern "C" { extern double atan2(double y, double x); }
extern "C" { extern void halo::sound::sound_update(void); }
namespace halo::main {

/**
 * Builds this frame's render_views array (one entry per local player, plus a trailing
 * non-player view) and dispatches to render_frame (or, when a screenshot is pending, to
 * screenshot_render instead). Each local player view's camera is filled by
 * render_view_camera_fill from that player's observer, unless the game engine is showing
 * end-of-game results for 2..3 local players (in which case every view is forced to the
 * trailing non-player camera instead, one entry only).
 *
 * @address 0x4c9260
 */
void RenderViews::frame_all_views(float time_since_tick, float time_since_frame)
{
    uint8_t showing_results;
    int16_t local_player_count_field;
    int32_t view_count;
    int32_t i;
    int16_t resolved_local_player_index;
    render_view *view;

    halo::effects::globals().player_effect_reentry_count = halo::effects::globals().player_effect_reentry_count + 1;
    halo::sound::sound_update();

    showing_results = (current_game_engine != 0 && (int32_t)game_engine_state_value > 1 &&
                        (int32_t)game_engine_state_value < 4)
                          ? 1
                          : 0;

    local_player_count_field = local_player_globals->local_player_count;
    view_count = 1;

    if (widget_memory_pool_valid != 0 && ui_root_widget[0] != 0) {
        strstr(ui_root_widget[0]->name, "error_modal");
    }
    if (showing_results || halo::cutscene::globals().cinematic_globals->in_progress != 0) {
        view_count = 1;
    }

    resolved_local_player_index = -1;
    for (i = 0; i < view_count; i++) {
        view = &render_views[i];
        halo::main::viewport_split_rect_compute(view_count, i, &view->rasterizer_camera.window_bounds,
                                     &view->rasterizer_camera.viewport_bounds);

        if (showing_results || i >= view_count) {
            view->local_player_index = -1;
        } else {
            int16_t candidate;

            if (render_view_local_player_sticky == 0 || resolved_local_player_index == -1) {
                if (main_globals_data.game_connection == 3) {
                    candidate = 0;
                } else {
                    candidate = -1;
                    if (local_player_globals->local_players[0] != k_datum_index_none &&
                        resolved_local_player_index < 0) {
                        candidate = 0;
                    }
                }
            } else {
                candidate = resolved_local_player_index;
            }
            view->local_player_index = candidate;
            resolved_local_player_index = candidate;
        }

        halo::main::render_view_camera_fill(view->local_player_index != -1
                                     ? &halo::camera::globals().observers[view->local_player_index].camera
                                     : 0,
                                 view);
        view->nonplayer = 0;
    }

    view = &render_views[view_count];
    halo::main::viewport_split_rect_compute(1, 0, &view->rasterizer_camera.window_bounds,
                                 &view->rasterizer_camera.viewport_bounds);
    view->local_player_index = -1;
    view->nonplayer = 1;
    view->rasterizer_camera.position = *global_zero_vector3d_pointer;
    view->rasterizer_camera.forward.i = halo::math::globals().global_forward3d_pointer->i;
    view->rasterizer_camera.forward.j = halo::math::globals().global_forward3d_pointer->j;
    view->rasterizer_camera.forward.k = halo::math::globals().global_forward3d_pointer->k;
    view->rasterizer_camera.up.i = halo::math::globals().global_up3d_pointer->i;
    view->rasterizer_camera.up.j = halo::math::globals().global_up3d_pointer->j;
    view->rasterizer_camera.up.k = halo::math::globals().global_up3d_pointer->k;
    view->rasterizer_camera.z_near = halo::rasterizer::globals().default_z_near;
    view->rasterizer_camera.mirrored = 0;
    view->rasterizer_camera.z_far = halo::rasterizer::globals().default_z_far;
    view->rasterizer_camera.vertical_field_of_view =
        (float)(2.0 * atan2(tan(0.6981316804885864) * 0.6375f, 1.0));
    if (unknown_00873d30 == 0) {
        view->source_camera = view->rasterizer_camera;
    }

    if (screenshots == 0) {
        halo::render::render_frame(0, render_views, (int16_t)(view_count + 1), 0, time_since_tick,
                     time_since_frame);
        halo::effects::globals().player_effect_reentry_count = halo::effects::globals().player_effect_reentry_count - 1;
        return;
    }

    if (input_globals.system_key_states[2] == 0 &&
        input_globals.states[0].buttons[_input_action_screenshot] == 0) {
        if (main_globals_data.screenshot_tile_count < 1) {
            halo::render::render_frame(0, render_views, (int16_t)(view_count + 1), 0, time_since_tick,
                         time_since_frame);
            halo::effects::globals().player_effect_reentry_count = halo::effects::globals().player_effect_reentry_count - 1;
            return;
        }
    } else {
        main_globals_data.screenshot_tile_count = 1;
    }
    halo::main::screenshot_render(render_views);
    halo::effects::globals().player_effect_reentry_count = halo::effects::globals().player_effect_reentry_count - 1;
}

}

namespace halo::main {

/**
 * Returns the number of local split-screen viewports to render this frame: normally 1, but the
 * raw local player count (local_player_globals->local_player_count) when no multiplayer game engine is
 * active (or its end-of-game state is outside the 2..3 "showing results" range), no cinematic is
 * suppressing it, and that count is a plausible 1 (this build never has more than one local
 * player, per k_maximum_local_players in types/game.h).
 *
 * @address 0x4c9220
 */
int RenderViews::local_view_count(void)
{
    int16_t local_player_count_field;

    if ((current_game_engine == 0 || (int32_t)game_engine_state_value < 2 ||
         (int32_t)game_engine_state_value > 3) &&
        halo::cutscene::globals().cinematic_globals->in_progress == 0) {
        local_player_count_field = local_player_globals->local_player_count;
        if (local_player_count_field > 0 && local_player_count_field < 2) {
            return local_player_count_field;
        }
    }
    return 1;
}

}

extern "C" { extern render_view pregame_render_view; }
namespace halo::main {

/**
 * Builds the default camera used before any scenario is loaded (looking down +Z, up +Y, at the
 * origin, 1x1 viewport split), stamps it into pregame_render_view as both its rasterizer and
 * source camera, marks the view as the non-player trailing entry, and renders one frame with it.
 *
 * @address 0x4c8f20
 */
void RenderViews::pregame_view_initialize(void)
{
    render_camera *camera = &pregame_render_view.rasterizer_camera;

    halo::sound::sound_update();

    camera->position.x = 0.0f;
    camera->position.y = 0.0f;
    camera->position.z = 0.0f;
    camera->forward.i = 0.0f;
    camera->forward.j = 0.0f;
    camera->forward.k = 1.0f;
    camera->up.i = 0.0f;
    camera->up.j = 1.0f;
    camera->up.k = 0.0f;

    pregame_render_view.local_player_index = -1;
    pregame_render_view.nonplayer = 1;
    camera->mirrored = 0;
    camera->vertical_field_of_view =
        (float)(2.0 * atan2(tan(0.6981316804885864) * 0.6375f, 1.0));

    halo::main::viewport_split_rect_compute(1, 0, &camera->window_bounds, &camera->viewport_bounds);

    camera->z_near = 0.01f;
    camera->z_far = 1.0f;

    pregame_render_view.source_camera = *camera;

    halo::render::render_pregame_frame(&pregame_render_view);
}

}

extern "C" { extern game_time_globals *game_time; }
extern "C" { extern console_globals console_globals_data; }
namespace halo::main {

/**
 * Fills view->rasterizer_camera's position/forward/up/field_of_view either from `observer`
 * (an observer_camera) or, when observer is NULL, from the engine's default zero/forward/up
 * vectors and a fixed 40-degree-ish field of view (identical formula to
 * src/main/render_pregame_view_initialize.c's default camera). When an observer is supplied and
 * the view belongs to a real local player (local_player_index != -1) with the console closed and
 * the game not paused, recomputes the field of view from the observer's own FOV adjusted to the
 * view's current aspect ratio, and -- unless the camera type is the dead-camera type (3) -- folds
 * in that player's camera shake by building an orientation matrix, multiplying it by the shake
 *
 * Original register convention: EAX -> observer, ECX -> view.
 *
 * @address 0x4c9050
 */
void RenderViews::view_camera_fill(observer_camera *observer, render_view *view)
{
    render_camera *camera = &view->rasterizer_camera;

    if (observer != 0) {
        camera->position.x = observer->position.x;
        camera->position.y = observer->position.y;
        camera->position.z = observer->position.z;
        camera->forward.i = observer->forward.i;
        camera->forward.j = observer->forward.j;
        camera->forward.k = observer->forward.k;
        camera->up.i = observer->up.i;
        camera->up.j = observer->up.j;
        camera->up.k = observer->up.k;

        {
            int32_t width = camera->viewport_bounds.right - camera->viewport_bounds.left;
            int32_t height = camera->viewport_bounds.bottom - camera->viewport_bounds.top;
            double half_fov_tan = tan((double)observer->field_of_view * 0.5);
            camera->vertical_field_of_view =
                (float)(2.0 * atan2(((double)height / (double)width) * half_fov_tan * 0.85f, 1.0));
        }

        if (view->local_player_index != -1 && console_globals_data.active == 0 &&
            game_time->paused == 0) {
            if (halo::camera::camera_get_type_for_player(view->local_player_index) != 3) {
                real_matrix4x3 shake_matrix;
                real_matrix4x3 orientation;

                halo::effects::player_effect_build_camera_shake_matrix(&shake_matrix, view->local_player_index);
                halo::math::matrix4x3_from_forward_up_position((real_vector3d *)&observer->up,
                                                    (real_vector3d *)&observer->forward,
                                                    *(real_point3d *)observer, &orientation);
                halo::math::matrix4x3_multiply(&orientation, &shake_matrix, &orientation);
                halo::math::matrix4x3_extract_forward_up_position(camera->up, camera->forward, orientation,
                                                       camera->position);
            }
        }
    } else {
        camera->position = *global_zero_vector3d_pointer;
        camera->forward = *halo::math::globals().global_forward3d_pointer;
        camera->up = *halo::math::globals().global_up3d_pointer;
        camera->vertical_field_of_view =
            (float)(2.0 * atan2(tan(0.6981316804885864) * 0.6375f, 1.0));
    }

    camera->z_far = halo::rasterizer::globals().default_z_far;
    camera->mirrored = 0;
    camera->z_near = halo::rasterizer::globals().default_z_near;

    if (unknown_00873d30 == 0) {
        view->source_camera = view->rasterizer_camera;
    }
}

}

extern "C" { extern int16_t screenshot_scale; }
extern "C" { extern Rectangle2D game_window_top_left; }
extern "C" { extern void console_print_error_va(uint8_t clear_first, const char *format, ...); }
extern "C" { extern void console_deactivate(void); }
extern "C" { extern void rasterizer_capture_and_present(const int16_t *tile, BitmapData *bitmap); }
extern "C" { extern void path_remove_last_component(uint8_t *path); }
namespace halo::main {

/**
 * Renders each active viewport tiled n by n (n = screenshot_scale, clamped to 1..3) into a
 * freshly allocated bitmap sized to the full game window at that scale, and saves each n by n
 * page as its own numbered "screenshots\\NNscreenshotROWCOL.tga" file. Clears
 * main_globals.screenshot_tile_count when the bitmap allocation fails; otherwise renders every
 * tile/page combination, exports each page, advances screenshot_index, and frees the bitmap
 * group.
 *
 * @address 0x4ca1a0
 */
void RenderViews::screenshot_render(render_view *views)
{
    int16_t width;
    int16_t height;
    BitmapData *bitmap;
    int16_t original_scale = screenshot_scale;
    int16_t page_row, page_col;

    if (original_scale < 1) {
        screenshot_scale = 1;
    } else {
        screenshot_scale = 3;
        if (original_scale < 4) {
            screenshot_scale = original_scale;
        }
    }

    height = (int16_t)((game_window_top_left.bottom - game_window_top_left.top) * screenshot_scale);
    width = (int16_t)((game_window_top_left.right - game_window_top_left.left) * screenshot_scale);

    bitmap = (BitmapData *)GlobalAlloc(0, sizeof(BitmapData));
    if (bitmap == 0) {
        main_globals_data.screenshot_tile_count = 0;
        return;
    }
    memset(bitmap, 0, sizeof(BitmapData));

    bitmap->bitmap_class = k_bitmap_group;
    bitmap->width = width;
    bitmap->height = height;
    bitmap->depth = 1;
    bitmap->type = 0;
    bitmap->format = bitmapdataformat_x8r8g8b8;
    bitmap->flags = to_bits(tags::bitmap_data_tag_flag::unused);
    if (((int32_t)width & ((int32_t)width - 1)) == 0 &&
        ((int32_t)height & ((int32_t)height - 1)) == 0) {
        bitmap->flags = to_bits(tags::bitmap_data_tag_flag::unused | tags::bitmap_data_tag_flag::power_of_two_dimensions);
    }

    *(void **)&((struct BitmapData *)bitmap)->pixel_base = GlobalAlloc(0, halo::bitmaps::bitmap_data_view(bitmap).calculate_pixel_data_size());

    if (*(void **)&((struct BitmapData *)bitmap)->pixel_base != 0) {
        halo::main::console_print_error_va(1, "");
        halo::main::console_deactivate();

        for (page_row = 0; page_row < main_globals_data.screenshot_tile_count; page_row++) {
            for (page_col = 0; page_col < main_globals_data.screenshot_tile_count; page_col++) {
                int16_t sub_row, sub_col;
                char filename[512];
                file_reference_record request;
                Point2DInt page;
                Point2DInt tile;

                page.x = page_col;
                page.y = page_row;
                for (sub_row = 0; sub_row < screenshot_scale; sub_row++) {
                    for (sub_col = 0; sub_col < screenshot_scale; sub_col++) {
                        tile.x = sub_col;
                        tile.y = sub_row;
                        if (main_globals_data.screenshot_tile_count < 2 && screenshot_scale < 2) {
                            halo::render::render_frame(0, views, 1, 0, 0.0f, 0.0f);
                            halo::rasterizer::rasterizer_capture_and_present(0, bitmap);
                        } else {
                            halo::render::render_frame(&tile, views, 1, &page, 0.0f, 0.0f);
                            halo::rasterizer::rasterizer_capture_and_present(&tile.x, bitmap);
                        }
                    }
                }

                sprintf(filename, "%s\\%dscreenshot%d%d.tga", "screenshots",
                        (uint32_t)main_globals_data.screenshot_index, (int32_t)page_row,
                        (int32_t)page_col);
                halo::cseries::directory_create_recursive((char *)"screenshots");

                memset(&request, 0, sizeof(request));
                request.signature = k_file_reference_signature;
                request.location = -1;
                if ((request.flags & 1) != 0) {
                    halo::saved_games::path_remove_last_component((char *)((uint8_t *)&request.path));
                }
                if (filename[0] != 0) {
                    strncpy(request.path, filename, k_main_path_length - 1);
                    request.path[k_main_path_length - 1] = 0;
                }
                request.flags = request.flags | 1;

                halo::bitmaps::targa_export(bitmap, &request);
            }
        }

        main_globals_data.screenshot_index = main_globals_data.screenshot_index + 1;
        halo::bitmaps::bitmap_data_view(bitmap).free();
    }
    main_globals_data.screenshot_tile_count = 0;
}

}

extern "C" { extern Rectangle2D game_screen_rect; }
namespace halo::main {

/**
 * Fits `view_count` viewports into as square a grid as possible, then computes the `view_index`th
 * cell of that grid, dividing game_screen_rect's width across the grid's columns and its height
 * across its rows. Writes the raw grid cell into `window` (with a small margin applied per row/
 * column when splitting more than one view), and a copy of the pre-margin cell into
 * `out_viewport`, except that a cell touching the outer edge of the grid instead gets clamped to
 * game_window_top_left's matching edge on that side.
 *
 * Original register convention: EAX -> view_count, EDX -> view_index, ECX -> window, stack -> out_viewport.
 *
 * @address 0x4c8da0
 */
void RenderViews::viewport_split_rect_compute(int32_t view_count, int32_t view_index, Rectangle2D *window, Rectangle2D *out_viewport)
{
    int32_t columns;
    int32_t rows;
    int32_t candidate_rows;
    uint8_t center_single_column;
    uint8_t one_fewer_row;
    int16_t extra_blank_flag;
    int32_t row_index;
    int32_t col_index;
    int16_t cell_width;
    int16_t cell_height;

    columns = 1;
    center_single_column = 0;
    one_fewer_row = 0;
    extra_blank_flag = (view_count < 2) ? 0 : 4;
    candidate_rows = 1;
    rows = 1;
    if (view_count > 1) {
        do {
            if (columns < candidate_rows) {
                columns = columns + 1;
            } else {
                columns = 1;
                candidate_rows = candidate_rows + 1;
            }
            rows = candidate_rows;
        } while (candidate_rows * columns < view_count);
    }

    if (rows * columns - view_count != 0 && view_count <= rows * columns) {
        if (view_index == 0) {
            center_single_column = 1;
            one_fewer_row = 1;
        } else {
            view_index = view_index + 1;
        }
    }

    row_index = view_index / columns;
    col_index = view_index - columns * row_index;

    cell_width = (int16_t)((game_screen_rect.right - game_screen_rect.left) / columns) *
                 (center_single_column + 1);
    window->left = cell_width * (int16_t)col_index + game_screen_rect.left;
    window->right = cell_width * ((int16_t)col_index + 1) + game_screen_rect.left;

    cell_height = (int16_t)((game_screen_rect.bottom - game_screen_rect.top) / rows);
    window->top = cell_height * (int16_t)row_index + game_screen_rect.top;
    window->bottom = cell_height * ((int16_t)row_index + 1) + game_screen_rect.top;

    *out_viewport = *window;

    window->left = window->left + (int16_t)col_index * extra_blank_flag;
    window->right = window->right - (int16_t)((col_index == 0) * extra_blank_flag);
    window->top = window->top + (int16_t)row_index * extra_blank_flag;
    window->bottom = window->bottom - (int16_t)((row_index == 0) * extra_blank_flag);

    if (col_index == 0) {
        out_viewport->left = game_window_top_left.left;
    }
    if (one_fewer_row + 1 + col_index == columns) {
        out_viewport->right = game_window_top_left.right;
    }
    if (row_index == 0) {
        out_viewport->top = game_window_top_left.top;
    }
    if (row_index + 1 == rows) {
        out_viewport->bottom = game_window_top_left.bottom;
    }
}

}
