/**
 * @file src/render/vars.cpp
 * Binds halo::render::Vars to the engine variables the data image defines under their original link names.
 */

#include "halo/render/vars.hpp"
#include "link/render_vars.hpp"
#include "halo/render/api.hpp"

namespace halo::render {

const Vars &vars()
{
    static const Vars table{
        build_sprite_group_warning,
        build_sprite_large_quad_count,
        build_sprite_screen_coverage,
        build_sprite_view_left,
        build_sprite_view_up,
        console_debug_toggle_6893ec,
        console_debug_toggle_6893ee,
        frame_graph_render_graph,
        frame_graph_render_infos,
        frame_graph_window_height,
        frame_graph_window_width,
        frame_graphs,
        frame_statistics_count,
        frame_statistics_dropped,
        frame_statistics_key_a_latch,
        frame_statistics_key_b_latch,
        frame_statistics_last_time,
        frame_statistics_times,
        frame_statistics_unknown_d0,
        frame_statistics_unknown_d8,
        global_null_rectangle3d_pointer,
        global_real_rgb_green_pointer,
        render_asymmetric_frustum_disabled,
        render_camera_facing_basis,
        render_camera_global,
        render_camera_world_to_view,
        render_clip_warning,
        render_debug_objects,
        render_fog_state,
        render_frame_index,
        render_frustum_global,
        render_lighting_smoothing_enabled,
        render_saved_projection_z,
        render_time_since_frame,
        render_time_since_tick,
        render_uncached_object_lighting,
        render_viewport_left,
        render_viewport_right,
        render_window_count,
        render_window_index,
        rendered_object_count,
        rendered_objects,
        rendered_objects_full_warning,
        sky_animation_times,
    };
    return table;
}

}  // namespace halo::render
