#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::main {

/**
 * Render view setup, split-screen rectangles and screenshots.
 * Stateless service class: the functions are static members, the state they act on lives in the engine globals.
 */
struct RenderViews {
    static void frame_all_views(float time_since_tick, float time_since_frame);
    static int local_view_count(void);
    static void pregame_view_initialize(void);
    static void view_camera_fill(observer_camera *observer, render_view *view);
    static void screenshot_render(render_view *views);
    static void viewport_split_rect_compute(int32_t view_count, int32_t view_index, Rectangle2D *window, Rectangle2D *out_viewport);
};

}
