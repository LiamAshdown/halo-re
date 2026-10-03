/**
 * @file src/math/rotation.cpp
 * Rotating vectors: axis-angle rotation and the bounded angular servos.
 */

#include "halo/core/crt.hpp"
#include "halo/math/math.hpp"
#include "halo/math/globals.hpp"

#include "tags.h"

namespace halo::math {

void vector3d_rotate_about_axis_perpendicular(real_vector3d &v, const real_vector3d &axis, real sin_angle, real cos_angle)
{
    real old_i;
    real old_j;
    real old_k;

    old_i = v.i;
    old_j = v.j;
    old_k = v.k;

    v.i = (axis.j * old_k - old_j * axis.k) * sin_angle + cos_angle * old_i;
    v.j = (old_i * axis.k - axis.i * old_k) * sin_angle + cos_angle * old_j;
    v.k = cos_angle * old_k + (old_j * axis.i - old_i * axis.j) * sin_angle;
}

void vector3d_rotate_pair_in_plane(real_vector3d &a, real_vector3d &b, real sin_angle, real cos_angle)
{
    real old_b_i;
    real old_b_j;
    real old_b_k;

    old_b_i = b.i;
    old_b_j = b.j;
    old_b_k = b.k;

    b.i = sin_angle * a.i + cos_angle * b.i;
    b.j = sin_angle * a.j + cos_angle * b.j;
    b.k = sin_angle * a.k + cos_angle * b.k;

    a.i = -old_b_i * sin_angle + cos_angle * a.i;
    a.j = cos_angle * a.j + -old_b_j * sin_angle;
    a.k = -old_b_k * sin_angle + cos_angle * a.k;
}

void vector3d_rotate_about_axis(real_vector3d &v, const real_vector3d &axis, real sin_angle, real cos_angle)
{
    real parallel_term;
    real orig_i;
    real orig_j;

    parallel_term = (1.0f - cos_angle) * (v.i * axis.i + axis.k * v.k + v.j * axis.j);
    orig_i = v.i;
    orig_j = v.j;

    v.i = (parallel_term * axis.i + cos_angle * orig_i) - (orig_j * axis.k - v.k * axis.j) * sin_angle;
    v.j = (parallel_term * axis.j + cos_angle * orig_j) - (axis.i * v.k - orig_i * axis.k) * sin_angle;
    v.k = (cos_angle * v.k + parallel_term * axis.k) - (orig_i * axis.j - orig_j * axis.i) * sin_angle;
}

uint8_t vector3d_rotate_toward(real_vector3d *target, const real_vector3d &source, real_vector3d *out, real sin_angle, real cos_angle)
{
    real_vector3d axis;
    real length;

    if (cos_angle <= source.k * target->k + target->i * source.i + target->j * source.j) {
        *out = *target;
        return 0;
    }

    axis.i = target->k * source.j - target->j * source.k;
    axis.j = target->i * source.k - source.i * target->k;
    axis.k = target->j * source.i - target->i * source.j;

    length = vector3d_normalize_with_length(axis);
    if (length == 0.0f) {
        vector3d_build_perpendicular(axis, *target);
        vector3d_normalize_with_length(axis);
    }

    *out = source;
    vector3d_rotate_about_axis(*out, axis, sin_angle, cos_angle);
    return 1;
}

void vector3d_rotate_toward_with_acceleration(real_vector3d *direction, const real_vector3d &target_direction, real_vector3d &angular_velocity, real maximum_velocity, real acceleration)
{
    real dot;
    real braking_speed;
    real target_speed;
    real_vector3d goal;
    real dx, dy, dz;
    real distance_squared;
    real_vector3d axis;
    real speed;

    if (acceleration <= 0.0f && maximum_velocity <= 0.0f) {
        angular_velocity.i = globals().global_origin3d.x;
        angular_velocity.j = globals().global_origin3d.y;
        angular_velocity.k = globals().global_origin3d.z;
        *direction = target_direction;
        return;
    }

    dot = target_direction.k * direction->k + direction->i * target_direction.i +
          target_direction.j * direction->j;
    if (dot < -1.0f) {
        dot = -1.0f;
    } else if (1.0f < dot) {
        dot = 1.0f;
    }

    braking_speed = (real)acos((double)dot) * acceleration;
    braking_speed = braking_speed + braking_speed;
    if (braking_speed < maximum_velocity * maximum_velocity) {
        target_speed = (real)sqrt((double)braking_speed);
    } else {
        target_speed = maximum_velocity;
    }

    goal.i = direction->j * target_direction.k - direction->k * target_direction.j;
    goal.j = direction->k * target_direction.i - direction->i * target_direction.k;
    goal.k = direction->i * target_direction.j - direction->j * target_direction.i;
    vector3d_normalize_with_length(goal);
    goal.i = goal.i * target_speed;
    goal.j = goal.j * target_speed;
    goal.k = goal.k * target_speed;

    dx = goal.i - angular_velocity.i;
    dy = goal.j - angular_velocity.j;
    dz = goal.k - angular_velocity.k;
    distance_squared = dx * dx + dy * dy + dz * dz;

    if (acceleration * acceleration <= distance_squared) {
        real step = acceleration / (real)sqrt((double)distance_squared);
        angular_velocity.i = dx * step + angular_velocity.i;
        angular_velocity.j = dy * step + angular_velocity.j;
        angular_velocity.k = dz * step + angular_velocity.k;
    } else if (target_speed < 1.0000001e-06f) {
        angular_velocity.i = globals().global_origin3d.x;
        angular_velocity.j = globals().global_origin3d.y;
        angular_velocity.k = globals().global_origin3d.z;
        *direction = target_direction;
        return;
    } else {
        angular_velocity = goal;
    }

    axis = angular_velocity;
    speed = vector3d_normalize_with_length(axis);
    if (speed == 0.0f) {
        return;
    }
    vector3d_rotate_about_axis(*direction, axis, (real)sin((double)speed), (real)cos((double)speed));
    vector3d_normalize_with_length(*direction);
}

constexpr real k_two_pi = 6.2831855f;
constexpr real k_pi = 3.1415927f;

void vector3d_rotate_toward_bounded(real_vector3d *current, real_vector3d *velocity, real *bounds, real max_velocity, real max_acceleration, const real_vector3d &target, real_matrix4x3 *transform)
{
    real current_x, current_y, current_z;
    real target_x, target_y, target_z;
    real current_azimuth, current_elevation;
    real target_azimuth, target_elevation;
    uint8_t full_circle;
    uint8_t target_in_range = 1;
    real clamped_x, clamped_y, clamped_z;
    real_vector3d axis;
    real axis_length;
    real predicted_azimuth_delta, predicted_elevation_delta;
    real azimuth_error, elevation_error;
    bounded_ramp_profile azimuth_profile;
    bounded_ramp_profile elevation_profile;
    uint8_t azimuth_done, elevation_done;

    if (transform != 0) {
        current_x = current->i * transform->forward.i + current->j * transform->forward.j + current->k * transform->forward.k;
        current_y = current->i * transform->left.i + current->j * transform->left.j + current->k * transform->left.k;
        current_z = current->i * transform->up.i + current->j * transform->up.j + current->k * transform->up.k;
        target_x = target.i * transform->forward.i + target.j * transform->forward.j + target.k * transform->forward.k;
        target_y = target.i * transform->left.i + target.j * transform->left.j + target.k * transform->left.k;
        target_z = target.i * transform->up.i + target.j * transform->up.j + target.k * transform->up.k;
    } else {
        current_x = current->i;
        current_y = current->j;
        current_z = current->k;
        target_x = target.i;
        target_y = target.j;
        target_z = target.k;
    }

    full_circle = (uint8_t)(-0.0001f < (bounds[1] - bounds[0]) - k_two_pi);

    current_azimuth = (real)atan2((double)current_y, (double)current_x);
    current_elevation = (real)atan2((double)current_z, (double)sqrt((double)(current_x * current_x + current_y * current_y)));
    target_azimuth = (real)atan2((double)target_y, (double)target_x);
    target_elevation = (real)atan2((double)target_z, (double)sqrt((double)(target_x * target_x + target_y * target_y)));

    if (full_circle) {
        if (target_azimuth < bounds[0]) {
            target_azimuth = target_azimuth + k_two_pi;
        } else if (target_azimuth > bounds[1]) {
            target_azimuth = target_azimuth - k_two_pi;
        }
    } else if (target_azimuth < bounds[0]) {
        target_azimuth = bounds[0];
        target_in_range = 0;
    } else if (target_azimuth > bounds[1]) {
        target_azimuth = bounds[1];
        target_in_range = 0;
    }

    if (target_elevation < bounds[2]) {
        target_elevation = bounds[2];
    } else if (target_elevation > bounds[3]) {
        target_elevation = bounds[3];
    } else if (target_in_range) {
        clamped_x = target.i;
        clamped_y = target.j;
        clamped_z = target.k;
        goto velocity_prediction;
    }

    clamped_x = (real)cos((double)target_elevation) * (real)cos((double)target_azimuth);
    clamped_y = (real)cos((double)target_elevation) * (real)sin((double)target_azimuth);
    clamped_z = (real)sin((double)target_elevation);
    if (transform != 0) {
        real_vector3d clamped;
        clamped.i = clamped_x; clamped.j = clamped_y; clamped.k = clamped_z;
        matrix4x3_transform_normal(clamped, clamped, *transform);
        vector3d_normalize_with_length(clamped);
        clamped_x = clamped.i; clamped_y = clamped.j; clamped_z = clamped.k;
    }

velocity_prediction:
    axis.i = velocity->i;
    axis.j = velocity->j;
    axis.k = velocity->k;
    axis_length = vector3d_normalize_with_length(axis);
    if (axis_length == 0.0f) {
        predicted_azimuth_delta = 0.0f;
        predicted_elevation_delta = 0.0f;
    } else {
        real_vector3d rotated;
        real new_azimuth, new_elevation;
        rotated.i = current_x; rotated.j = current_y; rotated.k = current_z;
        vector3d_rotate_about_axis(rotated, axis, (real)sin((double)axis_length), (real)cos((double)axis_length));
        new_azimuth = (real)atan2((double)rotated.j, (double)rotated.i);
        new_elevation = (real)atan2((double)rotated.k, (double)sqrt((double)(rotated.i * rotated.i + rotated.j * rotated.j)));
        predicted_azimuth_delta = new_azimuth - current_azimuth;
        predicted_elevation_delta = new_elevation - current_elevation;
    }

    azimuth_error = current_azimuth - target_azimuth;
    elevation_error = current_elevation - target_elevation;
    if (full_circle) {
        if (azimuth_error > k_pi) {
            azimuth_error = azimuth_error - k_two_pi;
        } else if (azimuth_error < -k_pi) {
            azimuth_error = azimuth_error + k_two_pi;
        }
    }

    bounded_ramp_profile_build(azimuth_error, predicted_azimuth_delta, max_velocity, max_acceleration, &azimuth_profile);
    bounded_ramp_profile_build(elevation_error, predicted_elevation_delta, max_velocity, max_acceleration, &elevation_profile);
    bounded_ramp_profile_synchronize(&azimuth_profile, &elevation_profile, max_acceleration);
    azimuth_done = bounded_ramp_profile_evaluate(azimuth_profile, 1.0f, azimuth_error, azimuth_error,
                                                  predicted_azimuth_delta, predicted_azimuth_delta);
    elevation_done = bounded_ramp_profile_evaluate(elevation_profile, 1.0f, elevation_error, elevation_error,
                                                    predicted_elevation_delta, predicted_elevation_delta);

    if (azimuth_done && elevation_done) {
        current->i = clamped_x;
        current->j = clamped_y;
        current->k = clamped_z;
        velocity->i = globals().global_origin3d.x;
        velocity->j = globals().global_origin3d.y;
        velocity->k = globals().global_origin3d.z;
        return;
    }

    {
        real new_azimuth = azimuth_error + target_azimuth;
        real new_elevation = elevation_error + target_elevation;
        real dir_x, dir_y, dir_z;
        real ref_x, ref_y, ref_z;
        real dot, cross_x, cross_y, cross_z;

        if (full_circle) {
            if (new_azimuth < bounds[0]) {
                new_azimuth = new_azimuth + k_two_pi;
            } else if (new_azimuth > bounds[1]) {
                new_azimuth = new_azimuth - k_two_pi;
            }
        } else if (new_azimuth < bounds[0]) {
            new_azimuth = bounds[0];
        } else if (new_azimuth > bounds[1]) {
            new_azimuth = bounds[1];
        }

        if (new_elevation < bounds[2]) {
            new_elevation = bounds[2];
        } else if (new_elevation > bounds[3]) {
            new_elevation = bounds[3];
        }

        dir_x = (real)cos((double)new_elevation) * (real)cos((double)new_azimuth);
        dir_y = (real)cos((double)new_elevation) * (real)sin((double)new_azimuth);
        dir_z = (real)sin((double)new_elevation);
        ref_x = (real)cos((double)(new_elevation + predicted_elevation_delta)) * (real)cos((double)(new_azimuth + predicted_azimuth_delta));
        ref_y = (real)cos((double)(new_elevation + predicted_elevation_delta)) * (real)sin((double)(new_azimuth + predicted_azimuth_delta));
        ref_z = (real)sin((double)(new_elevation + predicted_elevation_delta));

        dot = dir_z * ref_z + ref_y * dir_y + ref_x * dir_x;
        if (dot < -1.0f) {
            dot = -1.0f;
        } else if (1.0f < dot) {
            dot = 1.0f;
        }

        cross_x = dir_y * ref_z - ref_y * dir_z;
        cross_y = dir_z * ref_x - ref_z * dir_x;
        cross_z = ref_y * dir_x - dir_y * ref_x;

        velocity->i = cross_x; velocity->j = cross_y; velocity->k = cross_z;
        axis_length = vector3d_normalize_with_length(*velocity);
        {
            real speed = (real)acos((double)dot);
            if (max_velocity < speed) {
                speed = max_velocity;
            }
            velocity->i = velocity->i * speed;
            velocity->j = velocity->j * speed;
            velocity->k = velocity->k * speed;
        }
        if (transform != 0) {
            real_vector3d direction;
            direction.i = dir_x; direction.j = dir_y; direction.k = dir_z;
            matrix4x3_transform_normal(*current, direction, *transform);
            vector3d_normalize_with_length(*current);
            return;
        }
        current->i = dir_x;
        current->j = dir_y;
        current->k = dir_z;
    }
}

}  // namespace halo::math
