/**
 * @file src/math/math_c_api.cpp
 * The math module's C ABI: one extern "C" wrapper per original function, same name, signature and calling
 * convention, forwarding to the halo::math implementation (pointers the C++ API takes by reference are
 * dereferenced here). Wrappers do nothing else.
 */

#include "halo/math/math_c_api.h"
#include "halo/math/math.hpp"

extern "C" real vector3d_magnitude_squared(real_vector3d *v)
{
    return halo::math::vector3d_magnitude_squared(*v);
}

extern "C" real vector3d_distance_squared(real_point3d *a, real_point3d *b)
{
    return halo::math::vector3d_distance_squared(*a, *b);
}

extern "C" real random_real_range(real min, real max)
{
    return halo::math::random_real_range(min, max);
}

extern "C" real vector2d_normalize_with_length(real_vector2d *v)
{
    return halo::math::vector2d_normalize_with_length(*v);
}

extern "C" void point3d_add_scaled(real_point3d *out, real_vector3d *direction, real_point3d *base, real scale)
{
    halo::math::point3d_add_scaled(*out, *direction, *base, scale);
}

extern "C" real vector3d_length(real_vector3d *v)
{
    return halo::math::vector3d_length(*v);
}

extern "C" real vector3d_normalize_with_length(real_vector3d *v)
{
    return halo::math::vector3d_normalize_with_length(*v);
}

extern "C" real random_real(void)
{
    return halo::math::random_real();
}

extern "C" void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b)
{
    halo::math::vector3d_cross_product(*out, *a, *b);
}

extern "C" int32_t random_int_range(int16_t min, int16_t max)
{
    return halo::math::random_int_range(min, max);
}

extern "C" int32_t float_compare_ascending(const void *a_value, const void *b_value)
{
    return halo::math::float_compare_ascending(a_value, b_value);
}

extern "C" real vector3d_distance(real_point3d *a, real_point3d *b)
{
    return halo::math::vector3d_distance(*a, *b);
}

extern "C" uint8_t point3d_within_horizontal_cone(const real_point3d *to_point, const real_point3d *reference, real min_cos_threshold)
{
    return halo::math::point3d_within_horizontal_cone(*to_point, *reference, min_cos_threshold);
}

extern "C" int object_sort_by_flag_then_distance(const void *a_record, const void *b_record)
{
    return halo::math::object_sort_by_flag_then_distance(a_record, b_record);
}

extern "C" void path_find_closest_point_on_segment(const real_point3d *point, const real_point3d *segment_start, const real_point3d *segment_end, real_point3d *out)
{
    halo::math::path_find_closest_point_on_segment(*point, *segment_start, *segment_end, *out);
}

extern "C" int point3d_within_radius(const real_point3d *a, const real_point3d *b, real radius)
{
    return halo::math::point3d_within_radius(*a, *b, radius);
}

extern "C" uint8_t ray2d_intersect_circle_distance(const real_vector2d *direction, const real_point2d *origin, const real_point2d *center, real *out_distance, real radius)
{
    return halo::math::ray2d_intersect_circle_distance(*direction, *origin, *center, *out_distance, radius);
}

extern "C" void vector2d_tangent_edge_directions(const real_vector2d *direction, real_vector2d *edge_pos, real_vector2d *edge_neg, real distance, real extent, real *adjacent_out)
{
    halo::math::vector2d_tangent_edge_directions(*direction, *edge_pos, *edge_neg, distance, extent, *adjacent_out);
}

extern "C" real random_range_real(real minimum, real maximum)
{
    return halo::math::random_range_real(minimum, maximum);
}

extern "C" int16_t vector3d_major_axis_index(real_vector3d *v)
{
    return halo::math::vector3d_major_axis_index(*v);
}

extern "C" real_point3d * decal_plane_solve_third_axis(real_point3d *out, uint32_t component_sign, int32_t dominant_axis, const real_plane3d *plane, const real_point2d *known)
{
    return halo::math::decal_plane_solve_third_axis(out, component_sign, dominant_axis, plane, *known);
}

extern "C" float vector3d_scalar_triple_product(const real_vector3d *a, const real_vector3d *b, const real_vector3d *c)
{
    return halo::math::vector3d_scalar_triple_product(*a, *b, *c);
}

extern "C" real_plane2d * plane2d_from_points(real_plane2d *out_plane, const real_point2d *a, const real_point2d *b)
{
    return halo::math::plane2d_from_points(out_plane, *a, *b);
}

extern "C" void plane3d_from_point_and_normal(real_plane3d *out, const real_vector3d *normal, const real_point3d *point)
{
    halo::math::plane3d_from_point_and_normal(*out, *normal, *point);
}

extern "C" void plane3d_negate(real_plane3d *out, const real_plane3d *in)
{
    halo::math::plane3d_negate(*out, *in);
}

extern "C" uint32_t color_real_to_argb_pack(float alpha, float *rgb)
{
    return halo::math::color_real_to_argb_pack(alpha, rgb);
}

extern "C" void vector3d_positive_modulo(const real_vector3d *v, real_vector3d *out, float period)
{
    halo::math::vector3d_positive_modulo(*v, *out, period);
}

extern "C" sphere_mesh * sphere_mesh_generate(int16_t subdivisions)
{
    return halo::math::sphere_mesh_generate(subdivisions);
}

extern "C" void sphere_mesh_build_face(int16_t *next_point_index, sphere_mesh *mesh, int16_t vertex_a, int16_t vertex_b, int16_t apex, int16_t *strip_cursor, sphere_mesh_edge_cache *edge_cache)
{
    halo::math::sphere_mesh_build_face(next_point_index, mesh, vertex_a, vertex_b, apex, *strip_cursor, edge_cache);
}

extern "C" int16_t sphere_mesh_get_face_point(sphere_mesh *mesh, int16_t *next_point_index, int16_t apex, int16_t vertex_a, int16_t vertex_b, int16_t row, int16_t col, sphere_mesh_edge_cache *edge_cache, sphere_mesh_face_cache *face_cache)
{
    return halo::math::sphere_mesh_get_face_point(mesh, next_point_index, apex, vertex_a, vertex_b, row, col, edge_cache, *face_cache);
}

extern "C" int16_t sphere_mesh_get_edge_point(int16_t vertex_a, int16_t vertex_b, sphere_mesh *mesh, int16_t position, int16_t *next_point_index, sphere_mesh_edge_cache *edge_cache)
{
    return halo::math::sphere_mesh_get_edge_point(vertex_a, vertex_b, mesh, position, *next_point_index, *edge_cache);
}

extern "C" void sphere_mesh_interpolate_vertex(int16_t position, int16_t total, sphere_mesh *mesh, int16_t new_index, int16_t vertex_lo, int16_t vertex_hi)
{
    halo::math::sphere_mesh_interpolate_vertex(position, total, *mesh, new_index, vertex_lo, vertex_hi);
}

extern "C" int16_t polygon2d_points_classify(real_point2d *points, int16_t count)
{
    return halo::math::polygon2d_points_classify(points, count);
}

extern "C" int16_t polygon2d_convex_hull_build(real_point2d *points, int16_t count, int16_t *hull)
{
    return halo::math::polygon2d_convex_hull_build(points, count, hull);
}

extern "C" uint8_t polygon2d_point_inside_tolerance(real_point2d *vertices, int16_t count, real_point2d *point, real tolerance)
{
    return halo::math::polygon2d_point_inside_tolerance(vertices, count, *point, tolerance);
}

extern "C" uint8_t polygon2d_point_inside_margin(real_point2d *vertices, int16_t count, real_point2d *point, real margin)
{
    return halo::math::polygon2d_point_inside_margin(vertices, count, *point, margin);
}

extern "C" int16_t polygon2d_clip_to_planes(int16_t vertex_count, real_point2d *vertices, int16_t clip_point_count, real_point2d *clip_points, int16_t maximum_count, real_point2d *out, real epsilon)
{
    return halo::math::polygon2d_clip_to_planes(vertex_count, vertices, clip_point_count, clip_points, maximum_count, out, epsilon);
}

extern "C" int16_t polygon2d_clip_to_plane(real_point2d *out, int16_t count, real_point2d *in, real_plane2d *plane, int16_t max_count, uint32_t *edge_bitmask, uint8_t *clipped_flag, real epsilon)
{
    return halo::math::polygon2d_clip_to_plane(out, count, in, *plane, max_count, edge_bitmask, clipped_flag, epsilon);
}

extern "C" int16_t polygon3d_clip_to_plane(int16_t count, real_point3d *in, real_plane3d *plane, int16_t max_count, real_point3d *out, uint8_t *clipped_flag, real epsilon, char keep_coplanar)
{
    return halo::math::polygon3d_clip_to_plane(count, in, *plane, max_count, out, clipped_flag, epsilon, keep_coplanar);
}

extern "C" int32_t uint32_log2_floor(uint32_t value)
{
    return halo::math::uint32_log2_floor(value);
}

extern "C" void bit_vector_or(uint32_t *a, int16_t bit_count, uint32_t *b, uint32_t *dst)
{
    halo::math::bit_vector_or(a, bit_count, b, dst);
}

extern "C" void matrix4x3_inverse(real_matrix4x3 *out, real_matrix4x3 *in)
{
    halo::math::matrix4x3_inverse(out, *in);
}

extern "C" void matrix4x3_from_axis_angle(real_matrix4x3 *out, real_vector3d *axis, real sin_angle, real cos_angle)
{
    halo::math::matrix4x3_from_axis_angle(*out, *axis, sin_angle, cos_angle);
}

extern "C" void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix4x3 *out)
{
    halo::math::matrix4x3_from_forward_up(*up, *forward, *out);
}

extern "C" void matrix4x3_from_euler_angles(real_matrix4x3 *out, real yaw, real pitch, real roll)
{
    halo::math::matrix4x3_from_euler_angles(*out, yaw, pitch, roll);
}

extern "C" void matrix4x3_from_quaternion(real_quaternion *q, real_matrix4x3 *out)
{
    halo::math::matrix4x3_from_quaternion(*q, *out);
}

extern "C" void quaternion_from_matrix4x3(real_matrix4x3 *m, real_quaternion *out)
{
    halo::math::quaternion_from_matrix4x3(m, *out);
}

extern "C" void matrix4x3_from_forward_up_position(real_vector3d *up, real_vector3d *forward, real_point3d *position, real_matrix4x3 *out)
{
    halo::math::matrix4x3_from_forward_up_position(up, forward, *position, out);
}

extern "C" void matrix4x3_extract_forward_up_position(real_vector3d *out_up, real_vector3d *out_forward, real_matrix4x3 *m, real_point3d *out_position)
{
    halo::math::matrix4x3_extract_forward_up_position(*out_up, *out_forward, *m, *out_position);
}

extern "C" void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m)
{
    halo::math::matrix4x3_transform_point(*out, *point, *m);
}

extern "C" void matrix4x3_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix4x3 *m)
{
    halo::math::matrix4x3_transform_vector(*out, *v, *m);
}

extern "C" void matrix4x3_transform_normal(real_vector3d *out, real_vector3d *normal, real_matrix4x3 *m)
{
    halo::math::matrix4x3_transform_normal(*out, *normal, *m);
}

extern "C" void matrix4x3_transform_plane(real_plane3d *out, real_matrix4x3 *m, real_plane3d *plane)
{
    halo::math::matrix4x3_transform_plane(*out, *m, *plane);
}

extern "C" void matrix4x3_inverse_transform_point(real_matrix4x3 *m, real_point3d *out, real_point3d *point)
{
    halo::math::matrix4x3_inverse_transform_point(*m, *out, *point);
}

extern "C" void matrix4x3_inverse_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix4x3 *m)
{
    halo::math::matrix4x3_inverse_transform_vector(*out, *v, *m);
}

extern "C" void matrix4x3_inverse_transform_normal(real_vector3d *out, real_vector3d *normal, real_matrix4x3 *m)
{
    halo::math::matrix4x3_inverse_transform_normal(*out, *normal, *m);
}

extern "C" void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out)
{
    halo::math::matrix4x3_multiply(a, b, out);
}

extern "C" void matrix4x3_multiply_sse(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out)
{
    halo::math::matrix4x3_multiply_sse(a, b, out);
}

extern "C" void matrix4x3_multiply_3dnow(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out)
{
    halo::math::matrix4x3_multiply_3dnow(a, b, out);
}

extern "C" void matrix3x3_transpose(real_matrix3x3 *out, real_matrix3x3 *in)
{
    halo::math::matrix3x3_transpose(out, in);
}

extern "C" void matrix3x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix3x3 *out)
{
    halo::math::matrix3x3_from_forward_up(*up, *forward, *out);
}

extern "C" void matrix3x3_multiply(real_matrix3x3 *out, real_matrix3x3 *a, real_matrix3x3 *b)
{
    halo::math::matrix3x3_multiply(out, a, b);
}

extern "C" void matrix3x3_inverse_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix3x3 *m)
{
    halo::math::matrix3x3_inverse_transform_vector(out, v, *m);
}

extern "C" real_quaternion * quaternion_from_matrix3x3(real_matrix3x3 *m, real_quaternion *out)
{
    return halo::math::quaternion_from_matrix3x3(m, out);
}

extern "C" void periodic_function_tables_init(void)
{
    halo::math::periodic_function_tables_init();
}

extern "C" void periodic_function_tables_free(void)
{
    halo::math::periodic_function_tables_free();
}

extern "C" real periodic_function_evaluate(periodic_function_t type, double time)
{
    return halo::math::periodic_function_evaluate(type, time);
}

extern "C" real transition_function_evaluate(transition_function_t type, real phase)
{
    return halo::math::transition_function_evaluate(type, phase);
}

extern "C" void periodic_function_build_noise_table(real *table)
{
    halo::math::periodic_function_build_noise_table(table);
}

extern "C" void periodic_function_build_transition_table(transition_function_t type, uint8_t *table)
{
    halo::math::periodic_function_build_transition_table(type, table);
}

extern "C" void periodic_function_build_table(periodic_function_t type, uint8_t *out)
{
    halo::math::periodic_function_build_table(type, out);
}

extern "C" uint32_t random_seed_generate(void)
{
    return halo::math::random_seed_generate();
}

extern "C" void sphere_point_table_init(void)
{
    halo::math::sphere_point_table_init();
}

extern "C" real random_real_range_seeded(random_seed *seed, real min, real max)
{
    return halo::math::random_real_range_seeded(*seed, min, max);
}

extern "C" real_vector3d * vector3d_randomize_direction(real_point3d *direction, real_vector3d *out, random_seed *seed, real lo, real hi)
{
    return halo::math::vector3d_randomize_direction(*direction, out, *seed, lo, hi);
}

extern "C" void vector2d_normalize(real_vector2d *v)
{
    halo::math::vector2d_normalize(*v);
}

extern "C" void vector3d_normalize(real_vector3d *v)
{
    halo::math::vector3d_normalize(*v);
}

extern "C" real vector3d_cross_product_length(real_vector3d *a, real_vector3d *b)
{
    return halo::math::vector3d_cross_product_length(*a, *b);
}

extern "C" void math_initialize(void)
{
    halo::math::math_initialize();
}

extern "C" real vector2d_angle_between(real_vector2d *a, real_vector2d *b)
{
    return halo::math::vector2d_angle_between(*a, *b);
}

extern "C" real vector3d_angle_between_4cd4f0(real_vector3d *a, real_vector3d *b)
{
    return halo::math::vector3d_angle_between_4cd4f0(*a, *b);
}

extern "C" real vector3d_angle_between_4cd5e0(real_vector3d *a, real_vector3d *b)
{
    return halo::math::vector3d_angle_between_4cd5e0(*a, *b);
}

extern "C" void vector3d_build_perpendicular(real_vector3d *out, real_vector3d *dir)
{
    halo::math::vector3d_build_perpendicular(*out, *dir);
}

extern "C" void vector3d_rotate_about_axis_perpendicular(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle)
{
    halo::math::vector3d_rotate_about_axis_perpendicular(*v, *axis, sin_angle, cos_angle);
}

extern "C" void vector3d_rotate_pair_in_plane(real_vector3d *a, real_vector3d *b, real sin_angle, real cos_angle)
{
    halo::math::vector3d_rotate_pair_in_plane(*a, *b, sin_angle, cos_angle);
}

extern "C" void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle)
{
    halo::math::vector3d_rotate_about_axis(*v, *axis, sin_angle, cos_angle);
}

extern "C" void vector3d_lerp(real_vector3d *out, real_vector3d *a, real_vector3d *b, real t)
{
    halo::math::vector3d_lerp(*out, *a, *b, t);
}

extern "C" void real_lerp_clamped(real *out, real a, real b, real t)
{
    halo::math::real_lerp_clamped(*out, a, b, t);
}

extern "C" uint8_t vector3d_rotate_toward(real_vector3d *target, real_vector3d *source, real_vector3d *out, real sin_angle, real cos_angle)
{
    return halo::math::vector3d_rotate_toward(target, *source, out, sin_angle, cos_angle);
}

extern "C" void vector3d_project_onto_unit_axis(real_vector3d *parallel_out, real_vector3d *axis, real_vector3d *v, real_vector3d *perp_out)
{
    halo::math::vector3d_project_onto_unit_axis(parallel_out, *axis, *v, perp_out);
}

extern "C" void vector3d_project_onto_axis(real_vector3d *parallel_out, real_vector3d *axis, real_vector3d *v, real_vector3d *perp_out)
{
    halo::math::vector3d_project_onto_axis(*parallel_out, *axis, *v, *perp_out);
}

extern "C" void quaternion_normalize(real_quaternion *q)
{
    halo::math::quaternion_normalize(*q);
}

extern "C" void quaternion_to_axis_angle(real_quaternion *quat, real_vector3d *axis_out, real *angle_out)
{
    halo::math::quaternion_to_axis_angle(*quat, axis_out, *angle_out);
}

extern "C" void quaternion_multiply(real_quaternion *a, real_quaternion *b, real_quaternion *out)
{
    halo::math::quaternion_multiply(a, b, out);
}

extern "C" void quaternion_lerp(real_quaternion *a, real_quaternion *b, real_quaternion *out, real t)
{
    halo::math::quaternion_lerp(*a, *b, *out, t);
}

extern "C" void quaternion_rotate_vector(real_quaternion *q, real_vector3d *v, real_vector3d *out)
{
    halo::math::quaternion_rotate_vector(*q, *v, *out);
}

extern "C" void euler_angles_to_basis_vectors(real_euler_angles3d *angles, real_vector3d *up_out, real_vector3d *forward_out)
{
    halo::math::euler_angles_to_basis_vectors(*angles, *up_out, *forward_out);
}

extern "C" real point3d_distance_squared_to_segment(real_point3d *segment_start, real_vector3d *segment_direction, real_point3d *point)
{
    return halo::math::point3d_distance_squared_to_segment(*segment_start, *segment_direction, *point);
}

extern "C" real segment3d_distance_squared_to_segment(real_point3d *b_start, real_point3d *a_start, real_vector3d *a_direction, real_vector3d *b_direction)
{
    return halo::math::segment3d_distance_squared_to_segment(b_start, a_start, a_direction, b_direction);
}

extern "C" uint8_t ray_intersects_sphere(real_point3d *origin, real_vector3d *normal_out, real_vector3d *direction, real *t_out, real_point3d *center, real radius)
{
    return halo::math::ray_intersects_sphere(*origin, normal_out, *direction, *t_out, *center, radius);
}

extern "C" uint8_t ray_intersects_cylinder(real height, real radius, real_vector3d *hit_out, real *t_out, real_point3d *center, real_point3d *origin, real_vector3d *direction)
{
    return halo::math::ray_intersects_cylinder(height, radius, hit_out, t_out, center, origin, direction);
}

extern "C" uint8_t ray_intersects_sphere_test(real_point3d *center, real_point3d *origin, real_vector3d *direction, real radius)
{
    return halo::math::ray_intersects_sphere_test(*center, *origin, *direction, radius);
}

extern "C" real ray_intersect_sphere_distance(real_point3d *origin, real_point3d *center, real_vector3d *direction, real radius)
{
    return halo::math::ray_intersect_sphere_distance(*origin, *center, *direction, radius);
}

extern "C" uint8_t triangle_point_barycentric_2d(real_point3d *a, real_point3d *v_ecx, real_point3d *v_edx, real_point3d *p, real *out_u, real *out_v)
{
    return halo::math::triangle_point_barycentric_2d(*a, *v_ecx, *v_edx, *p, *out_u, *out_v);
}

extern "C" int segment3d_within_radius_of_segment(real_point3d *a_start, real_point3d *b_start, real_vector3d *a_direction, real_vector3d *b_direction, real radius)
{
    return halo::math::segment3d_within_radius_of_segment(a_start, b_start, a_direction, b_direction, radius);
}

extern "C" uint8_t vector3d_projection_band_test(real_vector3d *axis, real_point3d *point_a, real_point3d *point_b, real radius, real max_distance, real sin_max_angle, real cos_max_angle)
{
    return halo::math::vector3d_projection_band_test(*axis, *point_a, *point_b, radius, max_distance, sin_max_angle, cos_max_angle);
}

extern "C" uint8_t plane3d_intersect_three(real_plane3d *p1, real_plane3d *p2, real_plane3d *p3, real_point3d *out)
{
    return halo::math::plane3d_intersect_three(*p1, *p2, *p3, *out);
}

extern "C" uint8_t plane3d_intersect_pair_to_line(real_vector3d *direction_out, real_plane3d *p2, real_plane3d *p1, real_point3d *point_out)
{
    return halo::math::plane3d_intersect_pair_to_line(*direction_out, *p2, *p1, *point_out);
}

extern "C" uint8_t real_seek_toward_clamped(int wrap, real *velocity, real *value, real target, real accel, real max_speed, real range_min, real range_max)
{
    return halo::math::real_seek_toward_clamped(wrap, *velocity, *value, target, accel, max_speed, range_min, range_max);
}

extern "C" void vector3d_rotate_toward_with_acceleration(real_vector3d *direction, real_vector3d *target_direction, real_vector3d *angular_velocity, real maximum_velocity, real acceleration)
{
    halo::math::vector3d_rotate_toward_with_acceleration(direction, *target_direction, *angular_velocity, maximum_velocity, acceleration);
}

extern "C" uint8_t lerp_find_threshold_byte(real lo, real hi, real threshold)
{
    return halo::math::lerp_find_threshold_byte(lo, hi, threshold);
}

extern "C" void vector3d_barycentric_interpolate(real_vector3d *out, real_vector3d *v1, real_vector3d *v2, real_vector3d *v0, float w2, float w1)
{
    halo::math::vector3d_barycentric_interpolate(*out, *v1, *v2, *v0, w2, w1);
}

extern "C" float cubic_interpolate_divided_difference(float y0, float y1, float y2, float y3, float x0, float x1, float x2, float x3, float x)
{
    return halo::math::cubic_interpolate_divided_difference(y0, y1, y2, y3, x0, x1, x2, x3, x);
}

extern "C" void vector3d_cubic_interpolate(real_vector3d *out, real_vector3d *p0, real_vector3d *p1, real_vector3d *p2, real_vector3d *p3, float t0, float t1, float t2, float t3, float t)
{
    halo::math::vector3d_cubic_interpolate(*out, *p0, *p1, *p2, *p3, t0, t1, t2, t3, t);
}

extern "C" void point3d_project_onto_line(real_point3d *point, real_vector3d *direction, real_point3d *line_origin, real_point3d *out_result)
{
    halo::math::point3d_project_onto_line(*point, *direction, *line_origin, *out_result);
}

extern "C" real real_inverse_lerp_clamped(real value, real ref_k0, real ref_k1)
{
    return halo::math::real_inverse_lerp_clamped(value, ref_k0, ref_k1);
}

extern "C" uint8_t real_matrix4x3_rotation_is_orthonormal(real_vector3d *forward, real_vector3d *left, real_vector3d *up)
{
    return halo::math::real_matrix4x3_rotation_is_orthonormal(forward, left, up);
}

extern "C" void real_matrix4x3_rotation_rebuild_orthonormal(real_vector3d *forward, real_vector3d *left, real_vector3d *up)
{
    halo::math::real_matrix4x3_rotation_rebuild_orthonormal(forward, left, up);
}

extern "C" void real_matrix4x3_rotation_from_forward(real_vector3d *forward, real_vector3d *left, real_vector3d *up)
{
    halo::math::real_matrix4x3_rotation_from_forward(forward, left, up);
}

extern "C" void bounded_ramp_profile_build(real position_error, real initial_velocity, real max_velocity, real max_acceleration, bounded_ramp_profile *profile)
{
    halo::math::bounded_ramp_profile_build(position_error, initial_velocity, max_velocity, max_acceleration, profile);
}

extern "C" void bounded_ramp_profile_synchronize(bounded_ramp_profile *profile_a, bounded_ramp_profile *profile_b, real max_acceleration)
{
    halo::math::bounded_ramp_profile_synchronize(profile_a, profile_b, max_acceleration);
}

extern "C" uint8_t bounded_ramp_profile_evaluate(bounded_ramp_profile *profile, real time, real start_position, real *out_position, real start_velocity, real *out_velocity)
{
    return halo::math::bounded_ramp_profile_evaluate(*profile, time, start_position, *out_position, start_velocity, *out_velocity);
}

extern "C" void vector3d_rotate_toward_bounded(real_vector3d *current, real_vector3d *velocity, real *bounds, real max_velocity, real max_acceleration, real_vector3d *target, real_matrix4x3 *transform)
{
    halo::math::vector3d_rotate_toward_bounded(current, velocity, bounds, max_velocity, max_acceleration, *target, transform);
}

extern "C" void vector3d_delta_toward_gravity_biased_clamp_length(real_point3d *origin, real_point3d *target, real_vector3d *out_delta, real max_length_aligned, real max_length_default)
{
    halo::math::vector3d_delta_toward_gravity_biased_clamp_length(*origin, *target, out_delta, max_length_aligned, max_length_default);
}
