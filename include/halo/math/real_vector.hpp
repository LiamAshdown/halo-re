/**
 * @file include/halo/math/real_vector.hpp
 * Real vectors and points: length, normalize, cross/dot products, projections, angles.
 * The C symbols other modules link against are the wrappers in src/math/math_c_api.cpp.
 */
#pragma once

#include "halo/math/math_types.hpp"

namespace halo::math {

/**
 * Squared length of a vector.
 *
 * Original register convention: EAX -> v.
 * @address 0x00401000
 */
real vector3d_magnitude_squared(const real_vector3d &v);

/**
 * Squared distance between two points.
 *
 * Original register convention: EAX -> a, ECX -> b.
 * @address 0x00401020
 */
real vector3d_distance_squared(const real_point3d &a, const real_point3d &b);

/**
 * Normalises a 2D vector in place and returns its length; a vector shorter than 0.0001 is left unchanged and 0
 * returned.
 *
 * Original register convention: ECX -> v.
 * @address 0x004018e0
 */
real vector2d_normalize_with_length(real_vector2d &v);

/**
 * out = base + scale * direction.
 *
 * Original register convention: EAX -> out, ECX -> direction, stack -> (base, scale).
 * @address 0x00401930
 */
void point3d_add_scaled(real_point3d &out, const real_vector3d &direction, const real_point3d &base, real scale);

/**
 * Length of a vector.
 *
 * Original register convention: EAX -> v.
 * @address 0x00401960
 */
real vector3d_length(const real_vector3d &v);

/**
 * Normalises a vector in place and returns its length; a vector shorter than 0.0001 is left unchanged and 0
 * returned.
 *
 * Original register convention: ECX -> v.
 * @address 0x00401990
 */
real vector3d_normalize_with_length(real_vector3d &v);

/**
 * Writes b x a (note the order) in the original FPU term order; `out` may alias an input only as the original
 * allows.
 *
 * Original register convention: EAX -> out, ECX -> a, stack -> b.
 * @address 0x004052c0
 */
void vector3d_cross_product(real_vector3d &out, const real_vector3d &a, const real_vector3d &b);

/**
 * Distance between two points.
 *
 * Original register convention: EAX -> a, ECX -> b.
 * @address 0x004088b0
 */
real vector3d_distance(const real_point3d &a, const real_point3d &b);

/**
 * Index (0, 1, 2) of the component with the largest magnitude.
 *
 * Original register convention: EAX -> v.
 * @address 0x0044d820
 */
int16_t vector3d_major_axis_index(const real_vector3d &v);

/**
 * a . (b x c).
 *
 * Original register convention: EAX -> b, EDX -> c, stack -> a.
 * @address 0x0044d8e0
 */
float vector3d_scalar_triple_product(const real_vector3d &a, const real_vector3d &b, const real_vector3d &c);

/**
 * Componentwise fmod by `period`, shifted into [0, period) for negative components.
 *
 * Original register convention: ESI -> v, EDI -> out, stack -> period.
 * @address 0x004588e0
 */
void vector3d_positive_modulo(const real_vector3d &v, real_vector3d &out, float period);

/**
 * Normalises a 2D vector in place; a zero vector is left unchanged.
 *
 * Original register convention: ECX -> v.
 * @address 0x004cd2e0
 */
void vector2d_normalize(real_vector2d &v);

/**
 * Normalises a vector in place; a zero vector is left unchanged.
 *
 * Original register convention: ECX -> v.
 * @address 0x004cd320
 */
void vector3d_normalize(real_vector3d &v);

/**
 * Length of the cross product of `a` and `b`.
 *
 * Original register convention: EAX -> a, ECX -> b.
 * @address 0x004cd380
 */
real vector3d_cross_product_length(const real_vector3d &a, const real_vector3d &b);

/**
 * Signed angle in radians from `a` to `b` (acos of the clamped dot product, sign of the 2D cross product).
 *
 * Original register convention: ESI -> a, EDI -> b.
 * @address 0x004cd480
 */
real vector2d_angle_between(const real_vector2d &a, const real_vector2d &b);

/**
 * Angle in radians between two vectors of any length, from the clamped double-angle cosine 2*dot^2/(|a|^2|b|^2)
 * - 1, halved and mirrored for a negative dot; 0 when either is zero.
 *
 * Original register convention: ECX -> a, EDX -> b.
 * @address 0x004cd4f0
 */
real vector3d_angle_between_4cd4f0(const real_vector3d &a, const real_vector3d &b);

/**
 * Angle in radians between two unit vectors: acos of the clamped dot product, exactly 0 when they are equal.
 *
 * Original register convention: EAX -> a, ECX -> b.
 * @address 0x004cd5e0
 */
real vector3d_angle_between_4cd5e0(const real_vector3d &a, const real_vector3d &b);

/**
 * Writes a vector perpendicular to `dir` by dropping its smallest-magnitude component and swapping and negating
 * the other two.
 *
 * Original register convention: ECX -> out, EDX -> dir.
 * @address 0x004cd670
 */
void vector3d_build_perpendicular(real_vector3d &out, const real_vector3d &dir);

/**
 * Splits `v` into its part along the unit `axis` and the remainder; either output may be NULL.
 *
 * Original register convention: EAX -> parallel_out, ECX -> axis, EDX -> v, ESI -> perp_out.
 * @address 0x004cda30
 */
void vector3d_project_onto_unit_axis(real_vector3d *parallel_out, const real_vector3d &axis, const real_vector3d &v, real_vector3d *perp_out);

/**
 * Splits `v` into its part along `axis` and the remainder; a zero axis gives a zero parallel part and
 * `perp_out` = `v`.
 *
 * Original register convention: ECX -> parallel_out, EDX -> axis, ESI -> v, EDI -> perp_out.
 * @address 0x004cda90
 */
void vector3d_project_onto_axis(real_vector3d &parallel_out, const real_vector3d &axis, const real_vector3d &v, real_vector3d &perp_out);

/**
 * Projects `point` onto the infinite line through `line_origin` along `direction`; `out_result` may alias
 * `point`.
 *
 * Original register convention: EAX -> direction, ECX -> line_origin, EDX -> out_result, stack -> point.
 * @address 0x005066e0
 */
void point3d_project_onto_line(const real_point3d &point, const real_vector3d &direction, const real_point3d &line_origin, real_point3d &out_result);

/**
 * out_delta = target - origin with gravity added to z, clamped to a length blended between max_length_aligned
 * and max_length_default by how well the delta aligns with `target`.
 *
 * Original register convention: EAX -> origin, ECX -> target, ESI -> out_delta (out), stack ->
 * (max_length_aligned, max_length_default).
 * @address 0x00572a90
 */
void vector3d_delta_toward_gravity_biased_clamp_length(const real_point3d &origin, const real_point3d &target, real_vector3d *out_delta, real max_length_aligned, real max_length_default);

}  // namespace halo::math
