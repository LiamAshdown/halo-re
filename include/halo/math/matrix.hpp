/**
 * @file include/halo/math/matrix.hpp
 * Real_matrix3x3 / real_matrix4x3: build, invert, multiply (scalar, sse, 3dnow!), transform.
 * Declared for other modules through halo/math/api.hpp.
 */
#pragma once

#include "halo/math/math_types.hpp"

namespace halo::math {

/**
 * Inverts a uniform-scale rigid transform; `out` may alias `in`. A matrix with scale 0 inverts to all zeros.
 *
 * Original register convention: EAX -> out, ECX -> in.
 * @address 0x004cb7a0
 */
void matrix4x3_inverse(real_matrix4x3 *out, const real_matrix4x3 &in);

/**
 * Builds a rotation matrix4x3 (scale 1, no translation) from a unit axis and the sine and cosine of the angle
 * (Rodrigues).
 *
 * Original register convention: EAX -> out, ECX -> axis, stack -> (sin_angle, cos_angle).
 * @address 0x004cb880
 */
void matrix4x3_from_axis_angle(real_matrix4x3 &out, const real_vector3d &axis, real sin_angle, real cos_angle);

/**
 * Builds a matrix4x3 with scale 1, no translation, the given forward and up rows and left = cross(up, forward).
 *
 * Original register convention: EAX -> up, ECX -> forward, stack -> out.
 * @address 0x004cb970
 */
void matrix4x3_from_forward_up(const real_vector3d &up, const real_vector3d &forward, real_matrix4x3 &out);

/**
 * Builds a rotation matrix4x3 (scale 1, no translation) from yaw, pitch and roll.
 *
 * Original register convention: EAX -> out, stack -> (yaw, pitch, roll).
 * @address 0x004cba10
 */
void matrix4x3_from_euler_angles(real_matrix4x3 &out, real yaw, real pitch, real roll);

/**
 * matrix4x3_from_forward_up, then sets the translation to `position`.
 *
 * Original register convention: EAX -> up, ECX -> forward, ESI -> position, stack -> out.
 * @address 0x004cbd60
 */
void matrix4x3_from_forward_up_position(const real_vector3d *up, const real_vector3d *forward, const real_point3d &position, real_matrix4x3 *out);

/**
 * Copies the forward row, up row and position out of a matrix4x3.
 *
 * Original register convention: EAX -> out_up, ECX -> out_forward, stack -> (m, out_position).
 * @address 0x004cbd90
 */
void matrix4x3_extract_forward_up_position(real_vector3d &out_up, real_vector3d &out_forward, const real_matrix4x3 &m, real_point3d &out_position);

/**
 * Transforms a point: scale (skipped when exactly 1), rotate, translate.
 *
 * Original register convention: EAX -> out, EDX -> point, stack -> m.
 * @address 0x004cbde0
 */
void matrix4x3_transform_point(real_point3d &out, const real_point3d &point, const real_matrix4x3 &m);

/**
 * Transforms a vector by scale (skipped when exactly 1) and rotation, without translation.
 *
 * Original register convention: EAX -> out, EDX -> v, stack -> m.
 * @address 0x004cbe50
 */
void matrix4x3_transform_vector(real_vector3d &out, const real_vector3d &v, const real_matrix4x3 &m);

/**
 * Rotates a direction by the matrix (no scale, no translation).
 *
 * Original register convention: EAX -> out, EDX -> normal, stack -> m.
 * @address 0x004cbec0
 */
void matrix4x3_transform_normal(real_vector3d &out, const real_vector3d &normal, const real_matrix4x3 &m);

/**
 * Transforms a plane: rotates the normal and moves the distance by the scaled plane distance and the
 * translation.
 *
 * Original register convention: EAX -> out, ECX -> m, EDX -> plane.
 * @address 0x004cbf10
 */
void matrix4x3_transform_plane(real_plane3d &out, const real_matrix4x3 &m, const real_plane3d &plane);

/**
 * Transforms a world point into the matrix's local space without building the inverse; a matrix with scale 0
 * gives the origin.
 *
 * Original register convention: ECX -> m, EDX -> out, ESI -> point.
 * @address 0x004cbf80
 */
void matrix4x3_inverse_transform_point(const real_matrix4x3 &m, real_point3d &out, const real_point3d &point);

/**
 * Transforms a vector into the matrix's local space: divides out the scale, then applies the transposed
 * rotation.
 *
 * Original register convention: EAX -> out, EDX -> v, stack -> m.
 * @address 0x004cc010
 */
void matrix4x3_inverse_transform_vector(real_vector3d &out, const real_vector3d &v, const real_matrix4x3 &m);

/**
 * Rotates a direction into the matrix's local space (transposed rotation only, no scale or translation).
 *
 * Original register convention: EAX -> out, EDX -> normal, stack -> m.
 * @address 0x004cc080
 */
void matrix4x3_inverse_transform_normal(real_vector3d &out, const real_vector3d &normal, const real_matrix4x3 &m);

/**
 * out = a * b (scalar build, the default matrix4x3_multiply_procedure); `out` may alias either operand.
 * @address 0x004cc0d0
 */
void matrix4x3_multiply(const real_matrix4x3 *a, const real_matrix4x3 *b, real_matrix4x3 *out);

/**
 * The SSE build of matrix4x3_multiply; computes the scalar product (see matrix4x3_multiply).
 *
 * Original register convention: stack -> a, b, out (cdecl).
 * @address 0x004cc250
 */
void matrix4x3_multiply_sse(const real_matrix4x3 *a, const real_matrix4x3 *b, real_matrix4x3 *out);

/**
 * The 3DNow! build of matrix4x3_multiply, as scalar code with the packed routine's per-component summation
 * order; `out` may alias either operand.
 * @address 0x004cc3a0
 */
void matrix4x3_multiply_3dnow(const real_matrix4x3 *a, const real_matrix4x3 *b, real_matrix4x3 *out);

/**
 * Transposes a 3x3 matrix; `out` may alias `in`.
 *
 * Original register convention: EAX -> out, ECX -> in.
 * @address 0x004cc500
 */
void matrix3x3_transpose(real_matrix3x3 *out, const real_matrix3x3 *in);

/**
 * Builds a 3x3 basis: forward and up as given, left = cross(up, forward).
 *
 * Original register convention: ECX -> up, EDX -> forward, stack -> out.
 * @address 0x004cc560
 */
void matrix3x3_from_forward_up(const real_vector3d &up, const real_vector3d &forward, real_matrix3x3 &out);

/**
 * out = a * b for 3x3 rotation matrices; `out` may alias either operand.
 *
 * Original register convention: EAX -> out, EDX -> a, stack -> b.
 * @address 0x004cc5f0
 */
void matrix3x3_multiply(real_matrix3x3 *out, const real_matrix3x3 *a, const real_matrix3x3 *b);

/**
 * Multiplies `v` by the 3x3 basis columns (forward*i + left*j + up*k); `out` may alias `v`.
 *
 * Original register convention: EAX -> out, ECX -> v, stack -> m.
 * @address 0x004cc710
 */
void matrix3x3_inverse_transform_vector(real_vector3d *out, real_vector3d *v, const real_matrix3x3 &m);

/**
 * Builds the rotation for (yaw, pitch, roll) and returns its forward and up rows.
 *
 * Original register convention: EAX -> angles, EDX -> up_out, ESI -> forward_out.
 * @address 0x004cdde0
 */
void euler_angles_to_basis_vectors(const real_euler_angles3d &angles, real_vector3d &up_out, real_vector3d &forward_out);

/**
 * True when forward, left and up are unit length and mutually perpendicular.
 *
 * Original register convention: ESI -> forward, EDI -> left, EBX -> up.
 * @address 0x005579e0
 */
uint8_t real_matrix4x3_rotation_is_orthonormal(real_vector3d *forward, real_vector3d *left, real_vector3d *up);

/**
 * Re-orthonormalises left and up against forward, replacing any vector that normalises to zero by its world
 * axis.
 *
 * Original register convention: ESI -> forward, EBX -> left, EDI -> up.
 * @address 0x00558860
 */
void real_matrix4x3_rotation_rebuild_orthonormal(real_vector3d *forward, real_vector3d *left, real_vector3d *up);

/**
 * Builds `left` and `up` orthonormal to `forward` from world up, falling back to world forward when `forward`
 * is parallel to world up.
 *
 * Original register convention: ESI -> forward, EBX -> left (out), EDI -> up (out).
 * @address 0x0055eed0
 */
void real_matrix4x3_rotation_from_forward(const real_vector3d *forward, real_vector3d *left, real_vector3d *up);

}  // namespace halo::math
