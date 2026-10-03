/**
 * @file src/camera/vars.cpp
 * Binds halo::camera::Vars to the engine variables the data image defines under their original link names.
 */

#include "halo/camera/vars.hpp"
#include "link/camera_vars.hpp"

namespace halo::camera {

const Vars &vars()
{
    static const Vars table{
        camera_director_globals,
        camera_input_axes,
        camera_script,
        director_camera_switching,
        director_last_pov_proc,
        directors,
        flying_camera_allow_roll,
        flying_camera_attached_object,
        flying_camera_attached_offset,
        flying_camera_current_mode,
        flying_camera_data,
        flying_camera_follow_script,
        flying_camera_home_initialized,
        flying_camera_home_location,
        flying_camera_render_frame,
        flying_camera_saved_flying,
        flying_camera_saved_orbiting,
        flying_camera_saved_orbiting_valid,
        flying_camera_speed,
        flying_camera_transition_procs,
        flying_camera_update_procs,
        hs_camera_control_pointer,
        live_mouse_state,
        matrix4x3_multiply_ptr,
        mouse_device,
        mouse_neutral_state,
        observer_channel_acceleration_limit,
        observer_derivative_float_counts,
        observer_dt,
        observer_parameter_float_counts,
        observers,
    };
    return table;
}

}  // namespace halo::camera
