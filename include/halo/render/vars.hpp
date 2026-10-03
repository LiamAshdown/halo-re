/**
 * @file include/halo/render/vars.hpp
 * Addresses of the engine variables the render module owns, by name. Other files bind a typed reference to the entry they use
 * (halo::link::ref<T>); the link names themselves appear only in standalone/data/link/render_vars.hpp.
 */
#pragma once

namespace halo::render {

/** Address table of the engine variables owned by the render module. */
struct Vars {
    void *build_sprite_group_warning;
    void *build_sprite_large_quad_count;
    void *build_sprite_screen_coverage;
    void *build_sprite_view_left;
    void *build_sprite_view_up;
    void *console_debug_toggle_6893ec;
    void *console_debug_toggle_6893ee;
    void *frame_graph_render_graph;
    void *frame_graph_render_infos;
    void *frame_graph_window_height;
    void *frame_graph_window_width;
    void *frame_graphs;
    void *frame_statistics_count;
    void *frame_statistics_dropped;
    void *frame_statistics_key_a_latch;
    void *frame_statistics_key_b_latch;
    void *frame_statistics_last_time;
    void *frame_statistics_times;
    void *frame_statistics_unknown_d0;
    void *frame_statistics_unknown_d8;
    void *global_null_rectangle3d_pointer;
    void *global_real_rgb_green_pointer;
    void *render_asymmetric_frustum_disabled;
    void *render_camera_facing_basis;
    void *render_camera_global;
    void *render_camera_world_to_view;
    void *render_clip_warning;
    void *render_debug_objects;
    void *render_fog_state;
    void *render_frame_index;
    void *render_frustum_global;
    void *render_lighting_smoothing_enabled;
    void *render_saved_projection_z;
    void *render_time_since_frame;
    void *render_time_since_tick;
    void *render_uncached_object_lighting;
    void *render_viewport_left;
    void *render_viewport_right;
    void *render_window_count;
    void *render_window_index;
    void *rendered_object_count;
    void *rendered_objects;
    void *rendered_objects_full_warning;
    void *sky_animation_times;
};

/** The singleton address table; its storage is constant-initialised. */
const Vars &vars();

}  // namespace halo::render
