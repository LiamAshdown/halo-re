#include "halo/camera/observer.hpp"
#include "halo/scenario/leaf.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/camera/api.hpp"
#include "halo/units/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/scenario/scenario.hpp"
#include "halo/game/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/camera/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/ai/api.hpp"

namespace {
/** The floats of an observer parameter record (parameters, derivatives, command vectors), which the spline code walks channel by channel. */
template <typename T>
float *float_view(T &record)
{
    static_assert(sizeof(T) % sizeof(float) == 0);
    return reinterpret_cast<float *>(&record);
}
}


static auto &observers = halo::link::ref<observer [1]>(halo::camera::vars().observers);
static auto &observer_dt = halo::link::ref<float>(halo::camera::vars().observer_dt);
static auto &observer_derivative_float_counts = halo::link::ref<int16_t [5]>(halo::camera::vars().observer_derivative_float_counts);
static auto &observer_channel_acceleration_limit = halo::link::ref<float [5]>(halo::camera::vars().observer_channel_acceleration_limit);
static auto &observer_parameter_float_counts = halo::link::ref<int16_t [5]>(halo::camera::vars().observer_parameter_float_counts);
static auto &global_origin3d_pointer = halo::link::ref<const real_point3d *>(halo::ai::vars().global_origin3d_pointer);
static auto &global_zero_vector3d_pointer = halo::link::ref<real_point3d *>(halo::units::vars().global_zero_vector3d_pointer);
static auto &directors = halo::link::ref<director [1]>(halo::camera::vars().directors);
static auto &matrix4x3_multiply_ptr = halo::link::ref<void (*)(void *a, void *b, void *out)>(halo::camera::vars().matrix4x3_multiply_ptr);

namespace halo::camera {

/**
 *
 * Register convention in the original: local player index in DI (unaff_DI); no other
 * parameters.
 *
 * @address 0x447b50
 */
void ObserverHandle::advance()
{
    int16_t local_player_index = (int16_t)player_handle;

    observer *o = &observers[local_player_index];
    int32_t i;

    if ((o->command->flags & _observer_command_frozen_bit) != 0) {
        return;
    }

    halo::camera::observer_compute_remaining_offset(float_view(o->current_command.parameters),
        float_view(o->parameters), float_view(o->remaining_offset));
    halo::camera::observer_compute_spline_coefficients(local_player_index);
    halo::camera::observer_evaluate_spline_acceleration(local_player_index);
    halo::camera::observer_evaluate_spline_velocity(local_player_index);
    halo::camera::observer_evaluate_spline_value_and_orthonormalize(local_player_index);

    for (i = 0; i < k_observer_parameter_count; i++) {
        float t = o->current_command.channel_times[i] - observer_dt;

        if (t <= 0.0f) {
            t = 0.0f;
        }
        o->current_command.channel_times[i] = t;
    }
}

/**
 *
 * Register convention in the original: local player index in AX (in_AX); no other parameters.
 *
 * @address 0x448900
 */
void ObserverHandle::commit()
{
    int16_t local_player_index = (int16_t)player_handle;

    observer *o = &observers[local_player_index];
    observer_camera *camera = &observers[local_player_index].camera;
    real_point3d position;
    float distance;
    float fov;
    float forward_i, forward_j;
    float horizontal_magnitude;
    int32_t leaf_index;
    float water_depth;

    position = *(real_point3d *)&o->parameters.position;

    if (0.0f <= o->parameters.distance) {
        distance = (o->parameters.distance <= 3.4028235e+38f) ? o->parameters.distance :
            3.4028235e+38f;
    } else {
        distance = 0.0f;
    }

    if (0.001 <= o->parameters.field_of_view) {
        fov = (o->parameters.field_of_view <= 1.5707964f) ? o->parameters.field_of_view :
            1.5707964f;
    } else {
        fov = 0.001f;
    }
    o->parameters.field_of_view = fov;

    if (position.x < -5000.0f) position.x = -5000.0f;
    else if (5000.0f < position.x) position.x = 5000.0f;
    if (position.y < -5000.0f) position.y = -5000.0f;
    else if (5000.0f < position.y) position.y = 5000.0f;
    if (position.z < -5000.0f) position.z = -5000.0f;
    else if (5000.0f < position.z) position.z = 5000.0f;
    if (distance < 0.0f) distance = 0.0f;
    else if (5000.0f < distance) distance = 5000.0f;

    forward_i = o->parameters.forward.i;
    forward_j = o->parameters.forward.j;
    horizontal_magnitude = (float)halo::libm::sqrt((double)(forward_i * forward_i + forward_j * forward_j));
    if (0.0001 <= halo::libm::fabs((double)horizontal_magnitude)) {
        float inv = 1.0f / horizontal_magnitude;
        forward_i = inv * forward_i;
        forward_j = inv * forward_j;
    }

    position.x = forward_i * o->parameters.focus_offset.i + forward_j * o->parameters.focus_offset.j
        + position.x;
    position.y = (forward_j * o->parameters.focus_offset.i - forward_i * o->parameters.focus_offset.j)
        + position.y;
    position.z = position.z + o->parameters.focus_offset.k;

    if ((o->current_command.flags & _observer_command_no_collision_bit) == 0 && distance != 0.0f) {
        halo::camera::observer_avoid_collision((real_vector3d *)&o->parameters.forward, &position,
            (real_vector3d *)&o->parameters.up, &distance, 0.02f);
    }

    camera->position.x = position.x - distance * o->parameters.forward.i;
    camera->position.y = position.y - distance * o->parameters.forward.j;
    camera->position.z = position.z - distance * o->parameters.forward.k;

    leaf_index = (int32_t)halo::physics::bsp3d_node_find_leaf(0, halo::physics::globals().collision_bsp, (real_point3d *)&camera->position);
    if (leaf_index != -1) {
        int16_t new_cluster =
            halo::scenario::structure_leaf_cluster(leaf_index);

        if (new_cluster != -1) {
            if (new_cluster != camera->cluster_index) {
                halo::cache::predicted_resource_list_touch(
                    &((ScenarioStructureBSPCluster *)halo::scenario::globals().structure_bsp->clusters.pointer)[new_cluster]
                        .predicted_resources);
            }
            camera->leaf_index = leaf_index;
            camera->cluster_index = new_cluster;
        }
    }

    water_depth = halo::scenario::location_view((bsp_leaf_reference *)&camera->leaf_index).water_surface_distance((real_point3d *)&camera->position);
    if (halo::libm::fabs((double)water_depth) < 0.05000000074505806) {
        if (water_depth <= 0.0f) {
            camera->position.z = water_depth + camera->position.z + 0.05f;
        } else {
            camera->position.z = camera->position.z - (0.05f - water_depth);
        }
    }

    if (camera->position.x < -5000.0f) camera->position.x = -5000.0f;
    else if (5000.0f < camera->position.x) camera->position.x = 5000.0f;
    if (camera->position.y < -5000.0f) camera->position.y = -5000.0f;
    else if (5000.0f < camera->position.y) camera->position.y = 5000.0f;
    if (camera->position.z < -5000.0f) camera->position.z = -5000.0f;
    else if (5000.0f < camera->position.z) camera->position.z = 5000.0f;

    camera->velocity.i = -o->velocity.position.i;
    camera->velocity.j = -o->velocity.position.j;
    camera->velocity.k = -o->velocity.position.k;
    camera->forward = o->parameters.forward;
    camera->up = o->parameters.up;
    camera->field_of_view = fov;
}

/**
 *
 * Register convention in the original: local player index in DX (in_DX); no other parameters.
 *
 * @address 0x447ab0
 */
void ObserverHandle::set_command()
{
    int16_t local_player_index = (int16_t)player_handle;

    observer *self = &observers[local_player_index];
    observer_command *command = self->command;
    int32_t i;

    if ((command->flags & _observer_command_valid_bit) == 0) {
        return;
    }

    for (i = 0; i < k_observer_parameter_count; i++) {
        float clamp_source;
        uint8_t use_clamp = 0;

        if ((command->interpolation_flags[i] & _observer_interpolation_own_time_bit) == 0) {
            if (command->timer < self->current_command.channel_times[i] &&
                (command->flags & _observer_command_snap_bit) == 0) {
                clamp_source = self->current_command.channel_times[i];
                use_clamp = 1;
            } else {
                command->channel_times[i] = command->timer;
            }
        } else if ((command->interpolation_flags[i] & _observer_interpolation_exact_bit) == 0 &&
            command->channel_times[i] < self->current_command.channel_times[i]) {
            clamp_source = self->current_command.channel_times[i];
            use_clamp = 1;
        }

        if (use_clamp) {
            command->channel_times[i] = (clamp_source <= 2.0f) ? clamp_source : 2.0f;
        }
    }

    self->current_command = *command;
}

/**
 *
 * Register convention in the original: local player index in DX (in_DX); no other parameters.
 *
 * @address 0x447be0
 */
void ObserverHandle::compute_spline_coefficients()
{
    int16_t local_player_index = (int16_t)player_handle;

    observer *o = &observers[local_player_index];
    float *remaining_offset = float_view(o->remaining_offset);
    float *derivative_velocity = float_view(o->velocity);
    float *acceleration = float_view(o->acceleration);
    float *coefficient_t5 = float_view(o->coefficient_t5);
    float *coefficient_t4 = float_view(o->coefficient_t4);
    float *coefficient_t3 = float_view(o->coefficient_t3);
    float *coefficient_t2 = float_view(o->coefficient_t2);
    float *coefficient_t1 = float_view(o->coefficient_t1);
    float *coefficient_t0 = float_view(o->coefficient_t0);
    float *command_velocity = float_view(o->current_command.velocity);
    int16_t channel;
    int32_t float_index = 0;

    for (channel = 0; channel < k_observer_parameter_count; channel++) {
        int16_t count = observer_derivative_float_counts[channel];

        if ((o->current_command.flags & _observer_command_valid_bit) != 0 &&
            observer_dt < o->current_command.channel_times[channel]) {
            float inv_t = 1.0f / o->current_command.channel_times[channel];
            float inv_t2 = inv_t * inv_t;
            float inv_t3 = inv_t2 * inv_t;
            float inv_t4 = inv_t3 * inv_t;
            int16_t i;

            for (i = 0; i < count; i++) {
                int32_t idx = float_index + i;

                coefficient_t5[idx] = inv_t3 * acceleration[idx] * 0.5f -
                    (inv_t4 * inv_t * remaining_offset[idx] * 6.0f +
                     inv_t4 * derivative_velocity[idx] * 3.0f);
                coefficient_t4[idx] = (inv_t4 * remaining_offset[idx] * 15.0f +
                     inv_t3 * derivative_velocity[idx] * 7.0f) - inv_t2 * acceleration[idx];
                coefficient_t3[idx] = inv_t * acceleration[idx] * 0.5f -
                    (inv_t3 * remaining_offset[idx] * 10.0f + inv_t2 * derivative_velocity[idx] *
                     4.0f);
                coefficient_t2[idx] = 0.0f;
                coefficient_t1[idx] = 0.0f;
                coefficient_t0[idx] = remaining_offset[idx];

                if (channel == _observer_parameter_position) {
                    float scaled_velocity = command_velocity[i] * 30.0f;

                    coefficient_t5[idx] -= inv_t4 * scaled_velocity * 3.0f;
                    coefficient_t4[idx] += inv_t3 * scaled_velocity * 8.0f;
                    coefficient_t3[idx] -= inv_t2 * scaled_velocity * 6.0f;
                    coefficient_t1[idx] += scaled_velocity;
                }
            }
        }
        float_index += count;
    }
}

/**
 *
 * Register convention in the original: local player index in AX (in_AX); no other parameters.
 *
 * @address 0x447e40
 */
void ObserverHandle::evaluate_spline_acceleration()
{
    int16_t local_player_index = (int16_t)player_handle;

    observer *o = &observers[local_player_index];
    float *coefficient_t5 = float_view(o->coefficient_t5);
    float *coefficient_t4 = float_view(o->coefficient_t4);
    float *coefficient_t3 = float_view(o->coefficient_t3);
    float *coefficient_t2 = float_view(o->coefficient_t2);
    float *acceleration = float_view(o->acceleration);
    int16_t channel;
    int32_t float_index = 0;

    for (channel = 0; channel < k_observer_parameter_count; channel++) {
        int16_t count = observer_derivative_float_counts[channel];
        float t = o->current_command.channel_times[channel] - observer_dt;
        float t2 = t * t;
        float t3 = t2 * t;
        int16_t i;

        if (t <= 0.0f) {
            for (i = 0; i < count; i++) {
                acceleration[float_index + i] = 0.0f;
            }
        } else {
            for (i = 0; i < count; i++) {
                int32_t idx = float_index + i;
                
                float value = (((t3 * coefficient_t5[idx] * 20.0f + t2 * coefficient_t4[idx] * 12.0f) +
                    t * coefficient_t3[idx] * 6.0f) + (coefficient_t2[idx] + coefficient_t2[idx]));

                acceleration[idx] = value;

                if (observer_channel_acceleration_limit[channel] < value ||
                    value < -observer_channel_acceleration_limit[channel]) {
                    int16_t other_channel;

                    for (other_channel = 0; other_channel < k_observer_parameter_count;
                         other_channel++) {
                        if (other_channel != channel &&
                            o->current_command.channel_times[other_channel] ==
                                o->current_command.channel_times[channel]) {
                            o->current_command.channel_times[other_channel] = 0.0f;
                        }
                    }
                    o->current_command.channel_times[channel] = 0.0f;
                }
            }
        }
        float_index += count;
    }
}

/**
 *
 * Register convention in the original: local player index in AX (in_AX); no other parameters.
 *
 * @address 0x448210
 */
void ObserverHandle::evaluate_spline_value_and_orthonormalize()
{
    int16_t local_player_index = (int16_t)player_handle;

    observer *o = &observers[local_player_index];
    float *command_parameters = float_view(o->current_command.parameters);
    float *coefficient_t5 = float_view(o->coefficient_t5);
    float *coefficient_t4 = float_view(o->coefficient_t4);
    float *coefficient_t3 = float_view(o->coefficient_t3);
    float *coefficient_t2 = float_view(o->coefficient_t2);
    float *coefficient_t1 = float_view(o->coefficient_t1);
    float *coefficient_t0 = float_view(o->coefficient_t0);
    float *velocity = float_view(o->velocity);
    float *parameters = float_view(o->parameters);
    float value[14];
    int16_t channel;
    int32_t derivative_index = 0;
    int32_t parameter_index = 0;

    for (channel = 0; channel < k_observer_parameter_count; channel++) {
        int16_t derivative_count = observer_derivative_float_counts[channel];
        int16_t parameter_count = observer_parameter_float_counts[channel];
        float t = o->current_command.channel_times[channel] - observer_dt;
        int16_t i;

        if (t > 0.0f || (o->current_command.flags & _observer_command_valid_bit) == 0) {
            if (t <= 0.0f) {
                for (i = 0; i < derivative_count; i++) {
                    value[i] = -(observer_dt * velocity[derivative_index + i]);
                }
            } else {
                float t3 = t * t * t;
                float t4 = t3 * t;

                for (i = 0; i < derivative_count; i++) {
                    int32_t idx = derivative_index + i;

                    value[i] = t * coefficient_t1[idx] + t * t * coefficient_t2[idx] +
                        t3 * coefficient_t3[idx] + t4 * coefficient_t4[idx] +
                        t4 * t * coefficient_t5[idx] + coefficient_t0[idx];
                }
            }

            if (channel < _observer_parameter_orientation) {
                
                for (i = 0; i < derivative_count; i++) {
                    parameters[parameter_index + i] = value[i] + parameters[parameter_index + i];
                }
            } else {
                
                halo::camera::vector3d_rotate_basis_by_axis_angle((Vector3D *)value,
                    (Vector3D *)&o->parameters.forward, (Vector3D *)&o->parameters.up);
            }
        } else {
            for (i = 0; i < parameter_count; i++) {
                parameters[parameter_index + i] = command_parameters[parameter_index + i];
            }
        }

        derivative_index += derivative_count;
        parameter_index += parameter_count;
    }

    {
        Vector3D *forward = &o->parameters.forward;
        Vector3D *up = &o->parameters.up;
        float check;
        float length;

        check = (forward->k * forward->k + forward->j * forward->j + forward->i * forward->i) -
            1.0f;
        if (_isnan((double)check) == 0 && halo::libm::fabs((double)check) < 0.001) {
            check = (up->k * up->k + up->j * up->j + up->i * up->i) - 1.0f;
            if (_isnan((double)check) == 0 && halo::libm::fabs((double)check) < 0.001) {
                check = up->j * forward->j + up->k * forward->k + up->i * forward->i;
                if (_isnan((double)check) == 0 && halo::libm::fabs((double)check) < 0.001) {
                    return;
                }
            }
        }

        {
            float right_i = up->j * forward->k - up->k * forward->j;
            float right_j = up->k * forward->i - forward->k * up->i;
            float right_k = forward->j * up->i - up->j * forward->i;

            up->i = right_k * forward->j - right_j * forward->k;
            up->j = right_i * forward->k - right_k * forward->i;
            up->k = right_j * forward->i - right_i * forward->j;
        }

        length = (float)halo::libm::sqrt((double)(forward->k * forward->k + forward->j * forward->j +
            forward->i * forward->i));
        if (0.0001 <= halo::libm::fabs((double)length)) {
            length = 1.0f / length;
            forward->i = length * forward->i;
            forward->j = length * forward->j;
            forward->k = length * forward->k;
        }

        length = (float)halo::libm::sqrt((double)(up->k * up->k + up->j * up->j + up->i * up->i));
        if (halo::libm::fabs((double)length) < 0.0001) {
            return;
        }
        length = 1.0f / length;
        up->i = length * up->i;
        up->j = length * up->j;
        up->k = length * up->k;
    }
}

/**
 *
 * Register convention in the original: local player index in AX (in_AX); no other parameters.
 *
 * @address 0x448010
 */
void ObserverHandle::evaluate_spline_velocity()
{
    int16_t local_player_index = (int16_t)player_handle;

    observer *o = &observers[local_player_index];
    float *remaining_offset = float_view(o->remaining_offset);
    float *coefficient_t5 = float_view(o->coefficient_t5);
    float *coefficient_t4 = float_view(o->coefficient_t4);
    float *coefficient_t3 = float_view(o->coefficient_t3);
    float *coefficient_t2 = float_view(o->coefficient_t2);
    float *coefficient_t1 = float_view(o->coefficient_t1);
    float *velocity = float_view(o->velocity);
    double inv_dt = 1.0 / (double)observer_dt; 
    int16_t channel;
    int32_t float_index = 0;

    for (channel = 0; channel < k_observer_parameter_count; channel++) {
        int16_t count = observer_derivative_float_counts[channel];
        float t = o->current_command.channel_times[channel] - observer_dt;
        int16_t i;

        if (t <= 0.0f) {
            uint8_t valid = (o->current_command.flags & _observer_command_valid_bit) != 0;

            if (!valid ||
                (o->current_command.interpolation_flags[channel] &
                 _observer_interpolation_exact_bit) == 0 &&
                (o->current_command.flags & _observer_command_snap_bit) == 0) {
                if (valid) {
                    for (i = 0; i < count; i++) {
                        velocity[float_index + i] = (float)-(inv_dt * remaining_offset[float_index + i]);
                    }
                }
            } else {
                for (i = 0; i < count; i++) {
                    velocity[float_index + i] = 0.0f;
                }
            }
        } else {
            float t2 = t * t;
            float t3 = t2 * t;
            float t4 = t3 * t;

            for (i = 0; i < count; i++) {
                int32_t idx = float_index + i;

                
                velocity[idx] = ((((t4 * coefficient_t5[idx] * 5.0f + t3 * coefficient_t4[idx] * 4.0f) +
                    t2 * coefficient_t3[idx] * 3.0f) + (t * coefficient_t2[idx] * 2.0f)) +
                    coefficient_t1[idx]);
            }
        }
        float_index += count;
    }
}

/**
 *
 * Register convention in the original: player index in CX (in_CX); no stack parameters.
 *
 * @address 0x4479a0
 */
observer_camera * ObserverHandle::get_camera()
{
    int16_t player_index = (int16_t)player_handle;

    if (player_index == -1) {
        return 0;
    }
    return &observers[player_index].camera;
}

/**
 *
 * Register convention in the original: none; cdecl, no arguments (tail call into observer_new
 * with EDX).
 *
 * @address 0x447870
 */
void ObserverSystem::initialize()
{
    halo::camera::observer_new(&observers[0]);
}

/**
 *
 * Register convention in the original: observer * in EDX (in_EDX); no stack parameters.
 *
 * @address 0x447740
 */
void ObserverSystem::construct(observer *self)
{
    self->parameters.forward = *(const Vector3D *)halo::math::globals().global_forward3d_pointer;
    self->parameters.up = *(const Vector3D *)halo::math::globals().global_up3d_pointer;
    self->parameters.field_of_view = 0.8726646f; 

    self->camera.position = *(const Point3D *)global_zero_vector3d_pointer;
    self->camera.leaf_index = -1;
    self->camera.cluster_index = -1;
    self->camera.velocity = *(const Vector3D *)global_origin3d_pointer;
    self->camera.forward = *(const Vector3D *)halo::math::globals().global_forward3d_pointer;
    self->camera.up = *(const Vector3D *)halo::math::globals().global_up3d_pointer;
    self->camera.field_of_view = 0.8726646f; 

    memset(&self->current_command, 0, sizeof(self->current_command));

    self->current_command.parameters.forward = self->parameters.forward;
    self->current_command.parameters.up = self->parameters.up;
    self->current_command.parameters.field_of_view = self->parameters.field_of_view;

    self->trailer_signature = k_observer_signature;
    self->header_signature = k_observer_signature;
    self->updated = 1;
    self->has_command = 0;
}

/**
 * Per-frame observer entry point: sets the observer's dt, applies its pending command (if
 * any), advances the easing spline, commits the result to observers_camera[0], and (when
 * add_bob is set, the local player is on foot in first person with no pending transition) adds
 * a one-tick movement-prediction bob to the published camera position.
 *
 * Register convention in the original: dt and add_bob are on the stack (Ghidra's recognized
 * param_1, param_2);.
 *
 * @address 0x447880
 */
void ObserverSystem::update(float dt, uint8_t add_bob)
{
    datum_index local_player;
    float time_fraction;
    real_vector3d position_delta;
    real_vector3d forward_delta;
    real_vector3d up_delta;

    observer_dt = dt;

    local_player = halo::game::globals().local_player_globals->local_players[0];
    if (local_player == (datum_index)k_datum_index_none) {
        return;
    }

    time_fraction = halo::game::globals().game_time->leftover_time;
    observers[0].updated = 1;
    halo::camera::observer_set_command(0);
    if (observer_dt != 0.0f) {
        halo::camera::observer_advance(0);
    }
    halo::camera::observer_commit(0);

    if (!add_bob) {
        return;
    }

    {
        player *p = &((player *)halo::game::globals().player_data->data)[halo::datum_slot(local_player)];
        if (p->unit != (datum_index)k_datum_index_none) {
            object *unit_object = halo::objects::object_try_and_get(p->unit, 3  );
            if (unit_object != 0 && unit_object->parent_object != (datum_index)k_datum_index_none) {
                return;
            }
        }
    }

    if (directors[0].pov_proc != halo::camera::camera_first_person_compute_pov ||
        !(directors[0].transition_time <= 0.0f)) {
        return;
    }

    if (halo::units::unit_predict_movement_delta(&position_delta, &forward_delta, &up_delta, time_fraction) !=
        0) {
        observers[0].camera.position.x += position_delta.i;
        observers[0].camera.position.y += position_delta.j;
        observers[0].camera.position.z += position_delta.k;
    }
}

/**
 *
 * Register convention in the original: none; cdecl, no arguments.
 *
 * @address 0x447a60
 */
void ObserverSystem::update_location()
{
    int32_t leaf_index;

    if (halo::game::globals().local_player_globals->local_players[0] == k_datum_index_none) {
        return;
    }
    leaf_index = (int32_t)halo::physics::bsp3d_node_find_leaf(0, halo::physics::globals().collision_bsp,
        (real_point3d *)&observers[0].camera.position);
    observers[0].camera.leaf_index = leaf_index;
    if (leaf_index == -1) {
        observers[0].camera.cluster_index = -1;
    } else {
        observers[0].camera.cluster_index = (int16_t)((ScenarioStructureBSPLeaf *)
            halo::scenario::globals().structure_bsp->leaves.pointer)[leaf_index & halo::k_leaf_index_mask].cluster;
    }
}

/**
 *
 * Register convention in the original: forward in EAX (in_EAX); position, up, distance and
 * radius_scale on the.
 *
 * @address 0x448d40
 */
void ObserverSystem::avoid_collision(real_vector3d *forward, real_point3d *position, real_vector3d *up, float *distance, float radius_scale)
{
    bsp_leaf_reference location;
    uint8_t use_alternate_mask;
    float probe_length;
    real_point3d pullback_point;
    float unobstructed_fraction; 
    float best_fraction;         
    float offsets[6];            
    int32_t winning_group;       
    float winning_sign;          
    int32_t k;
    collision_result collision;

    unobstructed_fraction = 1.0f;
    location.leaf_index = (int32_t)halo::physics::bsp3d_node_find_leaf(0, halo::physics::globals().collision_bsp, position);
    if (location.leaf_index == -1) {
        location.cluster_index = -1;
    } else {
        location.cluster_index = (int16_t)((ScenarioStructureBSPLeaf *)halo::scenario::globals().structure_bsp->leaves.pointer)
            [location.leaf_index & halo::k_leaf_index_mask].cluster;
    }
    use_alternate_mask = halo::scenario::scenario_location_get_water_and_weather(position, &location, 0); 

    probe_length = radius_scale + *distance;
    pullback_point.x = position->x - probe_length * forward->i;
    pullback_point.y = position->y - probe_length * forward->j;
    pullback_point.z = position->z - probe_length * forward->k;

    halo::camera::observer_collision_test_ray(position, use_alternate_mask, &pullback_point,
        &unobstructed_fraction);

    {
        float scale = *distance * 0.174f;

        offsets[0] = up->i * scale;
        offsets[1] = up->j * scale;
        offsets[2] = up->k * scale;
        offsets[3] = (up->j * forward->k - forward->j * up->k) * scale;
        offsets[4] = (forward->i * up->k - up->i * forward->k) * scale;
        offsets[5] = (forward->j * up->i - up->j * forward->i) * scale;
    }

    best_fraction = unobstructed_fraction;
    winning_group = -1;
    winning_sign = 0.0f;

    for (k = 0; k < 4; k++) {
        float sign = (k & 2) ? 1.0f : -1.0f;
        int32_t group = k & 1;
        float *offset = &offsets[group * 3];
        real_point3d probe;
        uint32_t mask;
        uint8_t hit;
        float hit_fraction;

        probe.x = sign * offset[0] + pullback_point.x;
        probe.y = sign * offset[1] + pullback_point.y;
        probe.z = sign * offset[2] + pullback_point.z;

        mask = to_bits(use_alternate_mask ? k_probe_flags_alternate : k_probe_flags_normal);
        {
            real_vector3d delta;

            delta.i = probe.x - position->x;
            delta.j = probe.y - position->y;
            delta.k = probe.z - position->z;
            hit = halo::physics::collision_test_movement_segment(mask, position, &delta, k_datum_index_none, &collision);
        }
        if (hit) {
            hit_fraction = collision.t;
            if (hit_fraction < best_fraction) {
                best_fraction = hit_fraction;
                winning_group = group;
                winning_sign = sign;
            }
        }
    }

    if (winning_group == -1) {
        *distance = unobstructed_fraction * *distance;
        return;
    }

    {
        float *offset = &offsets[winning_group * 3];
        float clear_t = 0.0f;                       
        float blocked_t = winning_sign;              
        float last_clear_result = unobstructed_fraction; 
        float last_blocked_fraction = best_fraction; 
        int32_t iterations_left = 10;
        uint8_t converged_this_time;
        float mid;
        float selector;

        do {
            real_point3d probe;
            real_vector3d delta;
            uint32_t mask;
            uint8_t hit;
            float hit_fraction;

            converged_this_time = 0;
            mid = (blocked_t + clear_t) * 0.5f;

            probe.x = mid * offset[0] + pullback_point.x;
            probe.y = mid * offset[1] + pullback_point.y;
            probe.z = mid * offset[2] + pullback_point.z;
            mask = to_bits(use_alternate_mask ? k_probe_flags_alternate : k_probe_flags_normal);
            delta.i = probe.x - position->x;
            delta.j = probe.y - position->y;
            delta.k = probe.z - position->z;
            hit = halo::physics::collision_test_movement_segment(mask, position, &delta, k_datum_index_none, &collision);

            bool mark_clear = !hit;

            if (hit) {
                hit_fraction = collision.t;
                converged_this_time = 1;
                mark_clear = 0.1 <= halo::libm::fabs((double)(hit_fraction - last_blocked_fraction));
                if (!mark_clear) {
                    last_blocked_fraction = hit_fraction;
                    blocked_t = mid;
                }
            }
            if (mark_clear) {
                clear_t = mid;
                last_clear_result = converged_this_time ? hit_fraction : 1.0f;
            }
            iterations_left--;
        } while (iterations_left != 0);

        selector = clear_t;
        if (last_blocked_fraction <= last_clear_result) {
            selector = (0.0f <= blocked_t) ? 1.0f : 0.0f;
        }
        if (selector == 0.0f) {
            if (last_clear_result < last_blocked_fraction) {
                blocked_t = clear_t;
            }
            blocked_t = -blocked_t;
        } else if (last_clear_result < last_blocked_fraction) {
            blocked_t = clear_t;
        }

        *distance = (blocked_t * unobstructed_fraction + (1.0f - blocked_t) * best_fraction) *
            *distance;
    }
}

/**
 *
 * @address 0x449170
 */
uint8_t ObserverSystem::collision_test_ray(real_point3d *origin, uint8_t use_alternate_mask, real_point3d *target, float *out_fraction)
{
    uint32_t flags = to_bits(use_alternate_mask ? k_probe_flags_alternate : k_probe_flags_normal);
    real_vector3d delta;
    collision_result collision;

    delta.i = target->x - origin->x;
    delta.j = target->y - origin->y;
    delta.k = target->z - origin->z;

    if (halo::physics::collision_test_movement_segment(flags, origin, &delta, k_datum_index_none, &collision) != 0) {
        *out_fraction = collision.t;
        return 1;
    }
    return 0;
}

/**
 *
 * Register convention in the original: target in EAX (in_EAX), current in ECX (in_ECX), output
 * in EDX (in_EDX);.
 *
 * @address 0x448710
 */
void ObserverSystem::compute_remaining_offset(float *target, float *current, float *out)
{
    real_matrix4x3 current_matrix, target_matrix, target_matrix_inverse, relative_matrix;
    real_quaternion relative_rotation;
    real_vector3d axis;
    float axis_length;
    float angle;
    int32_t i;

    for (i = 0; i < 8; i++) {
        out[i] = target[i] - current[i];
    }
    target += 8;   
    current += 8;  

    halo::math::matrix4x3_from_forward_up(*(real_vector3d *)(current + 3), *(real_vector3d *)current,
        current_matrix);
    halo::math::matrix4x3_from_forward_up(*(real_vector3d *)(target + 3), *(real_vector3d *)target,
        target_matrix);
    halo::math::matrix4x3_inverse(&target_matrix_inverse, target_matrix);
    matrix4x3_multiply_ptr(&current_matrix, &target_matrix_inverse, &relative_matrix);
    halo::math::quaternion_from_matrix4x3(&relative_matrix, relative_rotation);

    axis.i = relative_rotation.i;
    axis.j = relative_rotation.j;
    axis.k = relative_rotation.k;

    axis_length = (float)halo::libm::sqrt((double)(axis.j * axis.j + axis.k * axis.k + axis.i * axis.i));
    if (halo::libm::fabs((double)axis_length) < 9.999999747378752e-05) {
        axis_length = 0.0f;
    } else {
        float inv_length = 1.0f / axis_length;
        axis.i = inv_length * axis.i;
        axis.j = inv_length * axis.j;
        axis.k = inv_length * axis.k;
    }

    angle = (float)halo::libm::atan2((double)axis_length, (double)relative_rotation.w);
    angle = angle + angle;
    if (3.1415927f < angle) {
        axis.i = -axis.i;
        axis.j = -axis.j;
        axis.k = -axis.k;
        angle = 6.2831855f - angle;
    }

    out[8] = angle * axis.i;
    out[9] = angle * axis.j;
    out[10] = angle * axis.k;
}

}

namespace halo::camera {

void observer_advance(int16_t local_player_index)
{
    halo::camera::ObserverHandle(local_player_index).advance();
}

void observer_commit(int16_t local_player_index)
{
    halo::camera::ObserverHandle(local_player_index).commit();
}

void observer_set_command(int16_t local_player_index)
{
    halo::camera::ObserverHandle(local_player_index).set_command();
}

void observer_compute_spline_coefficients(int16_t local_player_index)
{
    halo::camera::ObserverHandle(local_player_index).compute_spline_coefficients();
}

void observer_evaluate_spline_acceleration(int16_t local_player_index)
{
    halo::camera::ObserverHandle(local_player_index).evaluate_spline_acceleration();
}

void observer_evaluate_spline_value_and_orthonormalize(int16_t local_player_index)
{
    halo::camera::ObserverHandle(local_player_index).evaluate_spline_value_and_orthonormalize();
}

void observer_evaluate_spline_velocity(int16_t local_player_index)
{
    halo::camera::ObserverHandle(local_player_index).evaluate_spline_velocity();
}

observer_camera *observer_get_camera(int16_t player_index)
{
    return halo::camera::ObserverHandle(player_index).get_camera();
}

void observer_initialize(void)
{
    halo::camera::ObserverSystem::initialize();
}

void observer_new(observer *self)
{
    halo::camera::ObserverSystem::construct(self);
}

void observer_update(float dt, uint8_t add_bob)
{
    halo::camera::ObserverSystem::update(dt, add_bob);
}

void observer_update_location(void)
{
    halo::camera::ObserverSystem::update_location();
}

void observer_avoid_collision(real_vector3d *forward, real_point3d *position, real_vector3d *up, float *distance, float radius_scale)
{
    halo::camera::ObserverSystem::avoid_collision(forward, position, up, distance, radius_scale);
}

uint8_t observer_collision_test_ray(real_point3d *origin, uint8_t use_alternate_mask, real_point3d *target, float *out_fraction)
{
    return halo::camera::ObserverSystem::collision_test_ray(origin, use_alternate_mask, target, out_fraction);
}

void observer_compute_remaining_offset(float *target, float *current, float *out)
{
    halo::camera::ObserverSystem::compute_remaining_offset(target, current, out);
}

}
