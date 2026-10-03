#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::physics {

/**
 * Recursive sphere, segment and pill queries over a collision BSP and the 2D surface helpers.
 * Stateless service class: the functions are static members, the state they act on lives in the engine globals.
 */
struct CollisionBsp {
    static int32_t bsp2d_node_find_leaf(int32_t node_index, TagReflexive *bsp2d_nodes, real_point2d *point);
    static uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp, real_point3d *point);
    static uint8_t query_pill_init(ModelCollisionGeometryBSP *bsp, collision_bsp_pill_result *result, real_point3d *origin, real_vector3d *delta, float radius, float max_fraction);
    static uint8_t query_pill_leaf_recursive(collision_bsp_pill_query *query, int32_t bsp2d_node_index);
    static uint8_t query_pill_leaf_test_surface(collision_bsp_pill_query *query, int32_t surface_index);
    static uint8_t query_pill_node_recursive(collision_bsp_pill_query *query, int32_t node_index);
    static uint8_t query_segment_init(uint32_t flags, collision_bsp_segment_result *result, ModelCollisionGeometryBSP *bsp, int16_t breakable_surface_count, uint32_t *breakable_surfaces, real_point3d *origin, real_vector3d *delta, float max_fraction);
    static uint8_t query_segment_node_recursive(collision_bsp_segment_query *query, uint32_t node_index, float t_min, float t_max);
    static void query_sphere_collect_geometry(collision_bsp_sphere_query *query, int32_t surface_index);
    static uint32_t query_sphere_init(ModelCollisionGeometryBSP *bsp, int16_t breakable_surface_count, collision_bsp_sphere_result *result, uint32_t *breakable_surfaces, real_point3d *center, float radius);
    static void query_sphere_leaf_edge_recursive(collision_bsp_sphere_query *query, int32_t bsp2d_node_index);
    static void query_sphere_node_recursive(collision_bsp_sphere_query *query, uint32_t node_index);
    static uint32_t surface_clip_line_2d(collision_bsp_boundary_clip *clip, ModelCollisionGeometryBSP *bsp, int32_t surface_index, real_point2d *origin, real_vector2d *direction);
    static uint32_t surface_closest_edge_point_2d(ModelCollisionGeometryBSP *bsp, int32_t surface_index, uint16_t axis, uint8_t sign, real_point2d *point, real_point2d *out_point);
    static int16_t surface_get_vertices(ModelCollisionGeometryBSP *bsp, int32_t surface_index, real_point3d *out_vertices);
    static real_point3d * surface_solve_third_axis(ModelCollisionGeometryBSP *collision_bsp, int32_t surface_index, uint8_t component_sign, real_point3d *out, int32_t dominant_axis, const real_point2d *known);
    static uint8_t surface_test_point_2d(ModelCollisionGeometryBSP *bsp, int16_t breakable_surface_count, uint32_t *breakable_surfaces, int32_t surface_index, int16_t axis, uint8_t sign, real_point2d *point);
    static int32_t surface_test_point_leaf(ModelCollisionGeometryBSP *bsp, int32_t leaf_index, int16_t breakable_surface_count, uint32_t *breakable_surfaces, uint32_t plane_index, real_point3d *crossing_point, uint8_t two_sided);
    static uint8_t surface_test_point_side_2d(ModelCollisionGeometryBSP *bsp, real_point2d *point, int32_t surface_index, int16_t axis, uint8_t sign);
    static int16_t model_collision_geometry_resolve_material_type(int16_t material_index, ModelCollisionGeometry *definition);
};

}
