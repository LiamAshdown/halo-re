/**
 * The physics module's public API (include/halo/physics/api.hpp): forwards the calls other modules make to the C++ implementation
 * in namespace halo::physics or to the member function of the record it operates on, and exposes the module's engine globals.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "physics.h"
#include "effects.h"
#include "game.h"
#include "sound.h"
#include "bitmaps.h"
#include <string.h>
#include "units.h"
#include "projectiles.h"

#include "halo/physics/breakable_surface.hpp"
#include "halo/physics/collision_bsp.hpp"
#include "halo/physics/collision_world.hpp"
#include "halo/physics/object_physics.hpp"
#include "halo/physics/motion.hpp"
#include "halo/physics/physics_model.hpp"
#include "halo/physics/api.hpp"
#include "halo/core/link.hpp"
#include "halo/objects/vars.hpp"
#include "halo/physics/vars.hpp"

static auto &global_collision_bsp = halo::link::ref<ModelCollisionGeometryBSP *>(halo::physics::vars().global_collision_bsp);
static auto &global_structure_collision_bsp = halo::link::ref<ModelCollisionGeometryBSP *>(halo::physics::vars().global_structure_collision_bsp);
static auto &sphere_point_table = halo::link::ref<real_point3d *>(halo::physics::vars().sphere_point_table);
static auto &sphere_point_table_count = halo::link::ref<int16_t>(halo::physics::vars().sphere_point_table_count);
static auto &collideable_cluster_first = halo::link::ref<datum_index *>(halo::objects::vars().collideable_cluster_first);
static auto &collideable_object_references = halo::link::ref<data_array *>(halo::physics::vars().collideable_object_references);
static auto &object_cluster_stamp = halo::link::ref<int32_t>(halo::physics::vars().object_cluster_stamp);
static auto &breakable_surface_state = halo::link::ref<breakable_surface_globals *>(halo::physics::vars().breakable_surface_state);

namespace halo::physics {

Globals &globals()
{
    static Globals instance{::global_collision_bsp, ::global_structure_collision_bsp, k_physics_gravity, ::sphere_point_table, ::sphere_point_table_count, ::collideable_cluster_first, ::collideable_object_references, ::object_cluster_stamp, ::breakable_surface_state};
    return instance;
}

void breakable_surface_apply_damage(damage_data *damage, int32_t surface_index, int32_t collision_surface_index)
{
    halo::physics::BreakableSurfaces::apply_damage(damage, surface_index, collision_surface_index);
}

void breakable_surface_damage_in_blast_radius(damage_data *damage)
{
    halo::physics::BreakableSurfaces::damage_in_blast_radius(damage);
}

void breakable_surface_shatter(uint16_t breakable_surface_index, damage_data *damage, int32_t collision_surface_index)
{
    halo::physics::BreakableSurfaces::breakable_surface_shatter(breakable_surface_index, damage, collision_surface_index);
}

int32_t bsp2d_node_find_leaf(int32_t node_index, TagReflexive *bsp2d_nodes, real_point2d *point)
{
    return halo::physics::CollisionBsp::bsp2d_node_find_leaf(node_index, bsp2d_nodes, point);
}

uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point)
{
    return halo::physics::CollisionBsp::bsp3d_node_find_leaf(node_index, bsp, point);
}

uint8_t collision_bsp_query_pill_init(ModelCollisionGeometryBSP *bsp, collision_bsp_pill_result *result, real_point3d *origin, real_vector3d *delta, float radius, float max_fraction)
{
    return halo::physics::CollisionBsp::query_pill_init(bsp, result, origin, delta, radius, max_fraction);
}

uint8_t collision_bsp_query_pill_leaf_recursive(collision_bsp_pill_query *query, int32_t bsp2d_node_index)
{
    return halo::physics::CollisionBsp::query_pill_leaf_recursive(query, bsp2d_node_index);
}

uint8_t collision_bsp_query_pill_leaf_test_surface(collision_bsp_pill_query *query, int32_t surface_index)
{
    return halo::physics::CollisionBsp::query_pill_leaf_test_surface(query, surface_index);
}

uint8_t collision_bsp_query_pill_node_recursive(collision_bsp_pill_query *query, int32_t node_index)
{
    return halo::physics::CollisionBsp::query_pill_node_recursive(query, node_index);
}

uint8_t collision_bsp_query_segment_init(uint32_t flags, collision_bsp_segment_result *result, ModelCollisionGeometryBSP *bsp, int16_t breakable_surface_count, uint32_t *breakable_surfaces, real_point3d *origin, real_vector3d *delta, float max_fraction)
{
    return halo::physics::CollisionBsp::query_segment_init(flags, result, bsp, breakable_surface_count, breakable_surfaces, origin, delta, max_fraction);
}

uint8_t collision_bsp_query_segment_node_recursive(collision_bsp_segment_query *query, uint32_t node_index, float t_min, float t_max)
{
    return halo::physics::CollisionBsp::query_segment_node_recursive(query, node_index, t_min, t_max);
}

void collision_bsp_query_sphere_collect_geometry(collision_bsp_sphere_query *query, int32_t surface_index)
{
    halo::physics::CollisionBsp::query_sphere_collect_geometry(query, surface_index);
}

uint32_t collision_bsp_query_sphere_init(ModelCollisionGeometryBSP *bsp, int16_t breakable_surface_count, collision_bsp_sphere_result *result, uint32_t *breakable_surfaces, real_point3d *center, float radius)
{
    return halo::physics::CollisionBsp::query_sphere_init(bsp, breakable_surface_count, result, breakable_surfaces, center, radius);
}

void collision_bsp_query_sphere_leaf_edge_recursive(collision_bsp_sphere_query *query, int32_t bsp2d_node_index)
{
    halo::physics::CollisionBsp::query_sphere_leaf_edge_recursive(query, bsp2d_node_index);
}

void collision_bsp_query_sphere_node_recursive(collision_bsp_sphere_query *query, uint32_t node_index)
{
    halo::physics::CollisionBsp::query_sphere_node_recursive(query, node_index);
}

uint32_t collision_bsp_surface_clip_line_2d(collision_bsp_boundary_clip *clip, ModelCollisionGeometryBSP *bsp, int32_t surface_index, real_point2d *origin, real_vector2d *direction)
{
    return halo::physics::CollisionBsp::surface_clip_line_2d(clip, bsp, surface_index, origin, direction);
}

uint32_t collision_bsp_surface_closest_edge_point_2d(ModelCollisionGeometryBSP *bsp, int32_t surface_index, uint16_t axis, uint8_t sign, real_point2d *point, real_point2d *out_point)
{
    return halo::physics::CollisionBsp::surface_closest_edge_point_2d(bsp, surface_index, axis, sign, point, out_point);
}

int16_t collision_bsp_surface_get_vertices(ModelCollisionGeometryBSP *bsp, int32_t surface_index, real_point3d *out_vertices)
{
    return halo::physics::CollisionBsp::surface_get_vertices(bsp, surface_index, out_vertices);
}

real_point3d * collision_bsp_surface_solve_third_axis(ModelCollisionGeometryBSP *collision_bsp, int32_t surface_index, uint8_t component_sign, real_point3d *out, int32_t dominant_axis, const real_point2d *known)
{
    return halo::physics::CollisionBsp::surface_solve_third_axis(collision_bsp, surface_index, component_sign, out, dominant_axis, known);
}

uint8_t collision_bsp_surface_test_point_2d(ModelCollisionGeometryBSP *bsp, int16_t breakable_surface_count, uint32_t *breakable_surfaces, int32_t surface_index, int16_t axis, uint8_t sign, real_point2d *point)
{
    return halo::physics::CollisionBsp::surface_test_point_2d(bsp, breakable_surface_count, breakable_surfaces, surface_index, axis, sign, point);
}

int32_t collision_bsp_surface_test_point_leaf(ModelCollisionGeometryBSP *bsp, int32_t leaf_index, int16_t breakable_surface_count, uint32_t *breakable_surfaces, uint32_t plane_index, real_point3d *crossing_point, uint8_t two_sided)
{
    return halo::physics::CollisionBsp::surface_test_point_leaf(bsp, leaf_index, breakable_surface_count, breakable_surfaces, plane_index, crossing_point, two_sided);
}

uint8_t collision_bsp_surface_test_point_side_2d(ModelCollisionGeometryBSP *bsp, real_point2d *point, int32_t surface_index, int16_t axis, uint8_t sign)
{
    return halo::physics::CollisionBsp::surface_test_point_side_2d(bsp, point, surface_index, axis, sign);
}

int16_t model_collision_geometry_resolve_material_type(int16_t material_index, ModelCollisionGeometry *definition)
{
    return halo::physics::CollisionBsp::model_collision_geometry_resolve_material_type(material_index, definition);
}

void collision_gather_nearby_object_shapes(uint32_t flags, uint32_t start_object_index, real_point3d *origin, float radius, float x_offset, float y_offset, uint32_t exclude_object_index, physics_model *model)
{
    halo::physics::CollisionWorld::gather_nearby_object_shapes(flags, start_object_index, origin, radius, x_offset, y_offset, exclude_object_index, model);
}

uint8_t collision_test_movement_pill(uint32_t flags, real_point3d *origin, float radius, real_vector3d *delta, collision_result *result)
{
    return halo::physics::CollisionWorld::test_movement_pill(flags, origin, radius, delta, result);
}

uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object_index, collision_result *result)
{
    return halo::physics::CollisionWorld::test_movement_segment(flags, origin, delta, exclude_object_index, result);
}

uint8_t collision_test_movement_segment_between_points(real_point3d *origin, real_point3d *target, uint32_t flags, uint32_t exclude_object_index, collision_result *result)
{
    return halo::physics::CollisionWorld::test_movement_segment_between_points(origin, target, flags, exclude_object_index, result);
}

uint8_t object_collision_context_build(uint32_t object_index, object_collision_context *out_context)
{
    return halo::physics::CollisionWorld::context_build(object_index, out_context);
}

uint8_t object_collision_context_gather_sphere_shapes(object_collision_context *context, real_point3d *origin, float radius_scale, float margin, float thickness, physics_model *model)
{
    return halo::physics::CollisionWorld::context_gather_sphere_shapes(context, origin, radius_scale, margin, thickness, model);
}

uint8_t object_collision_context_test_pill(object_collision_context *context, real_point3d *origin, real_vector3d *delta, float radius_scale, object_node_collision_result *out_result)
{
    return halo::physics::CollisionWorld::context_test_pill(context, origin, delta, radius_scale, out_result);
}

uint32_t object_collision_context_test_point(object_collision_context *context, real_point3d *point)
{
    return halo::physics::CollisionWorld::context_test_point(context, point);
}

uint8_t object_collision_context_test_segment(object_collision_context *context, uint32_t flags, real_point3d *origin, real_vector3d *delta, object_node_collision_result *out_result)
{
    return halo::physics::CollisionWorld::context_test_segment(context, flags, origin, delta, out_result);
}

uint8_t object_collision_test_cluster_group(uint32_t flags, real_point3d *position, uint32_t exclude_object_index)
{
    return halo::physics::CollisionWorld::test_cluster_group(flags, position, exclude_object_index);
}

uint8_t object_collision_test_nearby_chain(uint32_t start_object_index, uint32_t type_mask, real_point3d *position, uint32_t exclude_object_index)
{
    return halo::physics::CollisionWorld::test_nearby_chain(start_object_index, type_mask, position, exclude_object_index);
}

uint8_t object_collision_test_ray_nearby_chain(uint32_t start_object_index, uint32_t type_mask, uint32_t test_flags, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object_index, collision_result *out_result)
{
    return halo::physics::CollisionWorld::test_ray_nearby_chain(start_object_index, type_mask, test_flags, origin, delta, exclude_object_index, out_result);
}

uint8_t object_physics_add_mass_point_shapes(float x_offset, float y_offset, object_physics_context *context, int16_t *model_counts)
{
    return halo::physics::ObjectPhysics::add_mass_point_shapes(x_offset, y_offset, context, model_counts);
}

void object_physics_blend_friction_axes(int16_t friction_type, float parallel_scale, float perpendicular_scale, float *friction, real_vector3d *forward, real_vector3d *up)
{
    halo::physics::ObjectPhysics::blend_friction_axes(friction_type, parallel_scale, perpendicular_scale, friction, forward, up);
}

uint8_t object_physics_check_impact_damage(uint32_t *self_object_index, uint32_t candidate_object_index)
{
    return halo::physics::ObjectPhysics::check_impact_damage(self_object_index, candidate_object_index);
}

void object_physics_compute_mass_point_forces(object_physics_context *context, powered_mass_point_state *powered_states, uint32_t mass_points_address, real_vector3d *out_force, real_vector3d *out_torque)
{
    halo::physics::ObjectPhysics::compute_mass_point_forces(context, powered_states, mass_points_address, out_force, out_torque);
}

uint8_t object_physics_context_build(uint32_t object_index, object_physics_context *out_context)
{
    return halo::physics::ObjectPhysics::context_build(object_index, out_context);
}

void object_physics_handle_nearby_object_impacts(uint32_t object_index)
{
    halo::physics::ObjectPhysics::handle_nearby_object_impacts(object_index);
}

void object_physics_integrate_and_test_at_rest(object_physics_context *context, mass_point_state *mass_point_states, real_vector3d *torque, real_vector3d *force)
{
    halo::physics::ObjectPhysics::integrate_and_test_at_rest(context, mass_point_states, torque, force);
}

void object_physics_mass_point_resolve_ground_contact(uint32_t exclude_object_index, mass_point_state *mass_point, PhysicsMassPoint *definition)
{
    halo::physics::ObjectPhysics::mass_point_resolve_ground_contact(exclude_object_index, mass_point, definition);
}

void object_physics_mass_point_update_orientation(real_vector3d *axis, real_vector3d *up, real_vector3d *forward, real_vector3d *fallback_forward, real_vector3d *fallback_up)
{
    halo::physics::ObjectPhysics::mass_point_update_orientation(axis, up, forward, fallback_forward, fallback_up);
}

uint8_t object_physics_resolve_mass_point_overlap(object_physics_context *self, object_physics_context *other)
{
    return halo::physics::ObjectPhysics::resolve_mass_point_overlap(self, other);
}

uint8_t object_physics_test_point_against_mass_points(object_physics_context *context, real_point3d *world_point, int16_t *out_index)
{
    return halo::physics::ObjectPhysics::test_point_against_mass_points(context, world_point, out_index);
}

uint8_t object_physics_test_ray_against_mass_points(real_point3d *world_origin, real_vector3d *world_direction, object_physics_ray_result *out_result, object_physics_context *context)
{
    return halo::physics::ObjectPhysics::test_ray_against_mass_points(world_origin, world_direction, out_result, context);
}

void object_physics_tick(uint32_t object_index, powered_mass_point_state *powered_states, uint32_t mass_points, real_vector3d *extra_force, real_vector3d *extra_torque)
{
    halo::physics::ObjectPhysics::tick(object_index, powered_states, mass_points, extra_force, extra_torque);
}

void object_physics_tick_single_pass(uint32_t object_index, powered_mass_point_state *powered_states, mass_point_state *mass_point_states, real_vector3d *extra_force, real_vector3d *extra_torque)
{
    halo::physics::ObjectPhysics::tick_single_pass(object_index, powered_states, mass_point_states, extra_force, extra_torque);
}

void physics_clamp_value_to_spring_range(float *value, physics_scalar_rates *rates, float step)
{
    halo::physics::PhysicsMotion::clamp_value_to_spring_range(value, rates, step);
}

int16_t physics_resolve_material_type(uint32_t object_index, int16_t vertex_slot)
{
    return halo::physics::PhysicsMotion::resolve_material_type(object_index, vertex_slot);
}

void physics_scalar_advance_and_wrap(physics_scalar_range *range, float *value, uint8_t wrap, float delta)
{
    halo::physics::PhysicsMotion::scalar_advance_and_wrap(range, value, wrap, delta);
}

float physics_scalar_approach_direction(physics_scalar_range *range, float value, uint8_t wrap, float target)
{
    return halo::physics::PhysicsMotion::scalar_approach_direction(range, value, wrap, target);
}

uint8_t physics_scalar_move_toward_target(physics_scalar_range *range, float *value, uint8_t wrap, float target, float rate)
{
    return halo::physics::PhysicsMotion::scalar_move_toward_target(range, value, wrap, target, rate);
}

uint8_t physics_scalar_step_to_target_clamped(physics_scalar_rates *rates, float *value, float target, float step)
{
    return halo::physics::PhysicsMotion::scalar_step_to_target_clamped(rates, value, target, step);
}

void point_physics_interpolate(PointPhysics *out, const PointPhysics *from, const PointPhysics *to, float fraction)
{
    halo::physics::PhysicsMotion::interpolate(out, from, to, fraction);
}

uint32_t point_physics_tick(real_vector3d *velocity, uint32_t flags_arg, PointPhysics *definition, bsp_leaf_reference *out_leaf, uint32_t unused_param_4, real_point3d *position, real_vector3d *wind, real_vector3d *out_normal, int16_t *out_material_type, real radius, real dt)
{
    return halo::physics::PhysicsMotion::tick(velocity, flags_arg, definition, out_leaf, unused_param_4, position, wind, out_normal, out_material_type, radius, dt);
}

void vector3d_project_onto_direction(real_vector3d *out, const real_vector3d *axis, const real_vector3d *v)
{
    halo::physics::PhysicsMotion::vector3d_project_onto_direction(out, axis, v);
}

uint8_t physics_model_build_from_sphere_query(uint32_t flags, real_point3d *center, float radius, float x_offset, float y_offset, uint32_t exclude_object_index, physics_model *model)
{
    return halo::physics::PhysicsModelOps::model_build_from_sphere_query(flags, center, radius, x_offset, y_offset, exclude_object_index, model);
}

int16_t physics_model_slide_along_contacts(real_point3d *start_position, real_vector3d *delta, physics_model *model, real_point3d *out_position, real_vector3d *out_velocity, int16_t max_contacts, physics_model_contact *contacts)
{
    return halo::physics::PhysicsModelOps::model_slide_along_contacts(start_position, delta, model, out_position, out_velocity, max_contacts, contacts);
}

uint8_t physics_point_find_clear_position(uint32_t flags, real_point3d *current_position, float sample_radius, float x_margin, float y_margin, uint32_t exclude_object_index, real_point3d *out_position)
{
    return halo::physics::PhysicsModelOps::point_find_clear_position(flags, current_position, sample_radius, x_margin, y_margin, exclude_object_index, out_position);
}

uint8_t physics_point_refresh_leaf(real_point3d *point, float radius)
{
    return halo::physics::PhysicsModelOps::point_refresh_leaf(point, radius);
}

void physics_point_walk_toward_target(physics_point_walk_state *state, real_point3d *start_position, uint32_t flags, real_vector3d *step_direction, uint32_t exclude_object_index)
{
    halo::physics::PhysicsModelOps::point_walk_toward_target(state, start_position, flags, step_direction, exclude_object_index);
}

void physics_shape_add_edge_proxy(int32_t edge_index, ModelCollisionGeometryBSP *bsp, real_matrix4x3 *matrix, float height_offset, float thickness, int32_t object_index, physics_model *model)
{
    halo::physics::PhysicsModelOps::shape_add_edge_proxy(edge_index, bsp, matrix, height_offset, thickness, object_index, model);
}

void physics_shape_add_surface_proxy(ModelCollisionGeometryBSP *bsp, float *moving_frame, int32_t surface_index, float margin, float thickness, int32_t object_index, physics_model *model)
{
    halo::physics::PhysicsModelOps::shape_add_surface_proxy(bsp, moving_frame, surface_index, margin, thickness, object_index, model);
}

void physics_shape_add_vertex_proxy(ModelCollisionGeometryBSP *bsp, uint32_t vertex_index, uint32_t object_index, real_matrix4x3 *matrix, float height_offset, float radius, physics_model *model)
{
    halo::physics::PhysicsModelOps::shape_add_vertex_proxy(bsp, vertex_index, object_index, matrix, height_offset, radius, model);
}

void physics_shape_build_proxies_from_query(collision_bsp_sphere_result *result, real_matrix4x3 *matrix, ModelCollisionGeometryBSP *bsp, float margin, float thickness, int32_t object_index, physics_model *model)
{
    halo::physics::PhysicsModelOps::shape_build_proxies_from_query(result, matrix, bsp, margin, thickness, object_index, model);
}

void physics_shape_edge_to_pill_and_quad(physics_model *model, real_point3d *near_vertex, real_vector3d *edge_dir, float height_offset, float thickness, uint32_t object_index, int32_t surface_index, uint8_t surface_flags, int8_t breakable_surface_index, int16_t material_type)
{
    halo::physics::PhysicsModelOps::shape_edge_to_pill_and_quad(model, near_vertex, edge_dir, height_offset, thickness, object_index, surface_index, surface_flags, breakable_surface_index, material_type);
}

uint8_t physics_shape_pill_sweep_test_point(real_point3d *near_vertex, real_vector3d *delta, real_point3d *origin, real_vector3d *edge_dir, float radius, float *out_t, float *out_edge_fraction)
{
    return halo::physics::PhysicsModelOps::shape_pill_sweep_test_point(near_vertex, delta, origin, edge_dir, radius, out_t, out_edge_fraction);
}

uint8_t physics_shape_pill_test_point(real_point3d *point, physics_model_pill *pill, real_plane3d *out_normal, float *out_depth)
{
    return halo::physics::PhysicsModelOps::shape_pill_test_point(point, pill, out_normal, out_depth);
}

uint8_t physics_shape_pill_test_ray(real_vector3d *delta, real_point3d *origin, real_plane3d *out_plane, physics_model_pill *pill, float *out_t)
{
    return halo::physics::PhysicsModelOps::shape_pill_test_ray(delta, origin, out_plane, pill, out_t);
}

uint8_t physics_shape_polygon_test_point(physics_model_shape *shape, real_point3d *point, float *out_depth, real_plane3d *out_normal)
{
    return halo::physics::PhysicsModelOps::shape_polygon_test_point(shape, point, out_depth, out_normal);
}

uint8_t physics_shape_polygon_test_ray(real_point3d *origin, physics_model_shape *shape, real_vector3d *delta, float *out_t, real_plane3d *out_plane)
{
    return halo::physics::PhysicsModelOps::shape_polygon_test_ray(origin, shape, delta, out_t, out_plane);
}

uint8_t physics_shape_sphere_sweep_test_ray(real_point3d *point, real_point3d *origin, real_vector3d *delta, float *out_t, float radius)
{
    return halo::physics::PhysicsModelOps::shape_sphere_sweep_test_ray(point, origin, delta, out_t, radius);
}

uint8_t physics_shape_sphere_test_point(real_point3d *point, physics_model_sphere *sphere, real_plane3d *out_normal, float *out_depth)
{
    return halo::physics::PhysicsModelOps::shape_sphere_test_point(point, sphere, out_normal, out_depth);
}

uint8_t physics_shape_sphere_test_ray(real_point3d *origin, real_vector3d *delta, physics_model_sphere *sphere, real_plane3d *out_plane, float *out_t)
{
    return halo::physics::PhysicsModelOps::shape_sphere_test_ray(origin, delta, sphere, out_plane, out_t);
}

void physics_shape_surface_to_polygon(int16_t vertex_count, real_point3d *vertices, real_plane3d *plane, float margin, float thickness, uint32_t object_index, int32_t surface_index, uint8_t surface_flags, int8_t breakable_surface_index, int16_t material_type, physics_model *model)
{
    halo::physics::PhysicsModelOps::shape_surface_to_polygon(vertex_count, vertices, plane, margin, thickness, object_index, surface_index, surface_flags, breakable_surface_index, material_type, model);
}

uint32_t physics_shape_test_point(physics_model *model, real_point3d *point, physics_model_contact *out_contact)
{
    return halo::physics::PhysicsModelOps::shape_test_point(model, point, out_contact);
}

uint32_t physics_shape_test_ray(physics_model *model, real_point3d *origin, real_vector3d *delta, physics_model_contact *out_contact)
{
    return halo::physics::PhysicsModelOps::shape_test_ray(model, origin, delta, out_contact);
}

void physics_shape_vertex_to_sphere(physics_model *model, real_point3d *vertex, int16_t material_type, float height_offset, float radius, uint32_t object_index, int32_t surface_index, uint8_t surface_flags, int8_t breakable_surface_index)
{
    halo::physics::PhysicsModelOps::shape_vertex_to_sphere(model, vertex, material_type, height_offset, radius, object_index, surface_index, surface_flags, breakable_surface_index);
}

int16_t physics_sweep_capsule_step(real_point3d *origin, real_vector3d *delta, real_vector3d *out_velocity, uint32_t exclude_object_index, uint32_t flags, float pill_height, float pill_radius, real_point3d *out_position, int16_t max_contacts, physics_model_contact *contacts)
{
    return halo::physics::PhysicsModelOps::sweep_capsule_step(origin, delta, out_velocity, exclude_object_index, flags, pill_height, pill_radius, out_position, max_contacts, contacts);
}

}
