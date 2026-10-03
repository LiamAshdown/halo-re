/**
 * @file include/halo/physics/api.hpp
 * Functions of the physics module that other modules and the data tables call (namespace halo::physics). The record types are
 * forward-declared, so the header is light enough for every caller and for the data tables.
 */
#pragma once

#include <stdint.h>
#include "halo/core/link.hpp"
#include "halo/physics/vars.hpp"

struct breakable_surface_globals;
struct data_array;
typedef uint32_t datum_index;

struct ModelCollisionGeometry;
struct ModelCollisionGeometryBSP;
struct PhysicsMassPoint;
struct PointPhysics;
struct TagReflexive;
struct bsp_leaf_reference;
struct collision_bsp_boundary_clip;
struct collision_bsp_pill_query;
struct collision_bsp_pill_result;
struct collision_bsp_segment_query;
struct collision_bsp_segment_result;
struct collision_bsp_sphere_query;
struct collision_bsp_sphere_result;
struct collision_result;
struct damage_data;
struct mass_point_state;
struct object_collision_context;
struct object_node_collision_result;
struct object_physics_context;
struct object_physics_ray_result;
struct physics_model;
struct physics_model_contact;
struct physics_model_pill;
struct physics_model_shape;
struct physics_model_sphere;
struct physics_point_walk_state;
struct physics_scalar_range;
struct physics_scalar_rates;
struct powered_mass_point_state;
struct real_matrix4x3;
struct real_plane3d;
struct real_point2d;
struct real_point3d;
struct real_vector2d;
struct real_vector3d;
typedef float real;

namespace halo::physics {

/**
 * The engine globals the physics module owns (their storage is defined by standalone/data under the original link names);
 * other modules reach them through globals().
 */
struct Globals {
    ModelCollisionGeometryBSP *&collision_bsp;
    ModelCollisionGeometryBSP *&structure_collision_bsp;
    float &gravity;
    real_point3d *&sphere_point_table;
    int16_t &sphere_point_table_count;
    datum_index *&collideable_cluster_first;
    data_array *&collideable_object_references;
    int32_t &object_cluster_stamp;
    breakable_surface_globals *&breakable_surface_state;
};

Globals &globals();

inline auto &k_physics_gravity = halo::link::ref<float>(halo::physics::vars().k_physics_gravity);

void breakable_surface_apply_damage(damage_data *damage, int32_t surface_index, int32_t collision_surface_index);
void breakable_surface_damage_in_blast_radius(damage_data *damage);
void breakable_surface_shatter(uint16_t breakable_surface_index, damage_data *damage, int32_t collision_surface_index);
int32_t bsp2d_node_find_leaf(int32_t node_index, TagReflexive *bsp2d_nodes, real_point2d *point);
uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point);
uint8_t collision_bsp_query_pill_init(ModelCollisionGeometryBSP *bsp, collision_bsp_pill_result *result, real_point3d *origin, real_vector3d *delta, float radius, float max_fraction);
uint8_t collision_bsp_query_pill_leaf_recursive(collision_bsp_pill_query *query, int32_t bsp2d_node_index);
uint8_t collision_bsp_query_pill_leaf_test_surface(collision_bsp_pill_query *query, int32_t surface_index);
uint8_t collision_bsp_query_pill_node_recursive(collision_bsp_pill_query *query, int32_t node_index);
uint8_t collision_bsp_query_segment_init(uint32_t flags, collision_bsp_segment_result *result, ModelCollisionGeometryBSP *bsp, int16_t breakable_surface_count, uint32_t *breakable_surfaces, real_point3d *origin, real_vector3d *delta, float max_fraction);
uint8_t collision_bsp_query_segment_node_recursive(collision_bsp_segment_query *query, uint32_t node_index, float t_min, float t_max);
void collision_bsp_query_sphere_collect_geometry(collision_bsp_sphere_query *query, int32_t surface_index);
uint32_t collision_bsp_query_sphere_init(ModelCollisionGeometryBSP *bsp, int16_t breakable_surface_count, collision_bsp_sphere_result *result, uint32_t *breakable_surfaces, real_point3d *center, float radius);
void collision_bsp_query_sphere_leaf_edge_recursive(collision_bsp_sphere_query *query, int32_t bsp2d_node_index);
void collision_bsp_query_sphere_node_recursive(collision_bsp_sphere_query *query, uint32_t node_index);
uint32_t collision_bsp_surface_clip_line_2d(collision_bsp_boundary_clip *clip, ModelCollisionGeometryBSP *bsp, int32_t surface_index, real_point2d *origin, real_vector2d *direction);
uint32_t collision_bsp_surface_closest_edge_point_2d(ModelCollisionGeometryBSP *bsp, int32_t surface_index, uint16_t axis, uint8_t sign, real_point2d *point, real_point2d *out_point);
int16_t collision_bsp_surface_get_vertices(ModelCollisionGeometryBSP *bsp, int32_t surface_index, real_point3d *out_vertices);
real_point3d * collision_bsp_surface_solve_third_axis(ModelCollisionGeometryBSP *collision_bsp, int32_t surface_index, uint8_t component_sign, real_point3d *out, int32_t dominant_axis, const real_point2d *known);
uint8_t collision_bsp_surface_test_point_2d(ModelCollisionGeometryBSP *bsp, int16_t breakable_surface_count, uint32_t *breakable_surfaces, int32_t surface_index, int16_t axis, uint8_t sign, real_point2d *point);
int32_t collision_bsp_surface_test_point_leaf(ModelCollisionGeometryBSP *bsp, int32_t leaf_index, int16_t breakable_surface_count, uint32_t *breakable_surfaces, uint32_t plane_index, real_point3d *crossing_point, uint8_t two_sided);
uint8_t collision_bsp_surface_test_point_side_2d(ModelCollisionGeometryBSP *bsp, real_point2d *point, int32_t surface_index, int16_t axis, uint8_t sign);
int16_t model_collision_geometry_resolve_material_type(int16_t material_index, ModelCollisionGeometry *definition);
void collision_gather_nearby_object_shapes(uint32_t flags, uint32_t start_object_index, real_point3d *origin, float radius, float x_offset, float y_offset, uint32_t exclude_object_index, physics_model *model);
uint8_t collision_test_movement_pill(uint32_t flags, real_point3d *origin, float radius, real_vector3d *delta, collision_result *result);
uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object_index, collision_result *result);
uint8_t collision_test_movement_segment_between_points(real_point3d *origin, real_point3d *target, uint32_t flags, uint32_t exclude_object_index, collision_result *result);
uint8_t object_collision_context_build(uint32_t object_index, object_collision_context *out_context);
uint8_t object_collision_context_gather_sphere_shapes(object_collision_context *context, real_point3d *origin, float radius_scale, float margin, float thickness, physics_model *model);
uint8_t object_collision_context_test_pill(object_collision_context *context, real_point3d *origin, real_vector3d *delta, float radius_scale, object_node_collision_result *out_result);
uint32_t object_collision_context_test_point(object_collision_context *context, real_point3d *point);
uint8_t object_collision_context_test_segment(object_collision_context *context, uint32_t flags, real_point3d *origin, real_vector3d *delta, object_node_collision_result *out_result);
uint8_t object_collision_test_cluster_group(uint32_t flags, real_point3d *position, uint32_t exclude_object_index);
uint8_t object_collision_test_nearby_chain(uint32_t start_object_index, uint32_t type_mask, real_point3d *position, uint32_t exclude_object_index);
uint8_t object_collision_test_ray_nearby_chain(uint32_t start_object_index, uint32_t type_mask, uint32_t test_flags, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object_index, collision_result *out_result);
uint8_t object_physics_add_mass_point_shapes(float x_offset, float y_offset, object_physics_context *context, int16_t *model_counts);
void object_physics_blend_friction_axes(int16_t friction_type, float parallel_scale, float perpendicular_scale, float *friction, real_vector3d *forward, real_vector3d *up);
uint8_t object_physics_check_impact_damage(uint32_t *self_object_index, uint32_t candidate_object_index);
void object_physics_compute_mass_point_forces(object_physics_context *context, powered_mass_point_state *powered_states, uint32_t mass_points_address, real_vector3d *out_force, real_vector3d *out_torque);
uint8_t object_physics_context_build(uint32_t object_index, object_physics_context *out_context);
void object_physics_handle_nearby_object_impacts(uint32_t object_index);
void object_physics_integrate_and_test_at_rest(object_physics_context *context, mass_point_state *mass_point_states, real_vector3d *torque, real_vector3d *force);
void object_physics_mass_point_resolve_ground_contact(uint32_t exclude_object_index, mass_point_state *mass_point, PhysicsMassPoint *definition);
void object_physics_mass_point_update_orientation(real_vector3d *axis, real_vector3d *up, real_vector3d *forward, real_vector3d *fallback_forward, real_vector3d *fallback_up);
uint8_t object_physics_resolve_mass_point_overlap(object_physics_context *self, object_physics_context *other);
uint8_t object_physics_test_point_against_mass_points(object_physics_context *context, real_point3d *world_point, int16_t *out_index);
uint8_t object_physics_test_ray_against_mass_points(real_point3d *world_origin, real_vector3d *world_direction, object_physics_ray_result *out_result, object_physics_context *context);
void object_physics_tick(uint32_t object_index, powered_mass_point_state *powered_states, uint32_t mass_points, real_vector3d *extra_force, real_vector3d *extra_torque);
void object_physics_tick_single_pass(uint32_t object_index, powered_mass_point_state *powered_states, mass_point_state *mass_point_states, real_vector3d *extra_force, real_vector3d *extra_torque);
void physics_clamp_value_to_spring_range(float *value, physics_scalar_rates *rates, float step);
int16_t physics_resolve_material_type(uint32_t object_index, int16_t vertex_slot);
void physics_scalar_advance_and_wrap(physics_scalar_range *range, float *value, uint8_t wrap, float delta);
float physics_scalar_approach_direction(physics_scalar_range *range, float value, uint8_t wrap, float target);
uint8_t physics_scalar_move_toward_target(physics_scalar_range *range, float *value, uint8_t wrap, float target, float rate);
uint8_t physics_scalar_step_to_target_clamped(physics_scalar_rates *rates, float *value, float target, float step);
void point_physics_interpolate(PointPhysics *out, const PointPhysics *from, const PointPhysics *to, float fraction);
uint32_t point_physics_tick(real_vector3d *velocity, uint32_t flags_arg, PointPhysics *definition, bsp_leaf_reference *out_leaf, uint32_t unused_param_4, real_point3d *position, real_vector3d *wind, real_vector3d *out_normal, int16_t *out_material_type, real radius, real dt);
void vector3d_project_onto_direction(real_vector3d *out, const real_vector3d *axis, const real_vector3d *v);
uint8_t physics_model_build_from_sphere_query(uint32_t flags, real_point3d *center, float radius, float x_offset, float y_offset, uint32_t exclude_object_index, physics_model *model);
int16_t physics_model_slide_along_contacts(real_point3d *start_position, real_vector3d *delta, physics_model *model, real_point3d *out_position, real_vector3d *out_velocity, int16_t max_contacts, physics_model_contact *contacts);
uint8_t physics_point_find_clear_position(uint32_t flags, real_point3d *current_position, float sample_radius, float x_margin, float y_margin, uint32_t exclude_object_index, real_point3d *out_position);
uint8_t physics_point_refresh_leaf(real_point3d *point, float radius);
void physics_point_walk_toward_target(physics_point_walk_state *state, real_point3d *start_position, uint32_t flags, real_vector3d *step_direction, uint32_t exclude_object_index);
void physics_shape_add_edge_proxy(int32_t edge_index, ModelCollisionGeometryBSP *bsp, real_matrix4x3 *matrix, float height_offset, float thickness, int32_t object_index, physics_model *model);
void physics_shape_add_surface_proxy(ModelCollisionGeometryBSP *bsp, float *moving_frame, int32_t surface_index, float margin, float thickness, int32_t object_index, physics_model *model);
void physics_shape_add_vertex_proxy(ModelCollisionGeometryBSP *bsp, uint32_t vertex_index, uint32_t object_index, real_matrix4x3 *matrix, float height_offset, float radius, physics_model *model);
void physics_shape_build_proxies_from_query(collision_bsp_sphere_result *result, real_matrix4x3 *matrix, ModelCollisionGeometryBSP *bsp, float margin, float thickness, int32_t object_index, physics_model *model);
void physics_shape_edge_to_pill_and_quad(physics_model *model, real_point3d *near_vertex, real_vector3d *edge_dir, float height_offset, float thickness, uint32_t object_index, int32_t surface_index, uint8_t surface_flags, int8_t breakable_surface_index, int16_t material_type);
uint8_t physics_shape_pill_sweep_test_point(real_point3d *near_vertex, real_vector3d *delta, real_point3d *origin, real_vector3d *edge_dir, float radius, float *out_t, float *out_edge_fraction);
uint8_t physics_shape_pill_test_point(real_point3d *point, physics_model_pill *pill, real_plane3d *out_normal, float *out_depth);
uint8_t physics_shape_pill_test_ray(real_vector3d *delta, real_point3d *origin, real_plane3d *out_plane, physics_model_pill *pill, float *out_t);
uint8_t physics_shape_polygon_test_point(physics_model_shape *shape, real_point3d *point, float *out_depth, real_plane3d *out_normal);
uint8_t physics_shape_polygon_test_ray(real_point3d *origin, physics_model_shape *shape, real_vector3d *delta, float *out_t, real_plane3d *out_plane);
uint8_t physics_shape_sphere_sweep_test_ray(real_point3d *point, real_point3d *origin, real_vector3d *delta, float *out_t, float radius);
uint8_t physics_shape_sphere_test_point(real_point3d *point, physics_model_sphere *sphere, real_plane3d *out_normal, float *out_depth);
uint8_t physics_shape_sphere_test_ray(real_point3d *origin, real_vector3d *delta, physics_model_sphere *sphere, real_plane3d *out_plane, float *out_t);
void physics_shape_surface_to_polygon(int16_t vertex_count, real_point3d *vertices, real_plane3d *plane, float margin, float thickness, uint32_t object_index, int32_t surface_index, uint8_t surface_flags, int8_t breakable_surface_index, int16_t material_type, physics_model *model);
uint32_t physics_shape_test_point(physics_model *model, real_point3d *point, physics_model_contact *out_contact);
uint32_t physics_shape_test_ray(physics_model *model, real_point3d *origin, real_vector3d *delta, physics_model_contact *out_contact);
void physics_shape_vertex_to_sphere(physics_model *model, real_point3d *vertex, int16_t material_type, float height_offset, float radius, uint32_t object_index, int32_t surface_index, uint8_t surface_flags, int8_t breakable_surface_index);
int16_t physics_sweep_capsule_step(real_point3d *origin, real_vector3d *delta, real_vector3d *out_velocity, uint32_t exclude_object_index, uint32_t flags, float pill_height, float pill_radius, real_point3d *out_position, int16_t max_contacts, physics_model_contact *contacts);

}
