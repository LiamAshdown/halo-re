/**
 * @file include/halo/camera/vars.hpp
 * Addresses of the engine variables the camera module owns, by name. Other files bind a typed reference to the entry they use
 * (halo::link::ref<T>); the link names themselves appear only in standalone/data/link/camera_vars.hpp.
 */
#pragma once

namespace halo::camera {

/** Address table of the engine variables owned by the camera module. */
struct Vars {
    void *camera_director_globals;
    void *camera_input_axes;
    void *camera_script;
    void *director_camera_switching;
    void *director_last_pov_proc;
    void *directors;
    void *flying_camera_allow_roll;
    void *flying_camera_attached_object;
    void *flying_camera_attached_offset;
    void *flying_camera_current_mode;
    void *flying_camera_data;
    void *flying_camera_follow_script;
    void *flying_camera_home_initialized;
    void *flying_camera_home_location;
    void *flying_camera_render_frame;
    void *flying_camera_saved_flying;
    void *flying_camera_saved_orbiting;
    void *flying_camera_saved_orbiting_valid;
    void *flying_camera_speed;
    void *flying_camera_transition_procs;
    void *flying_camera_update_procs;
    void *hs_camera_control_pointer;
    void *live_mouse_state;
    void *matrix4x3_multiply_ptr;
    void *mouse_device;
    void *mouse_neutral_state;
    void *observer_channel_acceleration_limit;
    void *observer_derivative_float_counts;
    void *observer_dt;
    void *observer_parameter_float_counts;
    void *observers;
};

/** The singleton address table; its storage is constant-initialised. */
const Vars &vars();

}  // namespace halo::camera
