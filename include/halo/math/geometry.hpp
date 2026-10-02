/**
 * @file include/halo/math/geometry.hpp
 * Planes, rays, segments, spheres, cylinders, triangles: intersection and distance queries.
 * The C symbols other modules link against are the wrappers in src/math/math_c_api.cpp.
 */
#pragma once

#include "halo/math/math_types.hpp"

namespace halo::math {

/**
 * Horizontal (x,y only) cone test: true when normalize(to_point.xy) . reference.xy is strictly greater than the
 * threshold. `reference` is used as given (not normalised); a near-zero or NaN length fails.
 *
 * Original register convention: EAX -> to_point, EDX -> reference, stack -> min_cos_threshold.
 * @address 0x00414910
 */
uint8_t point3d_within_horizontal_cone(const real_point3d &to_point, const real_point3d &reference, real min_cos_threshold);

/**
 * Writes the lerped point when the projection parameter of `point` lies in [0,1] (or is NaN), else
 * `segment_end`. The parameter is computed from start - point, so it is the negated textbook value.
 *
 * Original register convention: EAX -> point, ECX -> segment_start, EDX -> segment_end, ESI -> out.
 * @address 0x0043b2f0
 */
void path_find_closest_point_on_segment(const real_point3d &point, const real_point3d &segment_start, const real_point3d &segment_end, real_point3d &out);

/**
 * Nonzero when `a` lies within `radius` of `b` (squared comparison).
 *
 * Original register convention: EAX -> a, ECX -> b, stack -> radius.
 * @address 0x0043c340
 */
int point3d_within_radius(const real_point3d &a, const real_point3d &b, real radius);

/**
 * Intersects a 2D ray with a circle; on a hit writes the distance to the near intersection (0 when the origin
 * is inside) and returns 1. Only rays pointing toward the centre hit.
 *
 * Original register convention: EAX -> direction, ECX -> origin, EDX -> center, ESI -> out_distance, stack ->
 * radius.
 * @address 0x0043c380
 */
uint8_t ray2d_intersect_circle_distance(const real_vector2d &direction, const real_point2d &origin, const real_point2d &center, real &out_distance, real radius);

/**
 * Rotates `direction` by +-asin(min(extent/distance, 1)) into the two tangent directions and writes distance *
 * cos(angle).
 *
 * Original register convention: ECX -> direction, EDX -> edge_pos, ESI -> edge_neg, stack -> (distance, extent,
 * adjacent_out).
 * @address 0x0043c400
 */
void vector2d_tangent_edge_directions(const real_vector2d &direction, real_vector2d &edge_pos, real_vector2d &edge_neg, real distance, real extent, real &adjacent_out);

/**
 * Writes the two known coordinates of a point into the surviving axes of k_projection_axes and solves the
 * dominant axis from the plane equation (0 when the plane is edge-on). Only the low byte of component_sign and
 * the low 16 bits of dominant_axis are used. Returns `out`.
 *
 * Original register convention: EAX -> component_sign, ESI -> dominant_axis, EBX -> plane, EDI -> known, stack
 * -> out.
 * @address 0x0044d860
 */
real_point3d * decal_plane_solve_third_axis(real_point3d *out, uint32_t component_sign, int32_t dominant_axis, const real_plane3d *plane, const real_point2d &known);

/**
 * Builds the 2D line through `a` and `b` (normal (a.y-b.y, b.x-a.x), normalised; d = normal . b). Returns
 * `out_plane`, or NULL with d = 0 when the points are closer than 0.0001.
 *
 * Original register convention: plane2d_from_points(real_plane2d *out_plane in ECX, const real_point2d *a in
 * EAX, const real_point2d *b in EDX).
 * @address 0x0044d950
 */
real_plane2d * plane2d_from_points(real_plane2d *out_plane, const real_point2d &a, const real_point2d &b);

/**
 * Builds the plane with the given normal through `point`.
 *
 * Original register convention: ECX -> normal, EDX -> point, stack -> out.
 * @address 0x0044d9e0
 */
void plane3d_from_point_and_normal(real_plane3d &out, const real_vector3d *normal, const real_point3d &point);

/**
 * out = -in for all four plane components.
 *
 * Original register convention: EAX -> out, ECX -> in.
 * @address 0x0044da20
 */
void plane3d_negate(real_plane3d &out, const real_plane3d &in);

/**
 * Squared distance from `point` to the segment start + t*direction, t clamped to [0,1].
 *
 * Original register convention: EAX -> segment_start, ECX -> segment_direction, EDX -> point.
 * @address 0x004cde30
 */
real point3d_distance_squared_to_segment(const real_point3d &segment_start, const real_vector3d &segment_direction, const real_point3d &point);

/**
 * Squared distance between two segments given as start + direction.
 *
 * Original register convention: EBX -> a_start, ESI -> a_direction, EDI -> b_direction, stack -> b_start.
 * @address 0x004cdef0
 */
real segment3d_distance_squared_to_segment(real_point3d *b_start, real_point3d *a_start, real_vector3d *a_direction, real_vector3d *b_direction);

/**
 * Intersects a ray with a sphere; on a hit writes the surface normal and ray parameter.
 *
 * Original register convention: EAX -> origin, ECX -> normal_out, EDX -> direction, EDI -> t_out, stack ->.
 * @address 0x004ce3a0
 */
uint8_t ray_intersects_sphere(const real_point3d &origin, real_vector3d *normal_out, const real_vector3d &direction, real &t_out, const real_point3d &center, real radius);

/**
 * Intersects a ray with a vertical cylinder of the given height and radius standing on `center`; on a hit
 * writes the hit point and parameter.
 *
 * Original register convention: EAX -> t_out, ECX -> center, EBX -> origin, ESI -> direction, stack ->
 * (height,.
 * @address 0x004ce4e0
 */
uint8_t ray_intersects_cylinder(real height, real radius, real_vector3d *hit_out, real *t_out, real_point3d *center, real_point3d *origin, real_vector3d *direction);

/**
 * True when the ray (or segment) from `origin` along `direction` passes within `radius` of `center`.
 *
 * Original register convention: EAX -> origin, ECX -> center, EDX -> direction, stack -> radius.
 * @address 0x004ce6c0
 */
uint8_t ray_intersects_sphere_test(const real_point3d &center, const real_point3d &origin, const real_vector3d &direction, real radius);

/**
 * Returns the ray parameter of the first intersection with the sphere (in units of `direction`), 0 when the
 * origin is inside, 3.4028235e38 on a miss.
 *
 * Original register convention: EAX -> origin, ECX -> center, EDX -> direction, stack -> radius.
 * @address 0x004ce7d0
 */
real ray_intersect_sphere_distance(const real_point3d &origin, const real_point3d &center, const real_vector3d &direction, real radius);

/**
 * Projects a triangle and a point onto the triangle's dominant plane and writes the barycentric (u, v) of the
 * point. Returns 0 when the point is off the plane or outside the triangle.
 *
 * Original register convention: EAX -> a, ECX -> v_ecx, EDX -> v_edx, ESI -> p, stack -> (out_u, out_v).
 * @address 0x004ce8c0
 */
uint8_t triangle_point_barycentric_2d(const real_point3d &a, const real_point3d &v_ecx, const real_point3d &v_edx, const real_point3d &p, real &out_u, real &out_v);

/**
 * Nonzero when two segments (start + direction) come within `radius` of each other.
 *
 * Original register convention: EBX -> b_start, ESI -> a_direction, EDI -> b_direction, stack -> (a_start,
 * radius).
 * @address 0x004ceae0
 */
int segment3d_within_radius_of_segment(real_point3d *a_start, real_point3d *b_start, real_vector3d *a_direction, real_vector3d *b_direction, real radius);

/**
 * Tests whether the segment between two points, projected along `axis`, stays within the radius / distance /
 * angle band.
 *
 * Original register convention: EAX -> axis, ECX -> point_a, EDX -> point_b, stack -> (param_1, param_2,.
 * @address 0x004cef90
 */
uint8_t vector3d_projection_band_test(const real_vector3d &axis, const real_point3d &point_a, const real_point3d &point_b, real radius, real max_distance, real sin_max_angle, real cos_max_angle);

/**
 * Writes the intersection point of three planes; returns 0 when the normals are degenerate (|det| < 0.0001).
 *
 * Original register convention: EBX -> p2, EDI -> p3, ESI -> out, stack -> p1.
 * @address 0x004cf040
 */
uint8_t plane3d_intersect_three(real_plane3d &p1, real_plane3d &p2, real_plane3d &p3, real_point3d &out);

/**
 * Intersects two planes: writes the line direction cross(p1.normal, p2.normal) and a point on the line. Returns
 * 0 (only the direction written) when the planes are parallel.
 *
 * Original register convention: ECX -> direction_out, EDX -> p2, ESI -> p1, EDI -> point_out.
 * @address 0x004cf1e0
 */
uint8_t plane3d_intersect_pair_to_line(real_vector3d &direction_out, const real_plane3d &p2, const real_plane3d &p1, real_point3d &point_out);

}  // namespace halo::math
