#pragma once


namespace halo::camera {

static_assert(sizeof(camera_constants) == 4);
static_assert(sizeof(director_camera_mode) == 4);
static_assert(sizeof(camera_script_mode) == 4);
static_assert(sizeof(director_camera_type) == 4);
static_assert(sizeof(director_seat_camera_state) == 4);
static_assert(sizeof(flying_camera_mode) == 4);
static_assert(sizeof(observer_parameter) == 4);
static_assert(sizeof(observer_command_flags) == 4);
static_assert(sizeof(observer_interpolation_flags) == 4);
static_assert(sizeof(camera_input_key_bits) == 4);
static_assert(sizeof(camera_script_globals) == 64);
static_assert(sizeof(camera_input_axis_definition) == 28);
static_assert(sizeof(camera_input_axis_state) == 12);
static_assert(sizeof(camera_input) == 36);
static_assert(sizeof(observer_parameters) == 56);
static_assert(sizeof(observer_command) == 104);
static_assert(sizeof(observer_parameter_derivatives) == 44);
static_assert(sizeof(observer_camera) == 60);
static_assert(sizeof(observer) == 668);
static_assert(sizeof(first_person_camera_data) == 4);
static_assert(sizeof(third_person_camera_data) == 28);
static_assert(sizeof(dead_camera_data) == 48);
static_assert(sizeof(editor_camera_data) == 28);
static_assert(sizeof(orbiting_camera_data) == 28);
static_assert(sizeof(director_camera_data) == 64);
static_assert(sizeof(director) == 248);
static_assert(sizeof(director_globals) == 8);
static_assert(sizeof(flying_camera_home) == 20);
static_assert(sizeof(unit_camera_properties) == 88);

}
