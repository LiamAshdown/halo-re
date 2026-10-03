#pragma once

#include "halo/ai/airest_types.hpp"

namespace halo::ai {

/**
 * Non-owning view of an obstacle-avoiding point search context (graph search over obstacle tangent
 * points). Members expand nodes, maintain the heap and run the search.
 */
class AiSearch {
public:
    ai_search_context * ptr;
    explicit constexpr AiSearch(ai_search_context * p) : ptr(p) {}

    int16_t add_node(int16_t parent, real_point2d *position, int32_t surface_index, int16_t point_id, uint8_t side, float base_cost);
    void context_init(uint8_t ignores_glass, uint32_t search_radius_bits, ai_search_obstacle_list *obstacles, real_point2d *origin, uint32_t structure_bsp, real_point2d *position, int32_t surface_index, uint32_t origin_surface_index, uint8_t final_leg, uint8_t ignore_flagged_obstacles);
    void expand_point_neighbors(int16_t node_index, int16_t start_point_id);
    void heap_sift_down(int16_t index);
    void heap_sift_up(int16_t index);
    uint8_t run(uint8_t ignores_glass, ai_search_obstacle_list *obstacles, uint32_t search_radius_bits, real_point2d *position, int32_t surface_index, real_point2d *origin, uint32_t origin_surface_index, uint8_t final_leg, uint8_t ignore_flagged_obstacles);
    uint8_t step();
};

/**
 * Non-owning view of an obstacle list used by the point search. Members query and partition the obstacles.
 */
class ObstacleList {
public:
    ai_search_obstacle_list * ptr;
    explicit constexpr ObstacleList(ai_search_obstacle_list * p) : ptr(p) {}

    uint8_t append_obstacle(uint16_t flags, uint32_t object_index, real_point2d *position, float radius);
    void compute_point_tangents(int16_t point_index, real_point2d *position, real_vector2d *edge_neg, float radius, real_vector2d *out_a, real *out_b);
    int16_t find_covering_point(real_point2d *position, int16_t exclude_index, float extra_radius);
    uint8_t find_nearest_visible_point(int16_t exclude_index, real_point2d *origin, real_vector2d *direction, float radius, float max_distance, uint8_t require_unflagged, ai_search_nearest_point_result *out_result);
    void flood_fill_group(float radius, uint32_t *out_bitmask, int16_t start_index);
    void gather_obstacles(real_point3d *center, float radius, real_vector3d *direction, uint32_t self_object_a, uint32_t self_object_b);
    void partition_into_groups(float radius);
};

/**
 * Stateless 2D geometry helpers of the point search (tangent and portal crossings, edge costs).
 */
class AiSearchGeometry {
public:
    static uint8_t choose_shorter_corner(real_point2d *p, real_point2d *corner_a, real_point2d *q, real_point2d *corner_b, real_point2d *r, real_point2d *out_point);
    static uint8_t evaluate_edge_cost(void *context, uint8_t ignore_permission, ai_search_obstacle_list *obstacle_list, int16_t exclude_index, real_point2d *point, int32_t start_surface_index, float distance, float base_cost, uint8_t skip_direct, uint8_t apply_offset, uint8_t require_unflagged, ai_search_edge_result *out_result, real_vector2d *direction);
    static void find_circle_portal_crossing(real_point2d *center, real_point2d *portal, real_point2d *out_point, real_point2d *fallback_reference, float radius);
    static void find_circle_tangent_point(real_point2d *center, real_point2d *target, real_point2d *out_point, float radius, uint8_t side);
};

}
