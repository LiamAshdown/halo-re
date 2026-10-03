/**
 * @file include/halo/camera/api.hpp
 * Functions of the camera module that other modules and the data tables call (namespace halo::camera). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdint.h>

struct Point3D;
struct Vector3D;
struct camera_input;
struct dead_camera_data;
struct editor_camera_data;
struct director;
struct camera_script_globals;
struct observer;
struct observer_camera;
struct observer_command;
struct real_point3d;
struct real_vector3d;
struct unit_camera_properties;
union director_camera_data;
typedef uint32_t datum_index;

namespace halo::camera {

/**
 * The engine globals the camera module owns (their storage is defined by standalone/data under the original link names);
 * other modules reach them through globals().
 */
struct Globals {
    uint8_t *&hs_camera_control_pointer;
    observer *observers;
    director *directors;
    camera_script_globals &camera_script;
};

Globals &globals();

void camera_control(uint8_t enable);
datum_index camera_dead_find_next_teammate(datum_index reference_player, datum_index current_target, uint8_t require_same_team);
uint8_t camera_dead_player_has_teammate(datum_index reference_player);
void camera_debug_compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
void camera_debug_load_from_file(void);
void camera_debug_save_to_file(void);
void camera_debug_start(int16_t camera_point_index, int16_t ticks, datum_index relative_object);
void camera_first_person_compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
int16_t camera_get_seat_camera_state(datum_index unit, int16_t *out_state);
int16_t camera_get_type_for_player(int16_t local_player_index);
void camera_initialize(void);
void camera_input_axes_update(int16_t local_player_index, uint32_t key_bits, float zoom);
uint8_t camera_is_local_player_default_first_person(void);
void camera_script_set_animation(datum_index animation_tag, char *name);
void camera_third_person_compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
void camera_track_compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
void camera_update(float dt);
dead_camera_data * dead_camera_new(dead_camera_data *self, int16_t local_player_index, datum_index unit);
uint8_t director_build_camera_input(int16_t local_player_index, camera_input *input);
void director_choose_gameplay_camera(int16_t local_player_index, uint8_t reset);
void director_game_state_loaded(void);
void director_set_flying_camera(int16_t local_player_index, uint8_t force);
void director_update_seat_camera(int16_t local_player_index, uint8_t force);
void editor_camera_compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
void editor_camera_set_position_and_direction(editor_camera_data *out, Vector3D *direction, Point3D *position);
void first_person_camera_apply_weapon_offset(real_point3d *position, datum_index unit, real_vector3d *aiming_direction);
void first_person_camera_command_for_unit(datum_index unit, observer_command *command);
void first_person_camera_deterministic(Point3D *out_position, datum_index unit, Vector3D *out_direction);
void first_person_camera_for_unit_and_vector(observer_command *command, Vector3D *vector, datum_index unit);
void first_person_camera_track_offset(unit_camera_properties *properties, float angle, Vector3D *out);
void flying_camera_attach_to_object(datum_index object_index);
void flying_camera_compute_pov(director_camera_data *data, camera_input *input, observer_command *command);
void flying_camera_enter_flying(editor_camera_data *data);
void flying_camera_enter_orbiting(editor_camera_data *data);
void flying_camera_initialize(editor_camera_data *data, int16_t local_player_index);
void flying_camera_update(director_camera_data *data, camera_input *input, observer_command *command);
void observer_advance(int16_t local_player_index);
void observer_avoid_collision(real_vector3d *forward, real_point3d *position, real_vector3d *up, float *distance, float radius_scale);
uint8_t observer_collision_test_ray(real_point3d *origin, uint8_t use_alternate_mask, real_point3d *target, float *out_fraction);
void observer_commit(int16_t local_player_index);
void observer_compute_remaining_offset(float *target, float *current, float *out);
void observer_compute_spline_coefficients(int16_t local_player_index);
void observer_evaluate_spline_acceleration(int16_t local_player_index);
void observer_evaluate_spline_value_and_orthonormalize(int16_t local_player_index);
void observer_evaluate_spline_velocity(int16_t local_player_index);
observer_camera * observer_get_camera(int16_t player_index);
void observer_initialize(void);
void observer_new(observer *self);
void observer_set_command(int16_t local_player_index);
void observer_update(float dt, uint8_t add_bob);
void observer_update_location(void);
void orbiting_camera_update(director_camera_data *data, camera_input *input, observer_command *command);
uint8_t real_approximately_equal(float a, float b);
uint8_t real_is_valid(float value);
double scalar_catmull_rom_interpolate(float value0, float value1, float value2, float value3, float time0, float dt, float time);
unit_camera_properties * unit_get_camera_properties(datum_index unit);
void vector3d_catmull_rom_interpolate(Vector3D *source1, Vector3D *source3, Vector3D *source2, Vector3D *out, Vector3D *source0, float time0, float dt, float time);
void vector3d_compute_up_from_forward(Vector3D *forward, Vector3D *up);
uint8_t vector3d_is_unit_length(Vector3D *v);
void vector3d_rotate_basis_by_axis_angle(Vector3D *axis_angle, Vector3D *forward, Vector3D *up);

}
