#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::physics {

/**
 * World-level movement tests: structure BSP, nearby objects and water surfaces.
 * Stateless service class: the functions are static members, the state they act on lives in the engine globals.
 */
struct CollisionWorld {
    static void gather_nearby_object_shapes(uint32_t flags, uint32_t start_object_index, real_point3d *origin, float radius, float x_offset, float y_offset, uint32_t exclude_object_index, physics_model *model);
    static uint8_t test_movement_pill(uint32_t flags, real_point3d *origin, float radius, real_vector3d *delta, collision_result *result);
    static uint8_t test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object_index, collision_result *result);
    static uint8_t test_movement_segment_between_points(real_point3d *origin, real_point3d *target, uint32_t flags, uint32_t exclude_object_index, collision_result *result);
    static uint8_t context_build(uint32_t object_index, object_collision_context *out_context);
    static uint8_t context_gather_sphere_shapes(object_collision_context *context, real_point3d *origin, float radius_scale, float margin, float thickness, physics_model *model);
    static uint8_t context_test_pill(object_collision_context *context, real_point3d *origin, real_vector3d *delta, float radius_scale, object_node_collision_result *out_result);
    static uint32_t context_test_point(object_collision_context *context, real_point3d *point);
    static uint8_t context_test_segment(object_collision_context *context, uint32_t flags, real_point3d *origin, real_vector3d *delta, object_node_collision_result *out_result);
    static uint8_t test_cluster_group(uint32_t flags, real_point3d *position, uint32_t exclude_object_index);
    static uint8_t test_nearby_chain(uint32_t start_object_index, uint32_t type_mask, real_point3d *position, uint32_t exclude_object_index);
    static uint8_t test_ray_nearby_chain(uint32_t start_object_index, uint32_t type_mask, uint32_t test_flags, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object_index, collision_result *out_result);
};

}
