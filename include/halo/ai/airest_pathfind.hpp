#pragma once

#include "halo/ai/airest_types.hpp"

namespace halo::ai {

/**
 * Non-owning view of a path-find context. Members run the A* search, maintain its open heap and
 * post-process the resulting waypoints.
 */
class PathFinder {
public:
    path_find_context * ptr;
    explicit constexpr PathFinder(path_find_context * p) : ptr(p) {}

    uint8_t navigate_around_obstacles(int16_t count, path_find_waypoint *waypoints, int16_t *out_count, path_find_waypoint *out_waypoints, uint8_t *out_valid);
    uint8_t compute_heuristic(uint32_t vertex_id, real_point3d *point, float *out_distance, float *out_secondary, real_vector3d *out_direction);
    void context_init(const path_find_request *request, uint32_t second_param);
    uint8_t find_unobstructed_ancestor(uint32_t vertex_id, real_point3d *point, uint8_t *out_used_start, real_point3d *out_position);
    int16_t hash_lookup_vertex(uint32_t vertex_id);
    void heap_push(int16_t node, int16_t key);
    void heap_sift_down(int16_t index);
    void heap_sift_up(int16_t index);
    uint8_t push_start_node();
    uint8_t reconstruct_path(uint8_t *out_result);
    uint8_t run();
    float score_avoidance_penalty(const real_point3d *segment_start, const real_point3d *segment_end, float *out_distance);
    void set_avoid_sphere(const real_point3d *position, float avoid_radius, datum_index avoid_object_index, float avoid_weight);
    void set_goal(const real_point3d *position, uint32_t goal_vertex_id, float goal_cost);
    void simplify_waypoints(int16_t count, path_find_waypoint *waypoints, int16_t *out_count, path_find_waypoint *out_waypoints, uint8_t *out_valid);
};

/**
 * Stateless geometric helpers of the path finder that work on the structure BSP rather than on a search
 * context: boundary tracing and segment tests.
 */
class PathFindGeometry {
public:
    static int16_t gather_adjacent_edges(void *context, int32_t vertex_id, path_find_adjacent_edge *out_edges);
    static uint8_t heights_are_close(ScenarioStructureBSP *structure_bsp, real_point2d *point, int32_t surface_a, int32_t surface_b);
    static uint8_t test_direct_reachability(const real_point3d *point_a, const real_point3d *point_b, real_point3d *out_position, void *context, uint8_t *out_success);
    static uint8_t test_segment_unobstructed(void *map, real_point3d *point_a, uint8_t ignore_permission, int32_t surface_a, real_point3d *point_b, int32_t surface_b, float radius, uint8_t flags, path_find_boundary_crossing *out_result);
    static uint8_t trace_bsp_boundary(void *map, uint8_t ignore_permission, real_point3d *start, int32_t start_surface, real_point3d *end, int32_t target_surface, path_find_boundary_crossing *out_result);
    static uint8_t trace_cluster_boundary(void *map, int32_t edge_index, real_point2d *origin, float radius, uint8_t side, uint8_t ignore_permission, real_point2d *out_point);
    static uint8_t trace_cluster_boundary_from_vertex(void *context, uint8_t ignore_permission, real_point2d *point, int32_t start_index, real_vector2d *direction, float max_distance, path_find_boundary_trace_result *out);
    static uint8_t validate_and_record_goal(ai_path_candidate_goal *candidate, void *context, uint32_t point_b, uint32_t unused_c, const real_point3d *position);
    static float vertex_distance(ScenarioStructureBSP *structure_bsp, int32_t surface, real_point3d *point_a, real_point3d *out_point);
};

}
