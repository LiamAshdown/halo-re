/**
 * @file include/halo/math/quaternion.hpp
 * Real_quaternion: normalize, multiply, lerp, rotate, matrix conversions.
 * The C symbols other modules link against are the wrappers in src/math/math_c_api.cpp.
 */
#pragma once

#include "halo/math/math_types.hpp"

namespace halo::math {

/**
 * Converts a quaternion to the rotation of a matrix4x3 (scale 1, no translation); a zero quaternion gives the
 * zero rotation.
 *
 * Original register convention: ECX -> q, EDX -> out.
 * @address 0x004cbad0
 */
void matrix4x3_from_quaternion(const real_quaternion &q, real_matrix4x3 &out);

/**
 * Extracts the rotation quaternion of a matrix4x3 (largest-diagonal method).
 *
 * Original register convention: ECX -> m, stack -> out.
 * @address 0x004cbc00
 */
void quaternion_from_matrix4x3(const real_matrix4x3 *m, real_quaternion &out);

/**
 * Extracts the rotation quaternion of a 3x3 matrix (largest-diagonal method). Returns `out`.
 *
 * Original register convention: ECX -> m, stack -> out.
 * @address 0x004cc780
 */
real_quaternion * quaternion_from_matrix3x3(real_matrix3x3 *m, real_quaternion *out);

/**
 * Normalises a quaternion in place; a zero quaternion becomes the identity.
 *
 * Original register convention: ECX -> q.
 * @address 0x004cdb20
 */
void quaternion_normalize(real_quaternion &q);

/**
 * Converts a quaternion to a unit axis and an angle in radians.
 *
 * Original register convention: EAX -> quat, ESI -> axis_out, EDI -> angle_out.
 * @address 0x004cdb90
 */
void quaternion_to_axis_angle(const real_quaternion &quat, real_vector3d *axis_out, real &angle_out);

/**
 * out = b * a (Hamilton product).
 *
 * Original register convention: EAX -> a, ECX -> b, EDX -> out.
 * @address 0x004cdbf0
 */
void quaternion_multiply(const real_quaternion *a, const real_quaternion *b, real_quaternion *out);

/**
 * Componentwise lerp from `b` (t = 0) to `a` (t = 1), negating the weight of `a` when dot(a, b) < 0 so it takes
 * the shorter path. The result is not normalised.
 *
 * Original register convention: ECX -> a, EDX -> b, ESI -> out, stack -> t.
 * @address 0x004cdcc0
 */
void quaternion_lerp(const real_quaternion &a, const real_quaternion &b, real_quaternion &out, real t);

/**
 * Rotates `v` by the quaternion `q`.
 *
 * Original register convention: ECX -> v, EDX -> out, stack -> q.
 * @address 0x004cdd40
 */
void quaternion_rotate_vector(const real_quaternion &q, const real_vector3d &v, real_vector3d &out);

}  // namespace halo::math
