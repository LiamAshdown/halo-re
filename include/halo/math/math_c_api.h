/**
 * @file include/halo/math/math_c_api.h
 * The C ABI of the math module: every original function with its original C signature and C linkage, the
 * symbols the link tables, the code-pointer slots and the unconverted modules use (baseline:
 * symbols/exports/math.txt). Defined in src/math/math_c_api.cpp; documented on the halo::math functions.
 */
#pragma once

#include "halo/math/math_types.hpp"

#ifdef __cplusplus
extern "C" {
#endif

real vector3d_magnitude_squared(real_vector3d *v);
real vector3d_distance_squared(real_point3d *a, real_point3d *b);
real random_real_range(real min, real max);
real vector2d_normalize_with_length(real_vector2d *v);
void point3d_add_scaled(real_point3d *out, real_vector3d *direction, real_point3d *base, real scale);
real vector3d_length(real_vector3d *v);
real vector3d_normalize_with_length(real_vector3d *v);
real random_real(void);
void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b);
int32_t random_int_range(int16_t min, int16_t max);
int32_t float_compare_ascending(const void *a_value, const void *b_value);
real vector3d_distance(real_point3d *a, real_point3d *b);
uint8_t point3d_within_horizontal_cone(const real_point3d *to_point, const real_point3d *reference, real min_cos_threshold);
int object_sort_by_flag_then_distance(const void *a_record, const void *b_record);
void path_find_closest_point_on_segment(const real_point3d *point, const real_point3d *segment_start, const real_point3d *segment_end, real_point3d *out);
int point3d_within_radius(const real_point3d *a, const real_point3d *b, real radius);
uint8_t ray2d_intersect_circle_distance(const real_vector2d *direction, const real_point2d *origin, const real_point2d *center, real *out_distance, real radius);
void vector2d_tangent_edge_directions(const real_vector2d *direction, real_vector2d *edge_pos, real_vector2d *edge_neg, real distance, real extent, real *adjacent_out);
real random_range_real(real minimum, real maximum);
int16_t vector3d_major_axis_index(real_vector3d *v);
real_point3d * decal_plane_solve_third_axis(real_point3d *out, uint32_t component_sign, int32_t dominant_axis, const real_plane3d *plane, const real_point2d *known);
float vector3d_scalar_triple_product(const real_vector3d *a, const real_vector3d *b, const real_vector3d *c);
real_plane2d * plane2d_from_points(real_plane2d *out_plane, const real_point2d *a, const real_point2d *b);
void plane3d_from_point_and_normal(real_plane3d *out, const real_vector3d *normal, const real_point3d *point);
void plane3d_negate(real_plane3d *out, const real_plane3d *in);
uint32_t color_real_to_argb_pack(float alpha, float *rgb);
void vector3d_positive_modulo(const real_vector3d *v, real_vector3d *out, float period);
sphere_mesh * sphere_mesh_generate(int16_t subdivisions);
void sphere_mesh_build_face(int16_t *next_point_index, sphere_mesh *mesh, int16_t vertex_a, int16_t vertex_b, int16_t apex, int16_t *strip_cursor, sphere_mesh_edge_cache *edge_cache);
int16_t sphere_mesh_get_face_point(sphere_mesh *mesh, int16_t *next_point_index, int16_t apex, int16_t vertex_a, int16_t vertex_b, int16_t row, int16_t col, sphere_mesh_edge_cache *edge_cache, sphere_mesh_face_cache *face_cache);
int16_t sphere_mesh_get_edge_point(int16_t vertex_a, int16_t vertex_b, sphere_mesh *mesh, int16_t position, int16_t *next_point_index, sphere_mesh_edge_cache *edge_cache);
void sphere_mesh_interpolate_vertex(int16_t position, int16_t total, sphere_mesh *mesh, int16_t new_index, int16_t vertex_lo, int16_t vertex_hi);
int16_t polygon2d_points_classify(real_point2d *points, int16_t count);
int16_t polygon2d_convex_hull_build(real_point2d *points, int16_t count, int16_t *hull);
uint8_t polygon2d_point_inside_tolerance(real_point2d *vertices, int16_t count, real_point2d *point, real tolerance);
uint8_t polygon2d_point_inside_margin(real_point2d *vertices, int16_t count, real_point2d *point, real margin);
int16_t polygon2d_clip_to_planes(int16_t vertex_count, real_point2d *vertices, int16_t clip_point_count, real_point2d *clip_points, int16_t maximum_count, real_point2d *out, real epsilon);
int16_t polygon2d_clip_to_plane(real_point2d *out, int16_t count, real_point2d *in, real_plane2d *plane, int16_t max_count, uint32_t *edge_bitmask, uint8_t *clipped_flag, real epsilon);
int16_t polygon3d_clip_to_plane(int16_t count, real_point3d *in, real_plane3d *plane, int16_t max_count, real_point3d *out, uint8_t *clipped_flag, real epsilon, char keep_coplanar);
int32_t uint32_log2_floor(uint32_t value);
void bit_vector_or(uint32_t *a, int16_t bit_count, uint32_t *b, uint32_t *dst);
void matrix4x3_inverse(real_matrix4x3 *out, real_matrix4x3 *in);
void matrix4x3_from_axis_angle(real_matrix4x3 *out, real_vector3d *axis, real sin_angle, real cos_angle);
void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix4x3 *out);
void matrix4x3_from_euler_angles(real_matrix4x3 *out, real yaw, real pitch, real roll);
void matrix4x3_from_quaternion(real_quaternion *q, real_matrix4x3 *out);
void quaternion_from_matrix4x3(real_matrix4x3 *m, real_quaternion *out);
void matrix4x3_from_forward_up_position(real_vector3d *up, real_vector3d *forward, real_point3d *position, real_matrix4x3 *out);
void matrix4x3_extract_forward_up_position(real_vector3d *out_up, real_vector3d *out_forward, real_matrix4x3 *m, real_point3d *out_position);
void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m);
void matrix4x3_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix4x3 *m);
void matrix4x3_transform_normal(real_vector3d *out, real_vector3d *normal, real_matrix4x3 *m);
void matrix4x3_transform_plane(real_plane3d *out, real_matrix4x3 *m, real_plane3d *plane);
void matrix4x3_inverse_transform_point(real_matrix4x3 *m, real_point3d *out, real_point3d *point);
void matrix4x3_inverse_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix4x3 *m);
void matrix4x3_inverse_transform_normal(real_vector3d *out, real_vector3d *normal, real_matrix4x3 *m);
void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out);
void matrix4x3_multiply_sse(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out);
void matrix4x3_multiply_3dnow(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out);
void matrix3x3_transpose(real_matrix3x3 *out, real_matrix3x3 *in);
void matrix3x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix3x3 *out);
void matrix3x3_multiply(real_matrix3x3 *out, real_matrix3x3 *a, real_matrix3x3 *b);
void matrix3x3_inverse_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix3x3 *m);
real_quaternion * quaternion_from_matrix3x3(real_matrix3x3 *m, real_quaternion *out);
void periodic_function_tables_init(void);
void periodic_function_tables_free(void);
real periodic_function_evaluate(periodic_function_t type, double time);
real transition_function_evaluate(transition_function_t type, real phase);
void periodic_function_build_noise_table(real *table);
void periodic_function_build_transition_table(transition_function_t type, uint8_t *table);
void periodic_function_build_table(periodic_function_t type, uint8_t *out);
uint32_t random_seed_generate(void);
void sphere_point_table_init(void);
real random_real_range_seeded(random_seed *seed, real min, real max);
real_vector3d * vector3d_randomize_direction(real_point3d *direction, real_vector3d *out, random_seed *seed, real lo, real hi);
void vector2d_normalize(real_vector2d *v);
void vector3d_normalize(real_vector3d *v);
real vector3d_cross_product_length(real_vector3d *a, real_vector3d *b);
void math_initialize(void);
real vector2d_angle_between(real_vector2d *a, real_vector2d *b);
real vector3d_angle_between_4cd4f0(real_vector3d *a, real_vector3d *b);
real vector3d_angle_between_4cd5e0(real_vector3d *a, real_vector3d *b);
void vector3d_build_perpendicular(real_vector3d *out, real_vector3d *dir);
void vector3d_rotate_about_axis_perpendicular(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle);
void vector3d_rotate_pair_in_plane(real_vector3d *a, real_vector3d *b, real sin_angle, real cos_angle);
void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle);
void vector3d_lerp(real_vector3d *out, real_vector3d *a, real_vector3d *b, real t);
void real_lerp_clamped(real *out, real a, real b, real t);
uint8_t vector3d_rotate_toward(real_vector3d *target, real_vector3d *source, real_vector3d *out, real sin_angle, real cos_angle);
void vector3d_project_onto_unit_axis(real_vector3d *parallel_out, real_vector3d *axis, real_vector3d *v, real_vector3d *perp_out);
void vector3d_project_onto_axis(real_vector3d *parallel_out, real_vector3d *axis, real_vector3d *v, real_vector3d *perp_out);
void quaternion_normalize(real_quaternion *q);
void quaternion_to_axis_angle(real_quaternion *quat, real_vector3d *axis_out, real *angle_out);
void quaternion_multiply(real_quaternion *a, real_quaternion *b, real_quaternion *out);
void quaternion_lerp(real_quaternion *a, real_quaternion *b, real_quaternion *out, real t);
void quaternion_rotate_vector(real_quaternion *q, real_vector3d *v, real_vector3d *out);
void euler_angles_to_basis_vectors(real_euler_angles3d *angles, real_vector3d *up_out, real_vector3d *forward_out);
real point3d_distance_squared_to_segment(real_point3d *segment_start, real_vector3d *segment_direction, real_point3d *point);
real segment3d_distance_squared_to_segment(real_point3d *b_start, real_point3d *a_start, real_vector3d *a_direction, real_vector3d *b_direction);
uint8_t ray_intersects_sphere(real_point3d *origin, real_vector3d *normal_out, real_vector3d *direction, real *t_out, real_point3d *center, real radius);
uint8_t ray_intersects_cylinder(real height, real radius, real_vector3d *hit_out, real *t_out, real_point3d *center, real_point3d *origin, real_vector3d *direction);
uint8_t ray_intersects_sphere_test(real_point3d *center, real_point3d *origin, real_vector3d *direction, real radius);
real ray_intersect_sphere_distance(real_point3d *origin, real_point3d *center, real_vector3d *direction, real radius);
uint8_t triangle_point_barycentric_2d(real_point3d *a, real_point3d *v_ecx, real_point3d *v_edx, real_point3d *p, real *out_u, real *out_v);
int segment3d_within_radius_of_segment(real_point3d *a_start, real_point3d *b_start, real_vector3d *a_direction, real_vector3d *b_direction, real radius);
uint8_t vector3d_projection_band_test(real_vector3d *axis, real_point3d *point_a, real_point3d *point_b, real radius, real max_distance, real sin_max_angle, real cos_max_angle);
uint8_t plane3d_intersect_three(real_plane3d *p1, real_plane3d *p2, real_plane3d *p3, real_point3d *out);
uint8_t plane3d_intersect_pair_to_line(real_vector3d *direction_out, real_plane3d *p2, real_plane3d *p1, real_point3d *point_out);
uint8_t real_seek_toward_clamped(int wrap, real *velocity, real *value, real target, real accel, real max_speed, real range_min, real range_max);
void vector3d_rotate_toward_with_acceleration(real_vector3d *direction, real_vector3d *target_direction, real_vector3d *angular_velocity, real maximum_velocity, real acceleration);
uint8_t lerp_find_threshold_byte(real lo, real hi, real threshold);
void vector3d_barycentric_interpolate(real_vector3d *out, real_vector3d *v1, real_vector3d *v2, real_vector3d *v0, float w2, float w1);
float cubic_interpolate_divided_difference(float y0, float y1, float y2, float y3, float x0, float x1, float x2, float x3, float x);
void vector3d_cubic_interpolate(real_vector3d *out, real_vector3d *p0, real_vector3d *p1, real_vector3d *p2, real_vector3d *p3, float t0, float t1, float t2, float t3, float t);
void point3d_project_onto_line(real_point3d *point, real_vector3d *direction, real_point3d *line_origin, real_point3d *out_result);
real real_inverse_lerp_clamped(real value, real ref_k0, real ref_k1);
uint8_t real_matrix4x3_rotation_is_orthonormal(real_vector3d *forward, real_vector3d *left, real_vector3d *up);
void real_matrix4x3_rotation_rebuild_orthonormal(real_vector3d *forward, real_vector3d *left, real_vector3d *up);
void real_matrix4x3_rotation_from_forward(real_vector3d *forward, real_vector3d *left, real_vector3d *up);
void bounded_ramp_profile_build(real position_error, real initial_velocity, real max_velocity, real max_acceleration, bounded_ramp_profile *profile);
void bounded_ramp_profile_synchronize(bounded_ramp_profile *profile_a, bounded_ramp_profile *profile_b, real max_acceleration);
uint8_t bounded_ramp_profile_evaluate(bounded_ramp_profile *profile, real time, real start_position, real *out_position, real start_velocity, real *out_velocity);
void vector3d_rotate_toward_bounded(real_vector3d *current, real_vector3d *velocity, real *bounds, real max_velocity, real max_acceleration, real_vector3d *target, real_matrix4x3 *transform);
void vector3d_delta_toward_gravity_biased_clamp_length(real_point3d *origin, real_point3d *target, real_vector3d *out_delta, real max_length_aligned, real max_length_default);

#ifdef __cplusplus
}
#endif
