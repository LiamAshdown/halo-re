/**
 * @file include/halo/math/rotation.hpp
 * Rotating vectors: axis-angle rotation and the bounded angular servos.
 * Declared for other modules through halo/math/api.hpp.
 */
#pragma once

#include "halo/math/math_types.hpp"

namespace halo::math {

/**
 * Rotates `v` in place about an axis perpendicular to it by the angle with the given sine and cosine.
 *
 * Original register convention: EAX -> v, ECX -> axis, stack -> (sin_angle, cos_angle).
 * @address 0x004cd700
 */
void vector3d_rotate_about_axis_perpendicular(real_vector3d &v, const real_vector3d &axis, real sin_angle, real cos_angle);

/**
 * Rotates two orthogonal vectors together in their own plane by the angle with the given sine and cosine.
 *
 * Original register convention: EAX -> a, ECX -> b, stack -> (sin_angle, cos_angle).
 * @address 0x004cd790
 */
void vector3d_rotate_pair_in_plane(real_vector3d &a, real_vector3d &b, real sin_angle, real cos_angle);

/**
 * Rotates `v` in place about a unit axis by the angle with the given sine and cosine.
 *
 * Original register convention: EAX -> v, ECX -> axis, stack -> (sin_angle, cos_angle).
 * @address 0x004cd820
 */
void vector3d_rotate_about_axis(real_vector3d &v, const real_vector3d &axis, real sin_angle, real cos_angle);

/**
 * Rotates `source` toward `target` by the given angle into `out` and returns 1; when `source` is already within
 * that angle, copies `target` and returns 0.
 *
 * Original register convention: ECX -> target, ESI -> source, EDI -> out, stack -> (sin_angle, cos_angle).
 * @address 0x004cd950
 */
uint8_t vector3d_rotate_toward(real_vector3d *target, const real_vector3d &source, real_vector3d *out, real sin_angle, real cos_angle);

/**
 * Angular servo: turns `direction` toward `target_direction`, carrying `angular_velocity` between calls, with
 * maximum speed and acceleration per tick.
 *
 * Original register convention: ESI -> direction, EDI -> target_direction,.
 * @address 0x004cf530
 */
void vector3d_rotate_toward_with_acceleration(real_vector3d *direction, const real_vector3d &target_direction, real_vector3d &angular_velocity, real maximum_velocity, real acceleration);

/**
 * Bounded angular servo: turns `current` toward `target` within the azimuth/elevation limits in bounds[4],
 * carrying `velocity` between calls with bounded speed and acceleration. A non-NULL `transform` gives the local
 * space the limits apply in.
 *
 * Original register convention: stack -> (current, velocity, bounds, max_velocity, max_acceleration),.
 * @address 0x00564ae0
 */
void vector3d_rotate_toward_bounded(real_vector3d *current, real_vector3d *velocity, real *bounds, real max_velocity, real max_acceleration, const real_vector3d &target, real_matrix4x3 *transform);

}  // namespace halo::math
