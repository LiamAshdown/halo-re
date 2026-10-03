/**
 * @file standalone/data/link/camera_vars.hpp
 * Link names of the engine variables owned by the camera module (halo::camera::vars()). The data image defines them under these C
 * names; only src/camera/vars.cpp includes this header, so the names are declared as untyped storage.
 */
#pragma once

extern "C" {
extern char camera_director_globals[];
extern char camera_input_axes[];
extern char camera_script[];
extern char director_camera_switching[];
extern char director_last_pov_proc[];
extern char directors[];
extern char flying_camera_allow_roll[];
extern char flying_camera_attached_object[];
extern char flying_camera_attached_offset[];
extern char flying_camera_current_mode[];
extern char flying_camera_data[];
extern char flying_camera_follow_script[];
extern char flying_camera_home_initialized[];
extern char flying_camera_home_location[];
extern char flying_camera_render_frame[];
extern char flying_camera_saved_flying[];
extern char flying_camera_saved_orbiting[];
extern char flying_camera_saved_orbiting_valid[];
extern char flying_camera_speed[];
extern char flying_camera_transition_procs[];
extern char flying_camera_update_procs[];
extern char hs_camera_control_pointer[];
extern char live_mouse_state[];
extern char matrix4x3_multiply_ptr[];
extern char mouse_device[];
extern char mouse_neutral_state[];
extern char observer_channel_acceleration_limit[];
extern char observer_derivative_float_counts[];
extern char observer_dt[];
extern char observer_parameter_float_counts[];
extern char observers[];
}
