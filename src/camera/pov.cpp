#include "halo/camera/pov.hpp"
#include "halo/models/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/scenario/api.hpp"

extern "C" {
extern player_control_globals *player_control_globals_ptr;
extern player_globals *local_player_globals;
extern director_pov_proc director_last_pov_proc;
extern void player_compute_view_forward_vector(datum_index player_handle, real *yaw_pitch, real_vector3d *out_forward);
extern real game_engine_get_max_look_pitch(int16_t local_player_index);
extern data_array *object_data;
extern double sqrt(double x);
extern double fabs(double x);
extern void unit_get_camera_position(datum_index unit, real_point3d *out);
extern double asin(double x);
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask);
extern int16_t object_get_node_local_transform(datum_index object_index, const char *marker_name, object_marker *markers, int32_t maximum_count);
extern const real_point3d *global_origin3d_pointer;
extern void object_get_root_object_velocities(datum_index object_index, real_vector3d *out_velocity, real_vector3d *out_angular_velocity);
extern Globals *global_globals;
extern double fcos(double angle);
extern double fsin(double angle);
extern void chimera__spectate_fp_camera_position(camera_basis_out *out, int16_t local_player_index);
extern data_array *player_data;
extern game_time_globals *game_time;
extern game_engine_definition *current_game_engine;
extern double cos(double x);
extern double sin(double x);
extern camera_script_globals camera_script;
extern int16_t network_game_mode;
extern real_point3d *global_zero_vector3d_pointer;
extern void animation_get_root_node_matrix(real_matrix4x3 *out, int16_t frame, ModelAnimationsAnimation *animation, GBXModel *model);
extern int32_t __ftol(double x);
extern double atan2(double y, double x);
extern datum_index flying_camera_attached_object;
extern editor_camera_data *flying_camera_data;
extern Vector3D flying_camera_attached_offset;
extern uint8_t flying_camera_follow_script;
extern void *flying_camera_render_frame;
extern int16_t flying_camera_current_mode;
extern director_pov_proc flying_camera_update_procs[2];
extern flying_camera_transition_proc flying_camera_transition_procs[2][2];
extern orbiting_camera_data flying_camera_saved_orbiting;
extern uint8_t flying_camera_saved_orbiting_valid;
extern editor_camera_data flying_camera_saved_flying;
extern uint8_t flying_camera_home_initialized;
extern flying_camera_home flying_camera_home_location;
extern float flying_camera_speed;
extern uint8_t flying_camera_allow_roll;
extern director directors[1];
}

namespace halo::camera {

namespace {
/**
 * The render frame the flying camera reads its starting pose from: a header followed by the render camera.
 */
struct flying_render_frame {
    uint8_t header[20];
    render_camera camera;
};
static_assert(offsetof(flying_render_frame, camera) == 0x14);
}

/**
 * Original function camera_first_person_compute_pov; the author notes are in
 * docs/original/camera/camera_first_person_compute_pov.c.txt.
 *
 * Register convention in the original: matches director_pov_proc exactly --
 * (director_camera_data *data, camera_input *input, observer_command *command), cdecl, all
 * three on the stack.
 *
 * @address 0x446d60
 */
void FirstPersonCamera::compute_pov(director_camera_data *data, camera_input *input, observer_command *command)
{
    float *cached_field_of_view = (float *)data; 
    datum_index local_player;
    Vector3D direction;
    real fov;

    if (input->local_player_index == -1 || input->local_player_index >= 1) {
        local_player = k_datum_index_none; 
    } else {
        local_player = local_player_globals->local_players[input->local_player_index];
    }

    player_compute_view_forward_vector(local_player,
        &player_control_globals_ptr->local_players[input->local_player_index].yaw,
        (real_vector3d *)&direction);

    halo::camera::first_person_camera_for_unit_and_vector(command, &direction,
                                             player_control_globals_ptr->local_players[input->local_player_index].unit);

    fov = game_engine_get_max_look_pitch(input->local_player_index);
    command->parameters.field_of_view = fov;
    if (*cached_field_of_view != fov) {
        command->interpolation_flags[_observer_parameter_field_of_view] = 1;
        *cached_field_of_view = fov;
        command->channel_times[_observer_parameter_field_of_view] = 0.18f;
    }

    if (director_last_pov_proc != halo::camera::camera_first_person_compute_pov) {
        command->channel_times[_observer_parameter_field_of_view] = 0.0f;
    }

    command->interpolation_flags[_observer_parameter_orientation] |= 3;
    command->channel_times[_observer_parameter_orientation] = 0.0f;
    command->channel_times[_observer_parameter_position] = 0.0f;
    command->timer = 0.0f;
    command->interpolation_flags[_observer_parameter_position] |= 3;
    command->flags |= 1;
}

/**
 * Original function first_person_camera_apply_weapon_offset; the author notes are in
 * docs/original/camera/first_person_camera_apply_weapon_offset.c.txt.
 *
 * Register convention in the original: EAX -> position (also the seed/output of
 * unit_get_camera_position), EBX.
 *
 * @address 0x447290
 */
void FirstPersonCamera::apply_weapon_offset(real_point3d *position, datum_index unit, real_vector3d *aiming_direction)
{
    object *unit_object;
    unit_camera_properties *properties;
    unit_data *unit_extension;
    Vector3D track_offset;
    double pitch_angle;
    float horizontal_i, horizontal_j;
    float magnitude;

    unit_object = ((object_header *)object_data->data)[(uint16_t)unit].data;
    properties = halo::camera::unit_get_camera_properties(unit);
    unit_get_camera_position(unit, position);

    unit_extension = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);
    *aiming_direction = unit_extension->aiming_vector;

    pitch_angle = asin((double)aiming_direction->k);
    halo::camera::first_person_camera_track_offset(properties, (float)pitch_angle, &track_offset);

    horizontal_i = aiming_direction->i;
    horizontal_j = aiming_direction->j;
    magnitude = (float)sqrt((double)(horizontal_i * horizontal_i + horizontal_j * horizontal_j));
    if (0.0001 <= fabs((double)magnitude)) {
        magnitude = 1.0f / magnitude;
        horizontal_i = magnitude * horizontal_i;
        horizontal_j = magnitude * horizontal_j;
    }

    position->x = track_offset.i * horizontal_i + track_offset.j * horizontal_j + position->x;
    position->y = (track_offset.i * horizontal_j - track_offset.j * horizontal_i) + position->y;
    position->z = track_offset.k + position->z;
}

/**
 * Original function first_person_camera_command_for_unit; the author notes are in
 * docs/original/camera/first_person_camera_command_for_unit.c.txt.
 *
 * Register convention in the original: unit handle in ECX (in_ECX); command pointer as the
 * single cdecl stack parameter (confirmed with objdump: `mov ebx,[esp+8]` reads it right after
 * `push ebx`, i.e.
 *
 * @address 0x446d30
 */
void FirstPersonCamera::command_for_unit(datum_index unit, observer_command *command)
{
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[halo::datum_slot(unit)].data;
    Vector3D *aiming_vector = (Vector3D *)&((unit_data *)((uint8_t *)obj + k_unit_data_offset))->aiming_vector; 

    halo::camera::first_person_camera_for_unit_and_vector(command, aiming_vector, unit);
}

/**
 * Original function first_person_camera_deterministic; the author notes are in
 * docs/original/camera/first_person_camera_deterministic.c.txt.
 *
 * Register convention in the original: output position in EAX (in_EAX), unit handle in ECX
 * (in_ECX), output direction as the single cdecl stack parameter (confirmed with objdump).
 *
 * @address 0x446a90
 */
void FirstPersonCamera::deterministic(Point3D *out_position, datum_index unit, Vector3D *out_direction)
{
    object_header *headers = (object_header *)object_data->data;
    object *unit_object = headers[halo::datum_slot(unit)].data;
    datum_index parent;

    unit_get_camera_position(unit, (real_point3d *)out_position);
    *out_direction = *(Vector3D *)&((unit_data *)((uint8_t *)unit_object + k_unit_data_offset))->aiming_vector; 

    parent = unit_object->parent_object;
    if (parent == k_datum_index_none) {
        return;
    }

    {
        object *parent_object = object_try_and_get(parent, 2);
        if (parent_object == (object *)0) {
            return;
        }

        {
            Unit *parent_unit_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(parent_object->definition_tag)].data;
            uint8_t *seats = (uint8_t *)parent_unit_tag->seats.pointer;
            int16_t seat_index = ((unit_data *)((uint8_t *)unit_object + k_unit_data_offset))->vehicle_seat_index;
            int8_t seat_flags = *(int8_t *)(seats + (int32_t)seat_index * sizeof(UnitSeat));

            if (seat_flags < 0) { 
                object_marker marker; 
                int16_t ok = object_get_node_local_transform(parent, "primary trigger", &marker, 1);
                if (ok != 0) {
                    real_matrix4x3 *m = &marker.node_transform; 
                    *out_position = *(Point3D *)&m->position;
                    *out_direction = *(Vector3D *)&m->forward;
                }
            }
        }
    }
}

/**
 * Original function first_person_camera_for_unit_and_vector; the author notes are in
 * docs/original/camera/first_person_camera_for_unit_and_vector.c.txt.
 *
 * Register convention in the original: command pointer in EBX (unaff_EBX), direction vector in
 * EAX (in_EAX); the unit handle is the single cdecl stack parameter.
 *
 * @address 0x446b70
 */
void FirstPersonCamera::for_unit_and_vector(observer_command *command, Vector3D *vector, datum_index unit)
{
    command->timer = 0.0f;
    command->flags = 0;
    command->parameters.focus_offset = *(Vector3D *)global_origin3d_pointer;
    command->parameters.distance = 0.0f;
    command->parameters.forward = *vector;
    command->parameters.field_of_view = 1.2217305f; 
    halo::camera::vector3d_compute_up_from_forward(&command->parameters.forward, &command->parameters.up);

    if (unit == k_datum_index_none) {
        return;
    }

    {
        object_header *headers = (object_header *)object_data->data;
        object *unit_object = headers[halo::datum_slot(unit)].data;
        datum_index parent;
        object *parent_object;

        unit_get_camera_position(unit, (real_point3d *)&command->parameters.position);
        object_get_root_object_velocities(unit, (real_vector3d *)&command->velocity, (real_vector3d *)0);

        parent = unit_object->parent_object;
        if (parent == k_datum_index_none) {
            command->flags = 1;
            return;
        }

        parent_object = object_try_and_get(parent, 2);
        if (parent_object == (object *)0) {
            command->flags = 1;
            return;
        }

        {
            Unit *parent_unit_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(parent_object->definition_tag)].data;
            uint8_t *seats = (uint8_t *)parent_unit_tag->seats.pointer;
            int16_t seat_index = ((unit_data *)((uint8_t *)unit_object + k_unit_data_offset))->vehicle_seat_index;
            uint8_t seat_flags = *(uint8_t *)(seats + (int32_t)seat_index * sizeof(UnitSeat));

            if ((seat_flags & 0x80) != 0) { 
                object_marker marker; 
                int16_t ok = object_get_node_local_transform(parent, "primary trigger", &marker, 1);
                if (ok != 0) {
                    real_matrix4x3 *m = &marker.node_transform; 
                    command->parameters.position = *(Point3D *)&m->position;
                    command->parameters.forward = *(Vector3D *)&m->forward;
                    command->parameters.up = *(Vector3D *)&m->up;
                    command->flags = 1;
                    return;
                }
            } else {
                real_matrix4x3 seat_matrix;
                halo::math::matrix4x3_from_forward_up_position((real_vector3d *)&parent_object->up,
                                                    (real_vector3d *)&parent_object->forward,
                                                    *((real_point3d *)&parent_object->position), &seat_matrix);
                halo::math::matrix4x3_inverse_transform_normal(*((real_vector3d *)&command->parameters.forward),
                                                    *((real_vector3d *)&command->parameters.forward), seat_matrix);
                halo::camera::vector3d_compute_up_from_forward(&command->parameters.forward, &command->parameters.up);
                halo::math::matrix4x3_transform_normal(*((real_vector3d *)&command->parameters.forward),
                                            *((real_vector3d *)&command->parameters.forward), seat_matrix);
                halo::math::matrix4x3_transform_normal(*((real_vector3d *)&command->parameters.up),
                                            *((real_vector3d *)&command->parameters.up), seat_matrix);
            }
        }

        command->flags = 1;
    }
}

/**
 * Original function first_person_camera_track_offset; the author notes are in
 * docs/original/camera/first_person_camera_track_offset.c.txt.
 *
 * Register convention in the original: unit_camera_properties * in ECX (in_ECX); angle and out
 * on the stack.
 *
 * @address 0x447190
 */
void FirstPersonCamera::track_offset(unit_camera_properties *properties, float angle, Vector3D *out)
{
    UnitCameraTrack *first_track;
    int32_t count_minus_one;
    int32_t clamped_index;
    datum_index track_tag;
    CameraTrack *track;
    int32_t control_point_count;
    float time;
    int16_t frame_guess;
    int16_t base_index;
    float dt;
    float time0;
    CameraTrackControlPoint *points;
    CameraTrackControlPoint *source0;
    CameraTrackControlPoint *source1;
    CameraTrackControlPoint *source2;
    CameraTrackControlPoint *source3;

    track_tag = (datum_index)k_datum_index_none;
    if (properties->camera_tracks.count != 0) {
        
        
        count_minus_one = (int32_t)properties->camera_tracks.count - 1;
        clamped_index = (count_minus_one < 0) ? count_minus_one : 0;
        first_track = (UnitCameraTrack *)((uint8_t *)properties->camera_tracks.pointer +
            clamped_index * (int32_t)sizeof(UnitCameraTrack));
        if (first_track != 0) {
            track_tag = *(datum_index *)&first_track->track.tag_id;
        }
    }
    if (track_tag == (datum_index)k_datum_index_none) {
        track_tag = *(datum_index *)&((GlobalsCamera *)global_globals->camera.pointer)->
            default_unit_camera_track.tag_id;
    }

    track = (CameraTrack *)halo::cache::globals().tag_instances[halo::datum_slot(track_tag)].data;
    control_point_count = (int32_t)track->control_points.count;

    
    time = (angle + 1.5707964f) * 0.31830987f;

    dt = 1.0f / (float)(control_point_count - 1);
    frame_guess = (int16_t)(time * (float)(control_point_count - 1));

    
    
    base_index = frame_guess;
    if (frame_guess > 0) {
        do {
            if (base_index + 4 <= control_point_count && base_index <= frame_guess - 1) {
                break;
            }
            base_index -= 1;
        } while (base_index > 0);
    }

    time0 = (float)base_index * dt;

    points = (CameraTrackControlPoint *)track->control_points.pointer;
    source0 = points + base_index;
    source1 = points + base_index + 1;
    source2 = points + base_index + 2;
    source3 = points + base_index + 3;

    halo::camera::vector3d_catmull_rom_interpolate((Vector3D *)source1, (Vector3D *)source3,
        (Vector3D *)source2, out, (Vector3D *)source0, time0, dt, time);
}

/**
 * Original function unit_get_camera_properties; the author notes are in
 * docs/original/camera/unit_get_camera_properties.c.txt.
 *
 * Register convention in the original: unit's datum_index in EAX (in_EAX), no other
 * parameters.
 *
 * @address 0x447110
 */
unit_camera_properties * FirstPersonCamera::unit_properties(datum_index unit)
{
    object *unit_object;
    object *vehicle_object;
    unit_data *unit_extension;
    Unit *vehicle_tag;
    UnitSeat *seat;

    unit_object = ((object_header *)object_data->data)[halo::datum_slot(unit)].data;

    if (unit_object->parent_object != (datum_index)k_datum_index_none) {
        vehicle_object = object_try_and_get(unit_object->parent_object, _object_mask_vehicle);
        if (vehicle_object != 0) {
            unit_extension = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);
            vehicle_tag = (Unit *)halo::cache::globals().tag_instances[halo::datum_slot(vehicle_object->definition_tag)].data;
            seat = &((UnitSeat *)vehicle_tag->seats.pointer)[unit_extension->vehicle_seat_index];
            if ((seat->flags & 0x15) != 0) {
                
                
                return (unit_camera_properties *)&seat->camera_marker_name;
            }
        }
    }
    return (unit_camera_properties *)
        (&((Unit *)halo::cache::globals().tag_instances[halo::datum_slot(unit_object->definition_tag)].data)->camera_marker_name);
}

/**
 * Original function camera_third_person_compute_pov; the author notes are in
 * docs/original/camera/camera_third_person_compute_pov.c.txt.
 *
 * Register convention in the original: all three parameters are on the stack (cdecl, matching
 * director_pov_proc);.
 *
 * @address 0x447370
 */
void ThirdPersonCamera::compute_pov(director_camera_data *data, camera_input *input, observer_command *command)
{
    third_person_camera_data *tp = &data->third_person;
    camera_basis_out basis;
    object *unit_object;
    uint8_t crouch_or_jump;
    float yaw, pitch;
    float cos_pitch, sin_pitch;
    Vector3D track_offset;
    float track_magnitude;
    local_player_control *player;

    chimera__spectate_fp_camera_position(&basis, input->local_player_index);

    *(real_point3d *)&command->parameters.position = basis.position;
    command->timer = 0.0f;
    command->flags = 0;
    command->parameters.field_of_view = 1.2217305f; 

    if (tp->initialized != 0 &&
        (basis.unit != tp->unit || basis.seat_index != tp->seat_index)) {
        command->timer = 1.0f;
    }
    tp->unit = basis.unit;
    tp->seat_index = basis.seat_index;

    if (basis.marker_offset != 0) {
        unit_object = ((object_header *)object_data->data)[halo::datum_slot(basis.unit)].data;
        crouch_or_jump = (uint8_t)((((unit_data *)((uint8_t *)unit_object + k_unit_data_offset))
            ->control_flags & 3) != 0);

        if (crouch_or_jump != tp->crouch_or_jump) {
            command->interpolation_flags[_observer_parameter_focus_offset] = 1;
            if (command->channel_times[_observer_parameter_focus_offset] < 0.5f) {
                command->channel_times[_observer_parameter_focus_offset] = 0.5f;
            }
            tp->crouch_or_jump = crouch_or_jump;
        }

        if (!input->has_look_input) {
            if (tp->yaw_offset != 0.0f || tp->pitch_offset != 0.0f) {
                tp->yaw_offset = 0.0f;
                tp->pitch_offset = 0.0f;
            }
        } else {
            tp->yaw_offset = input->yaw_delta + tp->yaw_offset;
            tp->pitch_offset = input->pitch_delta + tp->pitch_offset;
            command->interpolation_flags[_observer_parameter_orientation] = 1;
            if (command->channel_times[_observer_parameter_orientation] < 0.4f) {
                command->channel_times[_observer_parameter_orientation] = 0.4f;
            }
        }

        {
            float distance_scale = tp->distance_scale - input->zoom_delta * 0.05f;
            if (distance_scale < 0.0f) {
                distance_scale = 0.0f;
            } else if (distance_scale > 5.0f) {
                distance_scale = 5.0f;
            }
            tp->distance_scale = distance_scale;
        }
        if (input->zoom_delta != 0.0f) {
            command->interpolation_flags[_observer_parameter_distance] = 1;
            if (command->channel_times[_observer_parameter_distance] < 0.4f) {
                command->channel_times[_observer_parameter_distance] = 0.4f;
            }
            command->interpolation_flags[_observer_parameter_focus_offset] = 1;
            if (command->channel_times[_observer_parameter_focus_offset] < 0.4f) {
                command->channel_times[_observer_parameter_focus_offset] = 0.4f;
            }
        }

        player = &player_control_globals_ptr->local_players[input->local_player_index];
        yaw = player->yaw + tp->yaw_offset;
        pitch = player->pitch + tp->pitch_offset;
        if (pitch < -1.5707964f) {
            pitch = -1.5707964f;
        } else if (pitch > 1.5707964f) {
            pitch = 1.5707964f;
        }

        cos_pitch = (float)fcos((double)pitch);
        sin_pitch = (float)fsin((double)pitch);

        command->parameters.forward.i = (float)fcos((double)yaw) * cos_pitch;
        command->parameters.forward.j = (float)fsin((double)yaw) * cos_pitch;
        command->parameters.forward.k = sin_pitch;

        halo::camera::first_person_camera_track_offset((unit_camera_properties *)basis.marker_offset, pitch,
            &track_offset);

        track_magnitude = (float)sqrt((double)(track_offset.i * track_offset.i +
            track_offset.j * track_offset.j + track_offset.k * track_offset.k));

        command->parameters.focus_offset.i =
            (cos_pitch * track_magnitude + track_offset.i) * tp->distance_scale;
        command->parameters.focus_offset.j = -(track_offset.j * tp->distance_scale);
        command->parameters.focus_offset.k =
            (sin_pitch * track_magnitude + track_offset.k) * tp->distance_scale;

        {
            float distance = (track_magnitude - 0.6f) * tp->distance_scale + 0.6f;
            if (distance <= 0.6f) {
                distance = 0.6f;
            }
            command->parameters.distance = distance;
        }

        object_get_root_object_velocities(basis.unit, (real_vector3d *)&command->velocity, 0);
        command->flags |= _observer_command_valid_bit;
    }

    halo::camera::vector3d_compute_up_from_forward(&command->parameters.forward, &command->parameters.up);
    tp->initialized = 1;
}

/**
 * Original function camera_track_compute_pov; the author notes are in
 * docs/original/camera/camera_track_compute_pov.c.txt.
 *
 * Register convention in the original: matches director_pov_proc exactly --
 * (director_camera_data *data, camera_input *input, observer_command *command), cdecl, all
 * three on the stack.
 *
 * @address 0x445380
 */
void TrackCamera::compute_pov(director_camera_data *data, camera_input *input, observer_command *command)
{
    dead_camera_data *dead = &data->dead;
    Point3D focus_position;

    if (dead->target_unit != k_datum_index_none) {
        object *target = object_try_and_get(dead->target_unit, k_all_object_types);
        if (target != (object *)0) {
            focus_position = *(Point3D *)&target->bounding_center;
        } else {
            focus_position = dead->focus;
        }
    } else {
        focus_position = dead->focus;
    }

    command->parameters.position = focus_position;
    command->parameters.distance = dead->distance;

    command->parameters.forward.i = (real)cos((double)dead->pitch) * (real)cos((double)dead->yaw);
    command->parameters.forward.j = (real)cos((double)dead->pitch) * (real)sin((double)dead->yaw);
    command->parameters.forward.k = (real)sin((double)dead->pitch);

    halo::camera::vector3d_compute_up_from_forward((Vector3D *)&command->parameters.forward, (Vector3D *)&command->parameters.up);
    command->parameters.field_of_view = dead->field_of_view;
    command->parameters.focus_offset = *(Vector3D *)global_origin3d_pointer;
    command->velocity = *(Vector3D *)global_origin3d_pointer;

    command->flags = 1; 

    command->timer = (dead->transition_time >= 0.0f) ? dead->transition_time : 0.0f;

    command->interpolation_flags[_observer_parameter_position] = 3;
    command->channel_times[_observer_parameter_position] = 0.0f;

    if (dead->transition_time == 3.0f) {
        
        
        command->parameters.distance = 0.5f;
        command->interpolation_flags[_observer_parameter_distance] = 3;
        command->channel_times[_observer_parameter_distance] = 0.0f;
    }

    dead->transition_time -= input->dt;
    dead->retarget_time -= input->dt;
    if (dead->retarget_time < 0.0f) {
        dead->retarget_time = 0.0f;
    }

    if (dead->retarget_time == 0.0f && game_time->paused == 0) {
        uint8_t has_teammate = halo::camera::camera_dead_player_has_teammate(dead->local_player);
        datum_index new_target = halo::camera::camera_dead_find_next_teammate(dead->local_player, dead->target_player,
                                                                  (uint8_t)has_teammate);
        datum_index new_unit = k_datum_index_none;

        dead->target_player = new_target;
        if (new_target != k_datum_index_none) {
            player *p = (player *)halo::memory::datum_get(new_target, player_data);
            if (p == (player *)0) {
                dead->target_player = dead->local_player;
            }
            p = (player *)((uint8_t *)player_data->data + (halo::datum_slot(dead->target_player)) * sizeof(player));
            new_unit = p->unit;
        }

        if (new_unit != dead->target_unit && new_unit != k_datum_index_none) {
            dead->transition_time = 3.0f;
            dead->target_unit = new_unit;
        }

        dead->retarget_time = (current_game_engine != (game_engine_definition *)0) ? 15.0f : 3.0f;
    }
}

/**
 * Original function camera_debug_compute_pov; the author notes are in
 * docs/original/camera/camera_debug_compute_pov.c.txt.
 *
 * Register convention in the original: matches director_pov_proc exactly --
 * (director_camera_data *data, camera_input *input, observer_command *command), cdecl, all
 * three on the stack (confirmed: `mov esi,[ebp+0x10]` loads the 3rd stack slot as `command`
 * right after the prologue).
 *
 * @address 0x444d50
 */
void DebugCamera::compute_pov(director_camera_data *data, camera_input *input, observer_command *command)
{
    real time_scale;
    Point3D default_position;

    default_position = *(Point3D *)global_zero_vector3d_pointer;

    time_scale = (network_game_mode == 1 || network_game_mode == 2) ? 1.0f : game_time->speed;

    command->flags = 8; 
    if (game_time->paused) {
        command->flags |= 0x20; 
    }

    switch (camera_script.mode) {
    case _camera_script_mode_point: {
        if (camera_script.object != k_datum_index_none) {
            object *obj = object_try_and_get(camera_script.object, k_all_object_types);
            if (obj == (object *)0) {
                break; 
            }
            default_position = *(Point3D *)&obj->bounding_center;
        }

        command->timer = (time_scale == 0.0f) ? 0.0f : camera_script.time_remaining / time_scale;
        command->parameters.field_of_view = camera_script.field_of_view;
        command->parameters.forward = camera_script.forward;
        command->parameters.up = camera_script.up;

        if (camera_script.object == k_datum_index_none) {
            command->parameters.position = camera_script.position;
            command->flags |= 1;
        } else {
            
            
            real yaw = (real)atan2((double)camera_script.forward.j, (double)camera_script.forward.i);
            real dot = camera_script.position.x * camera_script.forward.i +
                       camera_script.position.y * camera_script.forward.j +
                       camera_script.position.z * camera_script.forward.k;
            real remaining_x, remaining_y, remaining_z;
            if (dot > 0.0f) {
                dot = 0.0f;
            }

            command->parameters.position = default_position;
            command->parameters.distance = -dot;

            remaining_x = camera_script.position.x - dot * camera_script.forward.i;
            remaining_y = camera_script.position.y - dot * camera_script.forward.j;
            remaining_z = camera_script.position.z - dot * camera_script.forward.k;

            command->channel_times[_observer_parameter_position] = 0.0f;
            command->interpolation_flags[_observer_parameter_position] = 1;
            command->flags |= 1;

            command->parameters.focus_offset.i = (real)sin((double)yaw) * remaining_y +
                                                  remaining_x * (real)cos((double)yaw);
            command->parameters.focus_offset.j = (real)sin((double)yaw) * remaining_x -
                                                  (real)cos((double)yaw) * remaining_y;
            command->parameters.focus_offset.k = remaining_z;
        }
        break;
    }

    case _camera_script_mode_animation: {
        ModelAnimations *anims = (ModelAnimations *)halo::cache::globals().tag_instances[halo::datum_slot(camera_script.animation_tag)].data;
        ModelAnimationsAnimation *anim =
            (ModelAnimationsAnimation *)((uint8_t *)anims->animations.pointer +
                                         (int32_t)camera_script.animation_index * sizeof(ModelAnimationsAnimation));
        int16_t frame = (int16_t)__ftol(
            (double)anim->frame_count - (double)(camera_script.time_remaining * 30.0f)); 
        int16_t frame_index;
        real_matrix4x3 sample;

        if (frame < 0) {
            frame_index = 0;
        } else if (frame > anim->frame_count - 1) {
            frame_index = (int16_t)(anim->frame_count - 1);
        } else {
            frame_index = frame;
        }

        halo::models::animation_get_root_node_matrix(&sample, frame_index, anim, 0);

        command->parameters.forward = *(Vector3D *)&sample.forward;
        command->parameters.up = *(Vector3D *)&sample.up;
        command->parameters.position = *(Point3D *)&sample.position;
        command->parameters.distance = 0.0f;
        command->timer = 0.0f;
        command->parameters.field_of_view = 1.2217305f; 
        command->flags |= 1;
        break;
    }

    case _camera_script_mode_first_person: {
        object *obj = object_try_and_get(camera_script.object, 3);
        if (obj != (object *)0) {
            halo::camera::first_person_camera_command_for_unit(camera_script.object, command);
        }
        break;
    }

    case _camera_script_mode_dead: {
        object *obj = object_try_and_get(camera_script.object, 3);
        if (obj != (object *)0) {
            director_camera_data *track_data = data;
            if (camera_script.changed) {
                track_data = (director_camera_data *)halo::camera::dead_camera_new(&data->dead, input->local_player_index,
                                                                       camera_script.object);
            }
            halo::camera::camera_track_compute_pov(track_data, input, command);
        }
        break;
    }
    }

    camera_script.changed = 0;
    camera_script.time_remaining -= time_scale * input->dt;
    if (camera_script.time_remaining < 0.0f) {
        camera_script.time_remaining = 0.0f;
    }
}

/**
 * Original function editor_camera_compute_pov; the author notes are in
 * docs/original/camera/editor_camera_compute_pov.c.txt.
 *
 * Register convention in the original: director_pov_proc (cdecl, three stack arguments; ecx =
 * [esp+0x8] input, ebx = [esp+0x14] data, ebp = [esp+0x20] command after the prologue).
 *
 * @address 0x446e90
 */
void EditorCamera::compute_pov(director_camera_data *data, camera_input *input, observer_command *command)
{
    editor_camera_data *camera = &data->editor;
    float cos_pitch;

    if (input->has_look_input) {
        float pitch;

        camera->yaw = input->yaw_delta + camera->yaw;
        pitch = input->pitch_delta + camera->pitch;
        if (pitch < -1.5676548f) {
            pitch = -1.5676548f;
        } else if (pitch > 1.5676548f) {
            pitch = 1.5676548f;
        }
        camera->pitch = pitch;
        camera->roll = input->roll_delta + camera->roll;
    }

    command->timer = 0.3f; 
    cos_pitch = (float)cos(camera->pitch);
    command->parameters.forward.i = (float)cos(camera->yaw) * cos_pitch;
    command->parameters.forward.j = (float)sin(camera->yaw) * cos_pitch;
    command->parameters.forward.k = (float)sin(camera->pitch);
    halo::camera::vector3d_compute_up_from_forward(&command->parameters.forward, &command->parameters.up);
    halo::math::vector3d_rotate_about_axis(*((real_vector3d *)&command->parameters.up),
        *((const real_vector3d *)&command->parameters.forward),
        (real)sin(camera->roll), (real)cos(camera->roll));

    if (input->has_look_input) {
        float cos_yaw = (float)cos(camera->yaw);
        float sin_yaw = (float)sin(camera->yaw);
        float move_x = cos_yaw * input->move_forward - sin_yaw * input->move_left;
        float move_y = sin_yaw * input->move_forward + cos_yaw * input->move_left;

        camera->position.x = move_x + camera->position.x;
        camera->position.y = move_y + camera->position.y;
        camera->position.z = input->move_up + camera->position.z;
    }

    command->parameters.position = camera->position;
    command->parameters.focus_offset = *(const Vector3D *)global_origin3d_pointer;
    command->parameters.distance = 0.0f;
    command->parameters.field_of_view = camera->field_of_view;
    command->flags = _observer_command_valid_bit;
}

/**
 * Original function editor_camera_set_position_and_direction; the author notes are in
 * docs/original/camera/editor_camera_set_position_and_direction.c.txt.
 *
 * Register convention in the original: output in EAX (in_EAX), direction vector in ECX
 * (in_ECX), position in EDX (in_EDX); no stack parameters.
 *
 * @address 0x446e30
 */
void EditorCamera::set_position_and_direction(editor_camera_data *out, Vector3D *direction, Point3D *position)
{
    out->roll = 0.0f;
    out->field_of_view = 1.2217305f; 
    out->position = *position;

    out->yaw = (real)atan2((double)direction->j, (double)direction->i);
    out->pitch = (real)atan2((double)direction->k,
                              sqrt((double)direction->i * (double)direction->i +
                                   (double)direction->j * (double)direction->j));
}

/**
 * Original function flying_camera_attach_to_object; the author notes are in
 * docs/original/camera/flying_camera_attach_to_object.c.txt.
 *
 * @address 0x446470
 */
void FlyingCamera::attach_to_object(datum_index object_index)
{
    flying_camera_attached_object = object_index;
    if (flying_camera_data == 0) {
        return;
    }
    if (object_index != k_datum_index_none) {
        object *attached = ((object_header *)object_data->data)[halo::datum_slot(object_index)].data;

        flying_camera_attached_offset.i = flying_camera_data->position.x - attached->bounding_center.x;
        flying_camera_attached_offset.j = flying_camera_data->position.y - attached->bounding_center.y;
        flying_camera_attached_offset.k = flying_camera_data->position.z - attached->bounding_center.z;
    } else {
        flying_camera_attached_offset = *(const Vector3D *)global_origin3d_pointer;
    }
}

/**
 * Original function flying_camera_compute_pov; the author notes are in
 * docs/original/camera/flying_camera_compute_pov.c.txt.
 *
 * Register convention in the original: director_pov_proc (cdecl, three stack arguments; edi =
 * [esp+0x10] input after the two pushes, the data / command arguments are re-read at 0x446591
 * / 0x446595).
 *
 * @address 0x4464f0
 */
void FlyingCamera::compute_pov(director_camera_data *data, camera_input *input, observer_command *command)
{
    if (flying_camera_follow_script) {
        render_camera *camera;
        editor_camera_data *flying;

        if (!input->has_look_input) {
            halo::camera::camera_debug_compute_pov((director_camera_data *)0, input, command);
            return;
        }
        camera = &((flying_render_frame *)flying_camera_render_frame)->camera;
        flying = flying_camera_data;
        flying->position = *(Point3D *)&camera->position;
        flying->yaw = (float)atan2(camera->forward.j, camera->forward.i);
        flying->pitch = (float)atan2(camera->forward.k,
            sqrt(camera->forward.i * camera->forward.i + camera->forward.j * camera->forward.j));
        halo::camera::flying_camera_attach_to_object(flying_camera_attached_object);
        if (flying_camera_current_mode != 0) {
            flying_camera_transition_procs[flying_camera_current_mode][1](flying);
        }
    }

    flying_camera_update_procs[flying_camera_current_mode](data, input, command);

    if (flying_camera_follow_script) {
        command->flags |= _observer_command_valid_bit | _observer_command_snap_bit;
        command->timer = 0.0f;
    }
}

/**
 * Original function flying_camera_enter_flying; the author notes are in
 * docs/original/camera/flying_camera_enter_flying.c.txt.
 *
 * Register convention in the original: cdecl, the record as the only stack argument.
 *
 * @address 0x4469a0
 */
void FlyingCamera::enter_flying(editor_camera_data *data)
{
    render_camera *camera = &((flying_render_frame *)flying_camera_render_frame)->camera;

    flying_camera_saved_orbiting = *(orbiting_camera_data *)data;
    flying_camera_saved_orbiting_valid = 1;
    data->position = *(Point3D *)&camera->position;
    data->yaw = (float)atan2(camera->forward.j, camera->forward.i);
    data->pitch = (float)atan2(camera->forward.k,
        sqrt(camera->forward.i * camera->forward.i + camera->forward.j * camera->forward.j));
    halo::camera::flying_camera_attach_to_object(flying_camera_attached_object);
}

/**
 * Original function flying_camera_enter_orbiting; the author notes are in
 * docs/original/camera/flying_camera_enter_orbiting.c.txt.
 *
 * Register convention in the original: cdecl, the record as the only stack argument.
 *
 * @address 0x446a10
 */
void FlyingCamera::enter_orbiting(editor_camera_data *data)
{
    orbiting_camera_data *orbit = (orbiting_camera_data *)data;
    render_camera *camera;

    flying_camera_saved_flying = *data;
    if (flying_camera_saved_orbiting_valid) {
        *orbit = flying_camera_saved_orbiting;
        return;
    }
    camera = &((flying_render_frame *)flying_camera_render_frame)->camera;
    orbit->unknown_00 = 0.0f;
    orbit->distance = 1.0f;
    orbit->unknown_08 = 0.0f;
    orbit->yaw = (float)atan2(camera->forward.j, camera->forward.i);
    orbit->pitch = (float)atan2(camera->forward.k,
        sqrt(camera->forward.i * camera->forward.i + camera->forward.j * camera->forward.j));
}

/**
 * Original function flying_camera_initialize; the author notes are in
 * docs/original/camera/flying_camera_initialize.c.txt.
 *
 * @address 0x446350
 */
void FlyingCamera::initialize(editor_camera_data *data, int16_t local_player_index)
{
    float cos_pitch;
    float forward_x;
    float forward_y;
    float forward_z;

    if (!flying_camera_home_initialized) {
        ScenarioPlayerStartingLocation *start = 0;

        if (halo::scenario::globals().scenario->player_starting_locations.count != 0) {
            start = (ScenarioPlayerStartingLocation *)halo::scenario::globals().scenario->player_starting_locations.pointer;
        }
        if (start != 0) {
            flying_camera_home_location.position = start->position;
            flying_camera_home_location.yaw = start->facing;
        } else {
            flying_camera_home_location.position.x = 0.0f;
            flying_camera_home_location.position.y = 0.0f;
            flying_camera_home_location.position.z = 0.0f;
            flying_camera_home_location.yaw = 0.0f;
            flying_camera_home_location.pitch = 0.0f;
        }
    }

    cos_pitch = (float)cos(flying_camera_home_location.pitch);
    flying_camera_home_initialized = 1;
    forward_x = (float)cos(flying_camera_home_location.yaw) * cos_pitch;
    forward_y = (float)sin(flying_camera_home_location.yaw) * cos_pitch;
    forward_z = (float)sin(flying_camera_home_location.pitch);

    data->position.y = 0.0f;
    data->position.x = 0.0f;
    data->yaw = 0.0f;
    data->pitch = 0.0f;
    data->roll = 0.0f;
    data->field_of_view = 1.2217305f; 
    data->position = flying_camera_home_location.position;
    data->yaw = (float)atan2(forward_y, forward_x);
    data->pitch = (float)atan2(forward_z, sqrt(forward_x * forward_x + forward_y * forward_y));

    if (local_player_index == 0) {
        flying_camera_data = data;
    }
    if (flying_camera_current_mode != 0) {
        flying_camera_transition_procs[flying_camera_current_mode][1](data);
    }
}

/**
 * Original function flying_camera_update; the author notes are in
 * docs/original/camera/flying_camera_update.c.txt.
 *
 * Register convention in the original: director_pov_proc (cdecl, three stack arguments: esi =
 * [esp+0x28] data, ebx = [esp+0x24] input, ebp = [esp+0x34] command after the four pushes).
 *
 * @address 0x4465d0
 */
void FlyingCamera::update(director_camera_data *data, camera_input *input, observer_command *command)
{
    editor_camera_data *camera = &data->editor;
    Vector3D *forward = &command->parameters.forward;
    Vector3D *up = &command->parameters.up;
    real_vector3d right;
    float cos_pitch;
    float cos_yaw;
    float sin_yaw;
    float move_x;
    float move_y;
    float move_z;
    Point3D position;

    if (input->has_look_input) {
        float pitch;

        camera->yaw = input->yaw_delta + camera->yaw;
        pitch = input->pitch_delta + camera->pitch;
        if (pitch < -1.5676548f) {
            pitch = -1.5676548f;
        } else if (pitch > 1.5676548f) {
            pitch = 1.5676548f;
        }
        camera->pitch = pitch;
        if (flying_camera_allow_roll) {
            camera->roll = input->roll_delta + camera->roll;
        } else {
            camera->roll = 0.0f;
        }
    }

    command->timer = 0.3f; 
    cos_pitch = (float)cos(camera->pitch);
    right.k = 0.0f;
    forward->i = (float)cos(camera->yaw) * cos_pitch;
    forward->j = (float)sin(camera->yaw) * cos_pitch;
    forward->k = (float)sin(camera->pitch);
    right.j = -forward->i;
    right.i = forward->j;
    if (halo::math::vector3d_normalize_with_length(right) == 0.0f) {
        right.i = 1.0f;
        right.j = 0.0f;
        right.k = 0.0f;
    }
    up->i = right.j * forward->k - right.k * forward->j;
    up->j = right.k * forward->i - right.i * forward->k;
    up->k = right.i * forward->j - right.j * forward->i;
    halo::math::vector3d_rotate_about_axis(*(real_vector3d *)up, *(const real_vector3d *)forward,
        (real)sin(camera->roll), (real)cos(camera->roll));

    cos_yaw = (float)cos(camera->yaw);
    sin_yaw = (float)sin(camera->yaw);
    move_x = flying_camera_speed * (cos_yaw * input->move_forward - sin_yaw * input->move_left);
    move_y = flying_camera_speed * (sin_yaw * input->move_forward + cos_yaw * input->move_left);
    move_z = flying_camera_speed * input->move_up;

    if (flying_camera_attached_object != k_datum_index_none &&
        object_try_and_get(flying_camera_attached_object, k_all_object_types) != 0) {
        object *attached;

        flying_camera_attached_offset.i = flying_camera_attached_offset.i + move_x;
        flying_camera_attached_offset.j = flying_camera_attached_offset.j + move_y;
        flying_camera_attached_offset.k = flying_camera_attached_offset.k + move_z;
        attached = ((object_header *)object_data->data)[halo::datum_slot(flying_camera_attached_object)].data;
        position.x = flying_camera_attached_offset.i + attached->bounding_center.x;
        position.y = flying_camera_attached_offset.j + attached->bounding_center.y;
        position.z = flying_camera_attached_offset.k + attached->bounding_center.z;
    } else {
        position.x = move_x + camera->position.x;
        position.y = move_y + camera->position.y;
        position.z = move_z + camera->position.z;
    }

    camera->position = position;
    command->parameters.position = position;
    command->parameters.focus_offset = *(const Vector3D *)global_origin3d_pointer;
    command->parameters.distance = 0.0f;
    command->parameters.field_of_view = 1.2217305f; 
    command->flags = _observer_command_valid_bit;
}

/**
 * Original function orbiting_camera_update; the author notes are in
 * docs/original/camera/orbiting_camera_update.c.txt.
 *
 * Register convention in the original: director_pov_proc (cdecl, three stack arguments; ebx =
 * [esp+0x2c] data, edi = [esp+0x30] input, ebp = [esp+0x34] command after the four pushes).
 *
 * @address 0x446870
 */
void OrbitingCamera::update(director_camera_data *data, camera_input *input, observer_command *command)
{
    orbiting_camera_data *orbit = &data->orbiting;
    camera_basis_out basis;
    float distance;

    chimera__spectate_fp_camera_position(&basis, input->local_player_index);
    command->parameters.position = *(Point3D *)&basis.position;

    if (input->has_look_input) {
        float pitch;

        orbit->yaw = input->yaw_delta + orbit->yaw;
        pitch = input->pitch_delta + orbit->pitch;
        if (pitch < -1.2566371f) {
            pitch = -1.2566371f;
        } else if (pitch > 1.2566371f) {
            pitch = 1.2566371f;
        }
        orbit->pitch = pitch;
        directors[input->local_player_index].look_input_consumed = 1;
    }

    distance = orbit->distance - input->zoom_delta * 0.33333334f;
    if (distance <= 0.6f) {
        distance = 0.6f;
    }
    orbit->distance = distance;

    if (basis.unit != k_datum_index_none) {
        float cos_pitch = (float)cos(orbit->pitch);

        command->parameters.forward.i = (float)cos(orbit->yaw) * cos_pitch;
        command->parameters.forward.j = (float)sin(orbit->yaw) * cos_pitch;
        command->parameters.forward.k = (float)sin(orbit->pitch);
        halo::camera::vector3d_compute_up_from_forward(&command->parameters.forward, &command->parameters.up);
        object_get_root_object_velocities(basis.unit, (real_vector3d *)&command->velocity, 0);
        command->flags = _observer_command_valid_bit;
    }

    command->parameters.focus_offset = *(const Vector3D *)global_origin3d_pointer;
    command->parameters.distance = orbit->distance;
    command->parameters.field_of_view = 1.2217305f; 
    command->timer = 0.5f;
}

}

namespace halo::camera {

void camera_first_person_compute_pov(director_camera_data *data, camera_input *input, observer_command *command)
{
    halo::camera::FirstPersonCamera::compute_pov(data, input, command);
}

void first_person_camera_apply_weapon_offset(real_point3d *position, datum_index unit, real_vector3d *aiming_direction)
{
    halo::camera::FirstPersonCamera::apply_weapon_offset(position, unit, aiming_direction);
}

void first_person_camera_command_for_unit(datum_index unit, observer_command *command)
{
    halo::camera::FirstPersonCamera::command_for_unit(unit, command);
}

void first_person_camera_deterministic(Point3D *out_position, datum_index unit, Vector3D *out_direction)
{
    halo::camera::FirstPersonCamera::deterministic(out_position, unit, out_direction);
}

void first_person_camera_for_unit_and_vector(observer_command *command, Vector3D *vector, datum_index unit)
{
    halo::camera::FirstPersonCamera::for_unit_and_vector(command, vector, unit);
}

void first_person_camera_track_offset(unit_camera_properties *properties, float angle, Vector3D *out)
{
    halo::camera::FirstPersonCamera::track_offset(properties, angle, out);
}

unit_camera_properties *unit_get_camera_properties(datum_index unit)
{
    return halo::camera::FirstPersonCamera::unit_properties(unit);
}

void camera_third_person_compute_pov(director_camera_data *data, camera_input *input, observer_command *command)
{
    halo::camera::ThirdPersonCamera::compute_pov(data, input, command);
}

void camera_track_compute_pov(director_camera_data *data, camera_input *input, observer_command *command)
{
    halo::camera::TrackCamera::compute_pov(data, input, command);
}

void camera_debug_compute_pov(director_camera_data *data, camera_input *input, observer_command *command)
{
    halo::camera::DebugCamera::compute_pov(data, input, command);
}

void editor_camera_compute_pov(director_camera_data *data, camera_input *input, observer_command *command)
{
    halo::camera::EditorCamera::compute_pov(data, input, command);
}

void editor_camera_set_position_and_direction(editor_camera_data *out, Vector3D *direction, Point3D *position)
{
    halo::camera::EditorCamera::set_position_and_direction(out, direction, position);
}

void flying_camera_attach_to_object(datum_index object_index)
{
    halo::camera::FlyingCamera::attach_to_object(object_index);
}

void flying_camera_compute_pov(director_camera_data *data, camera_input *input, observer_command *command)
{
    halo::camera::FlyingCamera::compute_pov(data, input, command);
}

void flying_camera_enter_flying(editor_camera_data *data)
{
    halo::camera::FlyingCamera::enter_flying(data);
}

void flying_camera_enter_orbiting(editor_camera_data *data)
{
    halo::camera::FlyingCamera::enter_orbiting(data);
}

void flying_camera_initialize(editor_camera_data *data, int16_t local_player_index)
{
    halo::camera::FlyingCamera::initialize(data, local_player_index);
}

void flying_camera_update(director_camera_data *data, camera_input *input, observer_command *command)
{
    halo::camera::FlyingCamera::update(data, input, command);
}

void orbiting_camera_update(director_camera_data *data, camera_input *input, observer_command *command)
{
    halo::camera::OrbitingCamera::update(data, input, command);
}

}
