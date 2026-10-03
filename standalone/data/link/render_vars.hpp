/**
 * @file standalone/data/link/render_vars.hpp
 * Link names of the engine variables owned by the render module (halo::render::vars()). The data image defines them under these C
 * names; only src/render/vars.cpp includes this header, so the names are declared as untyped storage.
 */
#pragma once

extern "C" {
extern char build_sprite_group_warning[];
extern char build_sprite_large_quad_count[];
extern char build_sprite_screen_coverage[];
extern char build_sprite_view_left[];
extern char build_sprite_view_up[];
extern char console_debug_toggle_6893ec[];
extern char console_debug_toggle_6893ee[];
extern char frame_graph_render_graph[];
extern char frame_graph_render_infos[];
extern char frame_graph_window_height[];
extern char frame_graph_window_width[];
extern char frame_graphs[];
extern char frame_statistics_count[];
extern char frame_statistics_dropped[];
extern char frame_statistics_key_a_latch[];
extern char frame_statistics_key_b_latch[];
extern char frame_statistics_last_time[];
extern char frame_statistics_times[];
extern char frame_statistics_unknown_d0[];
extern char frame_statistics_unknown_d8[];
extern char global_null_rectangle3d_pointer[];
extern char global_real_rgb_green_pointer[];
extern char render_asymmetric_frustum_disabled[];
extern char render_camera_facing_basis[];
extern char render_camera_global[];
extern char render_camera_world_to_view[];
extern char render_clip_warning[];
extern char render_debug_objects[];
extern char render_fog_state[];
extern char render_frame_index[];
extern char render_frustum_global[];
extern char render_lighting_smoothing_enabled[];
extern char render_saved_projection_z[];
extern char render_time_since_frame[];
extern char render_time_since_tick[];
extern char render_uncached_object_lighting[];
extern char render_viewport_left[];
extern char render_viewport_right[];
extern char render_window_count[];
extern char render_window_index[];
extern char rendered_object_count[];
extern char rendered_objects[];
extern char rendered_objects_full_warning[];
extern char sky_animation_times[];
}
