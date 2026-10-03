#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::physics {

/**
 * Scratch physics model of sphere, pill and polygon proxies built from a query, and the tests run against it.
 * Stateless service class: the functions are static members, the state they act on lives in the engine globals.
 */
struct PhysicsModelOps {
    static uint8_t model_build_from_sphere_query(uint32_t flags, real_point3d *center, float radius, float x_offset, float y_offset, uint32_t exclude_object_index, physics_model *model);
    static int16_t model_slide_along_contacts(real_point3d *start_position, real_vector3d *delta, physics_model *model, real_point3d *out_position, real_vector3d *out_velocity, int16_t max_contacts, physics_model_contact *contacts);
    static uint8_t point_find_clear_position(uint32_t flags, real_point3d *current_position, float sample_radius, float x_margin, float y_margin, uint32_t exclude_object_index, real_point3d *out_position);
    static uint8_t point_refresh_leaf(real_point3d *point, float radius);
    static void point_walk_toward_target(physics_point_walk_state *state, real_point3d *start_position, uint32_t flags, real_vector3d *step_direction, uint32_t exclude_object_index);
    static void shape_add_edge_proxy(int32_t edge_index, ModelCollisionGeometryBSP *bsp, real_matrix4x3 *matrix, float height_offset, float thickness, int32_t object_index, physics_model *model);
    static void shape_add_surface_proxy(ModelCollisionGeometryBSP *bsp, float *moving_frame, int32_t surface_index, float margin, float thickness, int32_t object_index, physics_model *model);
    static void shape_add_vertex_proxy(ModelCollisionGeometryBSP *bsp, uint32_t vertex_index, uint32_t object_index, real_matrix4x3 *matrix, float height_offset, float radius, physics_model *model);
    static void shape_build_proxies_from_query(collision_bsp_sphere_result *result, real_matrix4x3 *matrix, ModelCollisionGeometryBSP *bsp, float margin, float thickness, int32_t object_index, physics_model *model);
    static void shape_edge_to_pill_and_quad(physics_model *model, real_point3d *near_vertex, real_vector3d *edge_dir, float height_offset, float thickness, uint32_t object_index, int32_t surface_index, uint8_t surface_flags, int8_t breakable_surface_index, int16_t material_type);
    static uint8_t shape_pill_sweep_test_point(real_point3d *near_vertex, real_vector3d *delta, real_point3d *origin, real_vector3d *edge_dir, float radius, float *out_t, float *out_edge_fraction);
    static uint8_t shape_pill_test_point(real_point3d *point, physics_model_pill *pill, real_plane3d *out_normal, float *out_depth);
    static uint8_t shape_pill_test_ray(real_vector3d *delta, real_point3d *origin, real_plane3d *out_plane, physics_model_pill *pill, float *out_t);
    static uint8_t shape_polygon_test_point(physics_model_shape *shape, real_point3d *point, float *out_depth, real_plane3d *out_normal);
    static uint8_t shape_polygon_test_ray(real_point3d *origin, physics_model_shape *shape, real_vector3d *delta, float *out_t, real_plane3d *out_plane);
    static uint8_t shape_sphere_sweep_test_ray(real_point3d *point, real_point3d *origin, real_vector3d *delta, float *out_t, float radius);
    static uint8_t shape_sphere_test_point(real_point3d *point, physics_model_sphere *sphere, real_plane3d *out_normal, float *out_depth);
    static uint8_t shape_sphere_test_ray(real_point3d *origin, real_vector3d *delta, physics_model_sphere *sphere, real_plane3d *out_plane, float *out_t);
    static void shape_surface_to_polygon(int16_t vertex_count, real_point3d *vertices, real_plane3d *plane, float margin, float thickness, uint32_t object_index, int32_t surface_index, uint8_t surface_flags, int8_t breakable_surface_index, int16_t material_type, physics_model *model);
    static uint32_t shape_test_point(physics_model *model, real_point3d *point, physics_model_contact *out_contact);
    static uint32_t shape_test_ray(physics_model *model, real_point3d *origin, real_vector3d *delta, physics_model_contact *out_contact);
    static void shape_vertex_to_sphere(physics_model *model, real_point3d *vertex, int16_t material_type, float height_offset, float radius, uint32_t object_index, int32_t surface_index, uint8_t surface_flags, int8_t breakable_surface_index);
    static int16_t sweep_capsule_step(real_point3d *origin, real_vector3d *delta, real_vector3d *out_velocity, uint32_t exclude_object_index, uint32_t flags, float pill_height, float pill_radius, real_point3d *out_position, int16_t max_contacts, physics_model_contact *contacts);
};

}
