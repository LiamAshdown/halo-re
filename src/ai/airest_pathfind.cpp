#include "halo/core/bit_cast.hpp"
#include "halo/ai/airest_pathfind.hpp"

#include <stdint.h>
#include <string.h>
#include "halo/math/api.hpp"
#include "halo/physics/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/ai/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"
#include "halo/physics/vars.hpp"
#include "halo/units/vars.hpp"
#include "halo/core/libm.hpp"
#include "halo/core/x87.hpp"
#include "halo/units/api.hpp"

static auto &global_origin3d_pointer = halo::link::ref<real_point3d *>(halo::ai::vars().global_origin3d_pointer);
static auto &breakable_surface_state = halo::link::ref<uint8_t *>(halo::physics::vars().breakable_surface_state);
static auto &breakable_surface_state_typed = reinterpret_cast<breakable_surface_globals *&>(breakable_surface_state);
static auto &global_zero_vector3d_pointer = halo::link::ref<real_point3d *>(halo::units::vars().global_zero_vector3d_pointer);

namespace halo::ai {

namespace {

static_assert(offsetof(ModelCollisionGeometryBSP, planes) == 0x0c);
static_assert(offsetof(ModelCollisionGeometryBSP, surfaces) == 0x3c);
static_assert(offsetof(ModelCollisionGeometryBSP, edges) == 0x48);
static_assert(offsetof(ModelCollisionGeometryBSP, vertices) == 0x54);
static_assert(sizeof(ModelCollisionGeometryBSPSurface) == 12);
static_assert(sizeof(ModelCollisionGeometryBSPEdge) == 0x18);
static_assert(sizeof(ModelCollisionGeometryBSPVertex) == 16);

/** Word indices into a collision bsp edge record viewed as int32_t[6]. */
enum bsp_edge_word : int32_t {
    k_edge_start_vertex = 0,
    k_edge_end_vertex = 1,
    k_edge_forward_edge = 2,
    k_edge_reverse_edge = 3,
    k_edge_left_surface = 4,
    k_edge_right_surface = 5,
};

constexpr uint32_t k_bsp_plane_index_mask = 0x7fffffff;

/** The vertex hash of a path_find_context: 512 buckets of 8 entries, probed linearly across all 4096 slots. */
constexpr uint32_t k_vertex_hash_bucket_mask = 0x1ff;
constexpr uint32_t k_vertex_hash_slot_mask = 0xfff;
constexpr int32_t k_vertex_hash_slot_count = 0x1000;

/** Capacity of the node array and of the (one-based) open heap of a path_find_context. */
constexpr int32_t k_path_node_capacity = 0x400;

template <typename T>
inline T *bsp_reflexive_at(const TagReflexive &reflexive, int32_t index)
{
    return reinterpret_cast<T *>(static_cast<uintptr_t>(reflexive.pointer)) + index;
}

inline ModelCollisionGeometryBSPSurface *bsp_surface(const void *bsp, int32_t index)
{
    return bsp_reflexive_at<ModelCollisionGeometryBSPSurface>(static_cast<const ModelCollisionGeometryBSP *>(bsp)->surfaces, index);
}

inline int32_t *bsp_edge(const void *bsp, int32_t index)
{
    return reinterpret_cast<int32_t *>(bsp_reflexive_at<ModelCollisionGeometryBSPEdge>(static_cast<const ModelCollisionGeometryBSP *>(bsp)->edges, index));
}

inline float *bsp_vertex(const void *bsp, int32_t index)
{
    return reinterpret_cast<float *>(bsp_reflexive_at<ModelCollisionGeometryBSPVertex>(static_cast<const ModelCollisionGeometryBSP *>(bsp)->vertices, index));
}

inline real_plane3d *bsp_surface_plane(const void *bsp, int32_t surface_index)
{
    return reinterpret_cast<real_plane3d *>(bsp_reflexive_at<ModelCollisionGeometryBSPPlane>(
        static_cast<const ModelCollisionGeometryBSP *>(bsp)->planes, static_cast<int32_t>(bsp_surface(bsp, surface_index)->plane & k_bsp_plane_index_mask)));
}

/** The per-actor obstacle and search cache that path_find_context.obstacle_cache points at: a valid flag, the number of waypoints already searched, and one obstacle list and search context per waypoint. */
struct path_find_search_slot {
    ai_search_context search;
    uint8_t padding[0x1534 - sizeof(ai_search_context)];
};
static_assert(sizeof(path_find_search_slot) == 0x1534);
static_assert(sizeof(ai_search_obstacle_list) == 0xa08);

struct path_find_obstacle_cache {
    uint8_t unknown_00[0x10588];
    uint8_t valid;
    uint8_t unknown_10589;
    int16_t searched_count;
    ai_search_obstacle_list obstacle_lists[4];
    path_find_search_slot searches[4];
};
static_assert(offsetof(path_find_obstacle_cache, valid) == 0x10588 && offsetof(path_find_obstacle_cache, searched_count) == 0x1058a);
static_assert(offsetof(path_find_obstacle_cache, obstacle_lists) == 0x1058c && offsetof(path_find_obstacle_cache, searches) == 0x12dac);
static_assert(offsetof(path_find_request, avoid_object_index) == 0x34);


static_assert(offsetof(ScenarioStructureBSP, collision_bsp) == 0xb0 && offsetof(ScenarioStructureBSP, pathfinding_surfaces) == 0x1e4);

inline ModelCollisionGeometryBSP *map_collision_bsp(const void *map)
{
    return reinterpret_cast<ModelCollisionGeometryBSP *>(static_cast<uintptr_t>(static_cast<const ScenarioStructureBSP *>(map)->collision_bsp.pointer));
}

inline uint8_t *map_surface_permissions(const void *map)
{
    return reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(static_cast<const ScenarioStructureBSP *>(map)->pathfinding_surfaces.pointer));
}
}

namespace {

static real_plane3d *ai_navigate_surface_plane(ModelCollisionGeometryBSP *bsp, int32_t surface)
{
    return bsp_surface_plane(bsp, surface);
}

}

/**
 * Behaviour of ai navigate around obstacles, moved unchanged from the original free function.
 *
 * @address 0x43be90
 */
uint8_t PathFinder::navigate_around_obstacles(int16_t count, path_find_waypoint *waypoints, int16_t *out_count, path_find_waypoint *out_waypoints, uint8_t *out_valid)
{
    path_find_context * context = ptr;
    path_find_request *request = (path_find_request *)context;
    ModelCollisionGeometryBSP *collision_bsp = halo::physics::globals().structure_collision_bsp;
    float radius = (request->pathfinding_radius > 0.2f) ? request->pathfinding_radius : 0.2f;
    path_find_obstacle_cache *cache = (path_find_obstacle_cache *)(uintptr_t)context->obstacle_cache;
    ai_search_obstacle_list local_obstacles;
    ai_search_context local_search;
    path_find_waypoint path[0x80];
    real_point3d previous;
    int32_t previous_surface = 0;
    int16_t i;

    if (cache != 0 && cache->valid == 0) {
        cache->searched_count = 0;
    }
    for (i = 0; i < count; i++) {
        ai_search_obstacle_list *obstacles = &local_obstacles;
        ai_search_context *search = &local_search;
        uint8_t last = (uint8_t)(i == count - 1 && *out_valid != 0);
        real_point3d *from;
        int32_t from_surface;
        int32_t surface = waypoints[i].surface_index;
        real_point3d *to = &waypoints[i].position;
        real_vector3d direction;
        uint8_t found;
        uint8_t overflow = 0;
        int16_t length = 0;
        int16_t index;

        if (i > 0) {
            from = &previous;
            from_surface = previous_surface;
        } else {
            from = &context->start_position;
            from_surface = (int32_t)context->start_vertex_id;
        }
        direction.i = to->x - from->x;
        direction.j = to->y - from->y;
        direction.k = to->z - from->z;
        {
            float magnitude = (float)halo::libm::sqrt(direction.j * direction.j + direction.k * direction.k +
                direction.i * direction.i);

            if (!((float)halo::libm::fabs(magnitude) < 0.0001f)) {
                float scale = 1.0f / magnitude;

                direction.i *= scale;
                direction.j *= scale;
                direction.k *= scale;
            }
        }
        if (cache != 0) {
            obstacles = &cache->obstacle_lists[i];
            search = &cache->searches[i].search;
        }
        if (cache == 0 || cache->valid == 0 || !(i < cache->searched_count)) {
            obstacles->group_count = 0;
            obstacles->count = 0;
            obstacles->flagged_count = 0;
            halo::ai::ai_search_gather_obstacles(obstacles, from, 4.0f, &direction, request->exclude_object_index_a, request->exclude_object_index_b);
            if (request->have_avoid_sphere && obstacles->count != 0x80) {
                ai_search_obstacle *entry = &obstacles->obstacles[obstacles->count++];

                obstacles->flagged_count++;
                entry->flags = 1;
                entry->link = -1;
                entry->object_index = request->avoid_object_index;
                entry->position.x = request->avoid_position.x;
                entry->position.y = request->avoid_position.y;
                entry->radius = request->avoid_radius;
            }
            halo::ai::ai_search_partition_into_groups(obstacles, radius);
            if (cache != 0 && cache->valid == 0) {
                cache->searched_count++;
            }
        }

        halo::ai::ai_search_context_init(search, request->ignores_glass, *(uint32_t *)&radius, obstacles, (real_point2d *)to,
            (uint32_t)(uintptr_t)halo::scenario::globals().structure_bsp, (real_point2d *)from, from_surface, (uint32_t)surface, last, 0);
        while (halo::ai::ai_search_step(search) != 0) {
        }
        if (search->result_node != -1) {
            search->complete = 1;
        } else if (search->best_node != -1) {
            search->result_node = search->best_node;
        }
        found = (uint8_t)(search->result_node != -1);
        if (!found) {
            if (obstacles->flagged_count <= 0 ||
                !halo::ai::ai_search_run(search, request->ignores_glass, obstacles, *(uint32_t *)&radius, (real_point2d *)from,
                    from_surface, (real_point2d *)to, (uint32_t)surface, last, 1)) {
                return 0;
            }
        }

        if (search->complete) {
            previous = *to;
            previous_surface = surface;
        } else {
            ai_search_node *best = &search->nodes[search->result_node];

            previous_surface = halo::bit_cast<int32_t>(best->z);
            halo::math::decal_plane_solve_third_axis(&previous, 1, 2, ai_navigate_surface_plane(collision_bsp, previous_surface),
                best->position);
        }

        index = search->result_node;
        while (index != 0) {
            ai_search_node *node = &search->nodes[index];
            real_plane3d *plane = ai_navigate_surface_plane(collision_bsp, halo::bit_cast<int32_t>(node->z));
            path_find_waypoint *point = &path[length++];

            point->surface_index = halo::bit_cast<int32_t>(node->z);
            point->position.x = node->position.x;
            point->position.y = node->position.y;
            if ((float)halo::libm::fabs(plane->normal.k) < 0.0001f) {
                point->position.z = 0.0f;
            } else {
                point->position.z = (plane->d - node->position.x * plane->normal.i - plane->normal.j * node->position.y) /
                    plane->normal.k;
            }
            index = node->parent;
            if (length >= 0x80) {
                overflow = 1;
                break;
            }
        }
        {
            int16_t emitted = *out_count;

            while (--length >= 0) {
                if (emitted >= 4) {
                    overflow = 1;
                    break;
                }
                out_waypoints[emitted++] = path[length];
            }
            *out_count = emitted;
        }
        if (overflow) {
            *out_valid = 0;
            return 1;
        }
    }
    return 1;
}

/**
 * Behaviour of path find compute heuristic, moved unchanged from the original free function.
 *
 * @address 0x43a310
 */
uint8_t PathFinder::compute_heuristic(uint32_t vertex_id, real_point3d *point, float *out_distance, float *out_secondary, real_vector3d *out_direction)
{
    path_find_context * context = ptr;
    int16_t node_index;
    path_find_node *node;
    float dx, dy, dz;
    float leash;
    float secondary;

    node_index = halo::ai::path_find_hash_lookup_vertex(context, vertex_id);
    if (node_index == -1) {
        if (out_secondary != 0) {
            *out_secondary = 3.4028235e+38f;
        }
        if (out_direction != 0) {
            *out_direction = *(real_vector3d *)global_origin3d_pointer;
        }
        *out_distance = 3.4028235e+38f;
        return 0;
    }

    node = &context->nodes[node_index];
    dx = point->x - node->position.x;
    dy = point->y - node->position.y;
    dz = point->z - node->position.z;
    leash = node->travelled_distance;

    secondary = 0.0f;
    if (((path_find_request *)context)->have_avoid_sphere != 0) {
        float closest_x, closest_y, closest_z;
        real_point3d closest;

        halo::math::path_find_closest_point_on_segment(reinterpret_cast<path_find_request *>(context)->avoid_position, node->position, *point,
            closest);
        closest_x = closest.x;
        closest_y = closest.y;
        closest_z = closest.z;
        {
            float fx = closest_x - ((path_find_request *)context)->avoid_position.x;
            float fy = closest_y - ((path_find_request *)context)->avoid_position.y;
            float fz = closest_z - ((path_find_request *)context)->avoid_position.z;
            secondary = (float)halo::libm::sqrt(fz * fz + fy * fy + fx * fx);
        }
        if (node->avoid_distance < secondary) {
            secondary = node->avoid_distance;
        }
    }

    if (out_secondary != 0) {
        *out_secondary = secondary;
    }
    *out_distance = (float)halo::libm::sqrt(dz * dz + dx * dx + dy * dy) + leash;

    if (out_direction != 0) {
        int16_t prev = -1;
        int16_t cur = node_index;
        path_find_node *cur_node;
        float accumulated = 0.0f;
        real_point3d *lookahead_position = point;

        do {
            cur_node = &context->nodes[cur];
            cur_node->unknown_00 = prev;
            prev = cur;
            cur = cur_node->parent;
        } while (cur_node->parent != -1);

        cur = prev;
        cur_node = &context->nodes[cur];
        while (cur != -1) {
            if (0.8f <= accumulated) {
                lookahead_position = &cur_node->position;
                break;
            }
            accumulated = accumulated + cur_node->cost;
            cur_node = &context->nodes[cur];
            cur = cur_node->unknown_00;
        }

        out_direction->i = lookahead_position->x - context->start_position.x;
        out_direction->j = lookahead_position->y - context->start_position.y;
        out_direction->k = lookahead_position->z - context->start_position.z;
        halo::math::vector3d_normalize_with_length(*out_direction);
    }
    return 1;
}

/**
 * Behaviour of path find context init, moved unchanged from the original free function.
 *
 * @address 0x43a700
 */
void PathFinder::context_init(const path_find_request *request, uint32_t second_param)
{
    path_find_context * context = ptr;

    memset(context, 0, sizeof(*context));
    context->structure_bsp = (uint32_t)halo::scenario::globals().structure_bsp;
    memcpy(context, request, sizeof(*request));
    context->obstacle_cache = second_param;
}

/**
 * Behaviour of path find find unobstructed ancestor, moved unchanged from the original free function.
 *
 * @address 0x43a220
 */
uint8_t PathFinder::find_unobstructed_ancestor(uint32_t vertex_id, real_point3d *point, uint8_t *out_used_start, real_point3d *out_position)
{
    path_find_context * context = ptr;
    int16_t node_index;
    path_find_node *node;

    node_index = halo::ai::path_find_hash_lookup_vertex(context, vertex_id);
    if (node_index == -1) {
        return 0;
    }

    node = &context->nodes[node_index];
    while (node->parent != -1) {
        path_find_node *parent = &context->nodes[node->parent];
        path_find_boundary_crossing crossing;

        if (halo::ai::path_find_trace_bsp_boundary((void *)context->structure_bsp, reinterpret_cast<path_find_request *>(context)->ignores_glass, point,
                (int32_t)vertex_id, &parent->position, (int32_t)parent->vertex_id, &crossing) != 0) {
            break;
        }
        node = parent;
    }

    if (node->parent == -1) {
        *out_used_start = 1;
        *out_position = context->start_position;
        return 1;
    }

    *out_used_start = 0;
    *out_position = node->position;
    return 1;
}

/**
 * Kept as raw offsets on an opaque context pointer rather than asserting a type this module does not
 * otherwise use. The 32-byte output record's own layout (edge id, a flag byte, a start vertex position,
 * and a direction vector) is inferred purely from the write pattern below.
 *
 * @address 0x43b1c0
 */
int16_t PathFindGeometry::gather_adjacent_edges(void *context, int32_t vertex_id, path_find_adjacent_edge *out_edges)
{
    uint8_t *flag_table = map_surface_permissions(context);
    ModelCollisionGeometryBSP *bsp = map_collision_bsp(context);
    int32_t edge_index = bsp_surface(bsp, vertex_id)->first_edge;
    int32_t first_edge_index = edge_index;
    int16_t count = 0;
    int32_t *edge;
    uint8_t is_second_vertex;

    do {
        edge = bsp_edge(bsp, edge_index);
        is_second_vertex = (vertex_id == edge[k_edge_right_surface]);

        edge_index = edge[is_second_vertex ? k_edge_left_surface : k_edge_right_surface];
        out_edges[count].edge_id = edge_index;
        out_edges[count].flag = flag_table[edge_index];

        {
            float *point_a = bsp_vertex(bsp, edge[k_edge_start_vertex]);
            float *point_b = bsp_vertex(bsp, edge[k_edge_end_vertex]);
            out_edges[count].start_x = point_a[0];
            out_edges[count].start_y = point_a[1];
            out_edges[count].start_z = point_a[2];
            out_edges[count].direction_x = point_b[0] - point_a[0];
            out_edges[count].direction_y = point_b[1] - point_a[1];
            out_edges[count].direction_z = point_b[2] - point_a[2];
        }

        count = count + 1;
        if (count == 0x40) {
            return count;
        }
        edge_index = edge[is_second_vertex ? k_edge_reverse_edge : k_edge_forward_edge];
    } while (edge_index != first_edge_index);

    return count;
}

/**
 * Behaviour of path find hash lookup vertex, moved unchanged from the original free function.
 *
 * @address 0x43b2b0
 */
int16_t PathFinder::hash_lookup_vertex(uint32_t vertex_id)
{
    path_find_context * context = ptr;
    uint32_t slot = (vertex_id & k_vertex_hash_bucket_mask) << 3;
    int16_t node;

    for (;;) {
        node = context->vertex_hash[slot];
        slot = (slot + 1) & k_vertex_hash_slot_mask;
        if (node == -1) {
            return node;
        }
        if (context->nodes[node].vertex_id == vertex_id) {
            return node;
        }
    }
}

/**
 * Behaviour of path find heap push, moved unchanged from the original free function.
 *
 * @address 0x43b0f0
 */
void PathFinder::heap_push(int16_t node, int16_t key)
{
    path_find_context * context = ptr;
    int16_t index = context->heap_count;

    if (index < k_path_find_maximum_heap) {
        context->heap_count = index + 1;
        context->heap[index].node = node;
        context->heap[index].key = key;
        halo::ai::path_find_heap_sift_up(context, index);
    }
}

/**
 * Behaviour of path find heap sift down, moved unchanged from the original free function.
 *
 * @address 0x43b010
 */
void PathFinder::heap_sift_down(int16_t index)
{
    path_find_context * context = ptr;
    int16_t node = context->heap[index].node;
    int16_t key = context->heap[index].key;
    int16_t child_slot;
    int16_t best_slot;
    int16_t best_node;
    int16_t best_key;
    int16_t i;

    for (;;) {
        best_slot = index;
        best_node = node;
        best_key = key;
        child_slot = index * 2;

        for (i = 0; i < 2; i = i + 1) {
            if (context->heap_count <= child_slot) {
                break;
            }
            if (context->heap[child_slot].key < best_key) {
                best_node = context->heap[child_slot].node;
                best_slot = child_slot;
                best_key = context->heap[child_slot].key;
            }
            child_slot = child_slot + 1;
        }

        if (best_slot == index) {
            context->heap[index].key = key;
            context->heap[index].node = node;
            context->nodes[node].heap_index = index;
            return;
        }

        context->heap[index].key = best_key;
        context->heap[index].node = best_node;
        context->nodes[best_node].heap_index = index;
        index = best_slot;
    }
}

/**
 * Behaviour of path find heap sift up, moved unchanged from the original free function.
 *
 * @address 0x43af70
 */
void PathFinder::heap_sift_up(int16_t index)
{
    path_find_context * context = ptr;
    int16_t node = context->heap[index].node;
    int16_t key = context->heap[index].key;
    int16_t parent_slot;
    int16_t parent_node;
    int16_t parent_key;

    while (1 < index) {
        parent_slot = index >> 1;
        parent_node = context->heap[parent_slot].node;
        parent_key = context->heap[parent_slot].key;
        if (parent_key <= key) {
            break;
        }
        context->heap[index].node = parent_node;
        context->heap[index].key = parent_key;
        context->nodes[parent_node].heap_index = index;
        index = parent_slot;
    }

    context->heap[index].node = node;
    context->heap[index].key = key;
    context->nodes[node].heap_index = index;
}

/**
 * Behaviour of path find heights are close, moved unchanged from the original free function.
 *
 * @address 0x43d910
 */
uint8_t PathFindGeometry::heights_are_close(ScenarioStructureBSP *structure_bsp, real_point2d *point, int32_t surface_a, int32_t surface_b)
{
    ModelCollisionGeometryBSP *collision_bsp;
    ModelCollisionGeometryBSPSurface *surfaces;
    real_plane3d *planes;
    real_point3d position_a, position_b;

    if (surface_a == -1 || surface_b == -1) {
        return 0;
    }
    collision_bsp = (ModelCollisionGeometryBSP *)(uintptr_t)structure_bsp->collision_bsp.pointer;
    surfaces = (ModelCollisionGeometryBSPSurface *)(uintptr_t)collision_bsp->surfaces.pointer;
    planes = (real_plane3d *)(uintptr_t)collision_bsp->planes.pointer;

    halo::math::decal_plane_solve_third_axis(&position_a, 1, 2, &planes[surfaces[surface_a].plane & k_bsp_plane_index_mask], *point);
    halo::math::decal_plane_solve_third_axis(&position_b, 1, 2, &planes[surfaces[surface_b].plane & k_bsp_plane_index_mask], *point);
    return (uint8_t)(halo::libm::fabs((double)(position_a.z - position_b.z)) < 0.05000000074505806);
}

/**
 * Behaviour of path find push start node, moved unchanged from the original free function.
 *
 * @address 0x43a760
 */
uint8_t PathFinder::push_start_node()
{
    path_find_context * context = ptr;
    float value;
    int32_t key;
    int16_t node_index;
    path_find_node *node;

    if ((context->start_vertex_id == halo::k_dword_none) || (context->start_position.z <= -1000.0f)) {
        return 0;
    }

    if (context->have_goal == 0) {
        value = 0.0f;
        key = 0;
    } else {
        double dx = (double)context->goal_position.x - context->start_position.x;
        double dy = (double)context->goal_position.y - context->start_position.y;
        double dz = (double)context->goal_position.z - context->start_position.z;
        double distance = halo::libm::sqrt(dx * dx + dy * dy + dz * dz);
        value = (float)distance;
        key = halo::x87::__ftol(distance * 10.0);
        if (0x7ffe < key) {
            return 0;
        }
    }

    node_index = context->node_count;
    node = &context->nodes[node_index];
    context->node_count = node_index + 1;

    node->parent = -1;
    node->previous_vertex_id = -1;
    node->vertex_id = context->start_vertex_id;
    node->position = context->start_position;
    node->distance = value;
    node->cost = 0.0f;
    node->travelled_distance = 0.0f;
    node->avoid_distance = 3.4028235e+38f;
    node->accumulated_cost = 0.0f;
    node->key = (int16_t)key;
    node->waypoint = 0;

    if (context->have_goal != 0) {
        context->best_cost = value;
        context->best_estimate = value;
        context->best_position.x = context->start_position.x;
        context->best_position.y = context->start_position.y;
        context->best_node = node_index;
        context->best_position.z = context->start_position.z;
    }

    context->vertex_hash[(node->vertex_id & k_vertex_hash_bucket_mask) * 8] = node_index;
    halo::ai::path_find_heap_push(context, node_index, (int16_t)key);
    return 1;
}

/**
 * Behaviour of path find reconstruct path, moved unchanged from the original free function.
 *
 * @address 0x43a4d0
 */
uint8_t PathFinder::reconstruct_path(uint8_t *out_result)
{
    path_find_context * context = ptr;
    path_find_waypoint chain[0x40];
    path_find_waypoint simplified[4];
    path_find_waypoint final_waypoints[4];
    int16_t simplified_count = 0;
    int16_t final_count = 0;
    uint8_t valid = 1;
    int16_t node_index;
    int16_t count;
    int16_t previous = -1;
    path_find_node *previous_node = 0;
    path_find_result *result = reinterpret_cast<path_find_result *>(out_result);
    real_point3d *end_point = &result->end_point;

    result->found = 0;
    if (!context->have_goal) {
        return 0;
    }
    node_index = halo::ai::path_find_hash_lookup_vertex(context, context->goal_vertex_id);
    if (node_index != -1) {
        *end_point = context->goal_position;
        result->end_surface_index = (int32_t)context->goal_vertex_id;
        result->remaining_distance = 0.0f;
    } else {
        if (!(context->best_cost < context->goal_cost)) {
            return 0;
        }
        node_index = context->best_node;
        *end_point = context->best_position;
        result->end_surface_index = (int32_t)context->nodes[node_index].vertex_id;
        result->remaining_distance = context->best_cost;
    }
    if (node_index == -1) {
        return 0;
    }
    count = (int16_t)(context->nodes[node_index].waypoint + 1);
    if (count > 0x40) {
        count = 0x40;
    }
    do {
        path_find_node *node = &context->nodes[node_index];
        int16_t waypoint = node->waypoint;

        if (waypoint >= 0x40) {
            valid = 0;
        } else {
            chain[waypoint].surface_index = (int32_t)node->vertex_id;
            chain[waypoint].position = (previous == -1) ? *end_point : previous_node->position;
        }
        previous = node_index;
        previous_node = node;
        node_index = node->parent;
    } while (node_index != -1);

    halo::ai::path_find_simplify_waypoints(context, count, chain, &simplified_count, simplified, &valid);
    if (!halo::ai::ai_navigate_around_obstacles(context, simplified_count, simplified, &final_count, final_waypoints, &valid)) {
        return 0;
    }
    result->valid = valid;
    result->waypoint_count = (int8_t)final_count;
    result->found = 1;
    result->unknown_1a[0] = 0;
    memcpy(result->waypoints, final_waypoints, (size_t)final_count * sizeof(path_find_waypoint));
    if (result->valid) {
        path_find_waypoint *last = result->waypoints + (result->waypoint_count - 1);

        *end_point = last->position;
        result->end_surface_index = last->surface_index;
        result->remaining_distance = halo::math::vector3d_distance(context->goal_position, *end_point);
    }
    return result->found;
}

namespace {

static uint8_t path_find_search(path_find_context *context)
{
    path_find_request *request = (path_find_request *)context;
    float radius = (0.2f > request->pathfinding_radius) ? 0.2f : request->pathfinding_radius;
    path_find_adjacent_edge edges[64];

    while (context->heap_count > 1) {
        int16_t current = context->heap[1].node;
        path_find_node *node = &context->nodes[current];
        int16_t edge_count;
        int16_t e;

        node->heap_index = -1;
        context->heap_count--;
        if (context->heap_count > 1) {
            context->heap[1] = context->heap[context->heap_count];
            halo::ai::path_find_heap_sift_down(context, 1);
        }
        if (current == -1) {
            break;
        }
        if (context->have_goal) {
            float limit;

            if (node->vertex_id == context->goal_vertex_id) {
                context->best_position = context->goal_position;
                context->best_node = current;
                context->best_cost = 0.0f;
                break;
            }
            limit = (5.0f > context->best_cost) ? 5.0f : context->best_cost;
            if (node->distance > limit * 10.0f + context->best_estimate) {
                break;
            }
        }
        edge_count = halo::ai::path_find_gather_adjacent_edges((void *)(uintptr_t)context->structure_bsp, node->vertex_id, edges);
        for (e = 0; e < edge_count; e++) {
            path_find_adjacent_edge *edge = &edges[e];
            uint8_t passable = (uint8_t)((uint32_t)edge->edge_id != (uint32_t)node->previous_vertex_id);
            real_point3d candidate;
            float step;
            float travelled;
            float cost;
            float avoid_distance;
            float g;
            float f;
            float goal_distance = 0.0f;
            int32_t key;
            int16_t slot;
            int16_t index = -1;
            path_find_node *next;

            if ((edge->flag & 0x40) == 0) {
                passable = 0;
            }
            if (request->ignores_glass == 0 && (edge->flag & 0x80) != 0) {
                ModelCollisionGeometryBSPSurface *record = bsp_surface(map_collision_bsp((const void *)(uintptr_t)context->structure_bsp), edge->edge_id);

                if ((record->flags & 8) != 0) {
                    uint32_t bit = (uint8_t)record->breakable_surface;
                    uint32_t word = *(uint32_t *)(breakable_surface_state + 1 +
                        ((bit >> 5) + halo::scenario::globals().structure_bsp_index * 8) * 4);

                    if ((word & (1u << (bit & 0x1f))) == 0) {
                        continue; // 0x43aa82: the glass is still intact
                    }
                }
            }
            if (!passable) {
                continue;
            }
            candidate.x = edge->direction_x * 0.5f + edge->start_x;
            candidate.y = edge->direction_y * 0.5f + edge->start_y;
            candidate.z = edge->direction_z * 0.5f + edge->start_z;
            if (context->have_goal) {
                float dx2 = edge->direction_x * edge->direction_x;
                float length2 = edge->direction_z * edge->direction_z + edge->direction_y * edge->direction_y + dx2;

                if (length2 > 16.0f && length2 > (radius + radius) * (radius + radius)) {
                    float length = (float)halo::libm::sqrt(length2);
                    float t = ((context->goal_position.x - edge->start_x) * edge->direction_x +
                        (context->goal_position.z - edge->start_z) * edge->direction_z +
                        (context->goal_position.y - edge->start_y) * edge->direction_y) /
                        (edge->direction_z * edge->direction_z + edge->direction_y * edge->direction_y + dx2);
                    float margin = radius / length;

                    if (t < margin) {
                        t = margin;
                    } else if (t > 1.0f - margin) {
                        t = 1.0f - margin;
                    }
                    candidate.x = t * edge->direction_x + edge->start_x;
                    candidate.y = t * edge->direction_y + edge->start_y;
                    candidate.z = t * edge->direction_z + edge->start_z;
                }
            }
            {
                float dx = candidate.x - node->position.x;
                float dy = candidate.y - node->position.y;
                float dz = candidate.z - node->position.z;

                step = (float)halo::libm::sqrt(dz * dz + dy * dy + dx * dx);
            }
            travelled = step + node->travelled_distance;
            if (request->have_avoid_sphere) {
                cost = (halo::ai::path_find_score_avoidance_penalty(context, &node->position, &candidate, &avoid_distance) + 1.0f) *
                    step;
                if (!(node->avoid_distance > avoid_distance)) {
                    avoid_distance = node->avoid_distance;
                }
            } else {
                cost = step;
                avoid_distance = 0.0f;
            }
            g = cost + node->accumulated_cost;
            f = g;
            if (context->have_goal) {
                float dx = context->goal_position.x - candidate.x;
                float dy = context->goal_position.y - candidate.y;
                float dz = context->goal_position.z - candidate.z;

                goal_distance = (float)halo::libm::sqrt(dz * dz + dy * dy + dx * dx);
                f = goal_distance + g;
            }
            key = (int32_t)(f * 10.0f);
            if (key >= INT16_MAX) {
                continue;
            }
            if (request->have_limit && travelled > request->limit_distance) {
                continue;
            }

            // 0x43aced: the vertex hash
            slot = (int16_t)((edge->edge_id & k_vertex_hash_bucket_mask) << 3);
            while (context->vertex_hash[slot] != -1) {
                path_find_node *existing = &context->nodes[context->vertex_hash[slot]];

                if (existing->vertex_id == (uint32_t)edge->edge_id) {
                    if (key >= existing->key || existing->heap_index == -1) {
                        index = -2; // no improvement, or already closed
                    } else {
                        index = context->vertex_hash[slot];
                    }
                    break;
                }
                slot = (int16_t)((slot + 1) & k_vertex_hash_slot_mask);
            }
            if (index == -2) {
                continue;
            }
            if (index == -1) {
                if (context->node_count >= k_path_node_capacity) {
                    continue;
                }
                index = context->node_count++;
                context->vertex_hash[slot] = index;
                context->nodes[index].heap_index = -1;
            }

            next = &context->nodes[index];
            next->parent = current;
            next->previous_vertex_id = (int32_t)node->vertex_id;
            next->vertex_id = (uint32_t)edge->edge_id;
            next->position = candidate;
            next->cost = step;
            next->avoid_distance = avoid_distance;
            next->travelled_distance = travelled;
            next->accumulated_cost = g;
            next->distance = f;
            next->key = (int16_t)key;
            next->waypoint = (int16_t)(node->waypoint + 1);
            if (next->heap_index != -1) {
                context->heap[next->heap_index].key = (int16_t)key;
                halo::ai::path_find_heap_sift_up(context, next->heap_index);
            } else if (context->heap_count < k_path_node_capacity) {
                int16_t heap_slot = context->heap_count++;

                context->heap[heap_slot].key = (int16_t)key;
                context->heap[heap_slot].node = index;
                halo::ai::path_find_heap_sift_up(context, heap_slot);
            }

            if (context->have_goal) {
                real_point3d best_point = next->position;
                float distance = goal_distance;

                if (distance < 4.0f) {
                    distance = halo::ai::path_find_vertex_distance((ScenarioStructureBSP *)(uintptr_t)context->structure_bsp,
                        edge->edge_id, &context->goal_position, &best_point);
                }
                if (distance < context->best_cost) {
                    context->best_cost = distance;
                    context->best_position = best_point;
                    context->best_node = index;
                    context->best_estimate = f;
                }
            }
        }
    }

    if (context->have_goal) {
        return (uint8_t)(context->best_cost <= context->goal_cost);
    }
    return 1;
}

}

/**
 * Behaviour of path find run, moved unchanged from the original free function.
 *
 * @address 0x43a8b0
 */
uint8_t PathFinder::run()
{
    path_find_context * context = ptr;
    int32_t i;

    context->node_count = 0;
    context->heap_count = 1;
    for (i = 0; i < k_vertex_hash_slot_count; i++) {
        context->vertex_hash[i] = -1;
    }
    context->best_node = -1;
    context->best_cost = 3.4028235e+38f;
    context->best_estimate = 3.4028235e+38f;
    if (!halo::ai::path_find_push_start_node(context)) {
        return 0;
    }
    return path_find_search(context);
}

/**
 * Behaviour of path find score avoidance penalty, moved unchanged from the original free function.
 *
 * @address 0x43b3b0
 */
float PathFinder::score_avoidance_penalty(const real_point3d *segment_start, const real_point3d *segment_end, float *out_distance)
{
    path_find_context * context = ptr;
    path_find_request *request = (path_find_request *)context;
    real_point3d closest;
    float dx, dy, dz;
    float distance2;

    halo::math::path_find_closest_point_on_segment(request->avoid_position, *segment_start, *segment_end, closest);
    dx = closest.x - request->avoid_position.x;
    dy = closest.y - request->avoid_position.y;
    dz = closest.z - request->avoid_position.z;
    distance2 = dz * dz + dx * dx + dy * dy;
    if (distance2 < request->avoid_radius * request->avoid_radius) {
        float distance = (float)halo::libm::sqrt(distance2);

        *out_distance = distance;
        return (1.0f - distance / request->avoid_radius) * request->avoid_weight;
    }
    *out_distance = 3.4028235e+38f;
    return 0.0f;
}

/**
 * Behaviour of path find set avoid sphere, moved unchanged from the original free function.
 *
 * @address 0x43a070
 */
void PathFinder::set_avoid_sphere(const real_point3d *position, float avoid_radius, datum_index avoid_object_index, float avoid_weight)
{
    path_find_context * context = ptr;
    path_find_request *request = (path_find_request *)context;

    request->have_avoid_sphere = 1;
    request->avoid_position = *position;
    request->avoid_radius = avoid_radius;
    request->avoid_object_index = avoid_object_index;
    request->avoid_weight = avoid_weight;
}

/**
 * Behaviour of path find set goal, moved unchanged from the original free function.
 *
 * @address 0x43a730
 */
void PathFinder::set_goal(const real_point3d *position, uint32_t goal_vertex_id, float goal_cost)
{
    path_find_context * context = ptr;
    context->have_goal = 1;
    context->goal_position = *position;
    context->goal_vertex_id = goal_vertex_id;
    context->goal_cost = goal_cost;
}

/**
 * Behaviour of path find simplify waypoints, moved unchanged from the original free function.
 *
 * @address 0x43cc00
 */
void PathFinder::simplify_waypoints(int16_t count, path_find_waypoint *waypoints, int16_t *out_count, path_find_waypoint *out_waypoints, uint8_t *out_valid)
{
    path_find_context * context = ptr;
    path_find_request *request = (path_find_request *)context;
    void *map = (void *)(uintptr_t)context->structure_bsp;
    uint8_t ignore_permission = request->ignores_glass;
    real_point3d current;
    int32_t current_surface;
    int16_t emitted = 0;
    uint8_t valid = 0;
    int16_t first = 1;

    if (count <= 1) {
        *out_count = 1;
        out_waypoints[0] = waypoints[0];
        return;
    }
    current = context->start_position;
    current_surface = (int32_t)context->start_vertex_id;

    for (;;) {
        int16_t blocked_index = -1;
        int32_t blocked_edge = -1;
        uint8_t blocked = 0;
        int16_t i;

        for (i = first; i < count; i++) {
            path_find_boundary_crossing hit;

            if (halo::ai::path_find_test_segment_unobstructed(map, &current, ignore_permission, current_surface,
                    &waypoints[i].position, waypoints[i].surface_index, 0.3f, 1, &hit) != 0) {
                if (!blocked) {
                    blocked_index = i;
                    blocked_edge = hit.edge_b;
                    blocked = 1;
                }
            } else if (blocked) {
                blocked_edge = -1;
                blocked_index = -1;
                blocked = 0;
            }
        }
        if (!blocked || blocked_edge == -1) {
            out_waypoints[emitted++] = waypoints[count - 1];
            valid = 1;
            break;
        }
        {
            real_point2d corner_a;
            real_point2d corner_b;
            real_point2d corner;
            real_point2d tangent[2];
            real_point2d crossing;
            real_point3d previous = current;
            uint8_t side;
            path_find_waypoint *entry;

            if (!halo::ai::path_find_trace_cluster_boundary(map, blocked_edge, (real_point2d *)&current, 0.3f, 1, ignore_permission,
                    &corner_a) ||
                !halo::ai::path_find_trace_cluster_boundary(map, blocked_edge, (real_point2d *)&current, 0.3f, 0, ignore_permission,
                    &corner_b)) {
                break;
            }
            side = halo::ai::ai_search_choose_shorter_corner((real_point2d *)&current, &corner_a,
                (real_point2d *)&waypoints[blocked_index - 1].position, &corner_b,
                (real_point2d *)&waypoints[blocked_index].position, &corner);
            halo::ai::ai_search_find_circle_tangent_point(&corner, (real_point2d *)&current, &tangent[0], 0.35f, side);
            halo::ai::ai_search_find_circle_tangent_point(&corner, (real_point2d *)&waypoints[blocked_index].position, &tangent[1],
                0.35f, (uint8_t)(side == 0));
            halo::ai::ai_search_find_circle_portal_crossing(&corner, tangent, &crossing, (real_point2d *)&current, 0.35f);
            current.x = crossing.x;
            current.y = crossing.y;
            if (current_surface == -1) {
                current_surface = -1;
            } else {
                path_find_boundary_crossing trace;

                halo::ai::path_find_trace_bsp_boundary(map, ignore_permission, &previous, current_surface, &current, -1, &trace);
                current.x = trace.position.x;
                current.y = trace.position.y;
                if (trace.edge_a != -1) {
                    current_surface = trace.edge_a;
                }
            }
            {
                entry = &out_waypoints[emitted++];
                halo::math::decal_plane_solve_third_axis(&entry->position, 1, 2,
                    bsp_surface_plane(map_collision_bsp(map), current_surface), *((real_point2d *)&current));
                entry->surface_index = current_surface;
            }
            if (emitted >= 4) {
                break;
            }
            first = blocked_index;
        }
    }
    *out_count = emitted;
    if (!valid) {
        *out_valid = 0;
    }
}

/**
 * Behaviour of path find test direct reachability, moved unchanged from the original free function.
 *
 * @address 0x43a0a0
 */
uint8_t PathFindGeometry::test_direct_reachability(const real_point3d *point_a, const real_point3d *point_b, real_point3d *out_position, void *context, uint8_t *out_success)
{
    float fraction;
    float dx, dy, dz;
    uint8_t hit;
    uint8_t success;

    {
        collision_bsp_segment_result result;
        real_vector3d delta;

        delta.i = point_a->x - point_b->x;
        delta.j = point_a->y - point_b->y;
        delta.k = point_a->z - point_b->z;
        hit = halo::physics::collision_bsp_query_segment_init(1, &result, map_collision_bsp(context),
            0, 0, const_cast<real_point3d *>(point_b), &delta, 3.4028235e+38f);
        fraction = result.t;
    }

    success = 0;
    if ((hit == 0) || (1.0f <= fraction)) {
        success = 1;
    } else {
        dx = point_a->x - point_b->x;
        dy = point_a->y - point_b->y;
        dz = point_a->z - point_b->z;
        if ((1.0f - fraction) * (1.0f - fraction) * (dx * dx + dy * dy + dz * dz) < 0.1f) {
            success = 1;
        }
    }

    if (out_success != 0) {
        *out_success = success;
    }
    if (out_position != 0) {
        *out_position = *point_a;
    }
    return success;
}

namespace {

static int32_t path_find_offset_end(void *map, uint8_t ignore_permission, real_point3d *point, int32_t surface,
    float ox, float oy, real_point3d *out_point)
{
    path_find_boundary_crossing scratch;

    out_point->x = ox + point->x;
    out_point->y = oy + point->y;
    if (surface == -1) {
        return -1;
    }
    halo::ai::path_find_trace_bsp_boundary(map, ignore_permission, point, surface, out_point, -1, &scratch);
    out_point->x = scratch.position.x;
    out_point->y = scratch.position.y;
    return (scratch.edge_a == -1) ? surface : scratch.edge_a;
}

static uint8_t path_find_trace_side(void *map, uint8_t ignore_permission, real_point3d *from, int32_t from_surface,
    real_point3d *to, int32_t to_surface, real_point3d *point_b, int32_t surface_b, uint8_t flags,
    path_find_boundary_crossing *result)
{
    path_find_boundary_crossing scratch;

    if (from_surface == -1) {
        result->found = 0;
        return 0;
    }
    if (halo::ai::path_find_trace_bsp_boundary(map, ignore_permission, from, from_surface, to, to_surface, result) != 0 &&
        result->edge_a != -1 && (flags & 1) == 0 &&
        halo::ai::path_find_trace_bsp_boundary(map, ignore_permission, &result->position, result->edge_a, point_b, surface_b,
            &scratch) == 0) {
        result->found = 0;
    }
    return result->found;
}

}

/**
 * Behaviour of path find test segment unobstructed, moved unchanged from the original free function.
 *
 * @address 0x43de90
 */
uint8_t PathFindGeometry::test_segment_unobstructed(void *map, real_point3d *point_a, uint8_t ignore_permission, int32_t surface_a, real_point3d *point_b, int32_t surface_b, float radius, uint8_t flags, path_find_boundary_crossing *out_result)
{
    float dx = point_b->x - point_a->x;
    float ny_neg = -(point_b->y - point_a->y);
    float length = (float)halo::libm::sqrt(ny_neg * ny_neg + dx * dx);
    float nx;
    float ny;
    real_point3d a_plus;
    real_point3d b_plus;
    real_point3d a_minus;
    real_point3d b_minus;
    int32_t a_plus_surface;
    int32_t b_plus_surface;
    int32_t a_minus_surface;
    int32_t b_minus_surface;
    path_find_boundary_crossing plus_result;
    path_find_boundary_crossing minus_result;
    path_find_boundary_crossing *chosen;
    uint8_t plus_hit;
    uint8_t minus_hit;

    if ((float)halo::libm::fabs(length) < 0.0001f || !(length > 0.0f)) {
        return 0;
    }
    nx = ny_neg * (1.0f / length);
    ny = dx * (1.0f / length);
    memset(&plus_result, 0, sizeof(plus_result));
    memset(&minus_result, 0, sizeof(minus_result));
    a_plus = *point_a;
    b_plus = *point_b;
    a_minus = *point_a;
    b_minus = *point_b;
    a_plus_surface = path_find_offset_end(map, ignore_permission, point_a, surface_a, nx * radius, ny * radius, &a_plus);
    b_plus_surface = path_find_offset_end(map, ignore_permission, point_b, surface_b, nx * radius, ny * radius, &b_plus);
    a_minus_surface = path_find_offset_end(map, ignore_permission, point_a, surface_a, nx * -radius, ny * -radius,
        &a_minus);
    b_minus_surface = path_find_offset_end(map, ignore_permission, point_b, surface_b, nx * -radius, ny * -radius,
        &b_minus);

    plus_hit = path_find_trace_side(map, ignore_permission, &a_plus, a_plus_surface, &b_plus, b_plus_surface, point_b,
        surface_b, flags, &plus_result);
    minus_hit = path_find_trace_side(map, ignore_permission, &a_minus, a_minus_surface, &b_minus, b_minus_surface,
        point_b, surface_b, flags, &minus_result);

    if (plus_hit) {
        chosen = (minus_hit && !(plus_result.fraction < minus_result.fraction)) ? &minus_result : &plus_result;
    } else if (minus_hit) {
        chosen = &minus_result;
    } else {
        *out_result = plus_result;
        return 0;
    }
    {
        float ex = point_b->x - chosen->position.x;
        float ey = point_b->y - chosen->position.y;

        if (radius * radius > ey * ey + ex * ex) {
            *out_result = plus_result;
            return 0;
        }
    }
    *out_result = *chosen;
    return 1;
}



/**
 * Behaviour of path find trace bsp boundary, moved unchanged from the original free function.
 *
 * @address 0x43d9b0
 */
uint8_t PathFindGeometry::trace_bsp_boundary(void *map, uint8_t ignore_permission, real_point3d *start, int32_t start_surface, real_point3d *end, int32_t target_surface, path_find_boundary_crossing *out_result)
{
    ModelCollisionGeometryBSP *bsp = map_collision_bsp(map);
    uint8_t *walkable = map_surface_permissions(map);
    uint32_t *broken = (uint32_t *)(breakable_surface_state + 1 + halo::scenario::globals().structure_bsp_index * 32);
    float dx = end->x - start->x;
    float dy = end->y - start->y;
    uint8_t retried = 0;
    int32_t surface = start_surface;

    for (;;) {
        ModelCollisionGeometryBSPSurface *record = bsp_surface(bsp, surface);
        int32_t edge_index = static_cast<int32_t>(record->first_edge);
        real_point3d centroid = *global_zero_vector3d_pointer;
        int32_t vertex_count = 0;
        uint8_t outside = 0;
        uint8_t borders_target = 0;
        int32_t next_surface = -1;

        do {
            int32_t *edge = bsp_edge(bsp, edge_index);
            uint8_t side = (uint8_t)(surface == edge[k_edge_right_surface]);
            float *va = bsp_vertex(bsp, edge[side ? k_edge_start_vertex : k_edge_end_vertex]);
            float *vb = bsp_vertex(bsp, edge[side ? k_edge_end_vertex : k_edge_start_vertex]);
            int32_t other = edge[side ? k_edge_left_surface : k_edge_right_surface];
            float ex = vb[0] - va[0];
            float ey = vb[1] - va[1];
            float px = end->x - va[0];
            float py = end->y - va[1];
            float ax = va[0] - start->x;
            float ay = va[1] - start->y;
            float bx = vb[0] - start->x;
            float by = vb[1] - start->y;

            if (other == target_surface) {
                borders_target = 1;
            }
            centroid.x += va[0];
            centroid.y += va[1];
            centroid.z += va[2];
            vertex_count++;
            if (ex * py - ey * px > 0.0f) {
                outside = 1;
                if (ay * dx - dy * ax > 0.0f && dy * bx - by * dx > 0.0f) {
                    uint8_t flags = walkable[other];
                    uint8_t passable = (uint8_t)((flags >> 6) & 1);

                    if (!ignore_permission && passable && (flags & 0x80) != 0) {
                        uint32_t bit = static_cast<uint8_t>(bsp_surface(bsp, other)->breakable_surface);

                        passable = (uint8_t)((broken[bit >> 5] & (1u << (bit & 0x1f))) != 0);
                    }
                    if (passable) {
                        next_surface = other;
                        break;
                    }
                    {
                        float length = (float)halo::libm::sqrt(ey * ey + ex * ex);
                        float t = (ay * ex - ey * ax - length * 0.0078125f) / (dy * ex - ey * dx);
                        real_point2d hit;

                        hit.x = dx * t + start->x;
                        hit.y = dy * t + start->y;
                        halo::math::decal_plane_solve_third_axis(&out_result->position, 1, 2, bsp_surface_plane(bsp, surface), hit);
                        out_result->edge_a = surface;
                        out_result->edge_b = edge_index;
                        out_result->found = 1;
                        out_result->fraction = t;
                        return 1;
                    }
                }
            }
            edge_index = edge[side ? k_edge_reverse_edge : k_edge_forward_edge];
        } while (edge_index != static_cast<int32_t>(record->first_edge));

        if (next_surface != -1) {
            surface = next_surface;
            continue;
        }
        if (!outside) {
            if (surface != target_surface && !borders_target && target_surface != -1) {
                halo::physics::collision_bsp_surface_solve_third_axis((ModelCollisionGeometryBSP *)bsp, start_surface, 1,
                    &out_result->position, 2, (real_point2d *)start);
                out_result->edge_a = -1;
                out_result->edge_b = -1;
                out_result->found = 1;
                out_result->fraction = 0.0f;
                return 1;
            }
            halo::math::decal_plane_solve_third_axis(&out_result->position, 1, 2, bsp_surface_plane(bsp, surface), *(real_point2d *)end);
            out_result->edge_a = surface;
            out_result->edge_b = -1;
            out_result->found = 0;
            out_result->fraction = 1.0f;
            return 0;
        }
        {
            float scale = 1.0f / (float)vertex_count;
            path_find_boundary_crossing local_result;

            centroid.x *= scale;
            centroid.y *= scale;
            if (!retried && walkable[surface] != 0 &&
                halo::ai::path_find_trace_bsp_boundary(map, ignore_permission, &centroid, surface, start, -1, &local_result) == 0) {
                retried = 1;
                surface = local_result.edge_a;
                continue;
            }
        }
        halo::math::decal_plane_solve_third_axis(&out_result->position, 1, 2, bsp_surface_plane(bsp, start_surface),
            *(real_point2d *)start);
        out_result->edge_a = -1;
        out_result->edge_b = -1;
        out_result->found = 1;
        out_result->fraction = 0.0f;
        return 1;
    }
}

namespace {

static uint8_t path_find_surface_passable(const void *bsp, uint8_t *walkable, uint32_t *broken, int32_t surface,
    uint8_t ignore_permission)
{
    uint8_t flags = walkable[surface];
    uint8_t passable = (uint8_t)((flags >> 6) & 1);

    if (!ignore_permission && passable && (flags & 0x80) != 0) {
        uint32_t bit = static_cast<uint8_t>(bsp_surface(bsp, surface)->breakable_surface);

        passable = (uint8_t)((broken[bit >> 5] & (1u << (bit & 0x1f))) != 0);
    }
    return passable;
}

}

/**
 * Behaviour of path find trace cluster boundary, moved unchanged from the original free function.
 *
 * @address 0x43d4b0
 */
uint8_t PathFindGeometry::trace_cluster_boundary(void *map, int32_t edge_index, real_point2d *origin, float radius, uint8_t side, uint8_t ignore_permission, real_point2d *out_point)
{
    ModelCollisionGeometryBSP *bsp = map_collision_bsp(map);
    uint8_t *walkable = map_surface_permissions(map);
    uint32_t *broken = (uint32_t *)(breakable_surface_state + 1 + halo::scenario::globals().structure_bsp_index * 32);
    int32_t first_pivot = -1;
    int32_t previous = -1;
    int32_t current = edge_index;
    int32_t *edge = bsp_edge(bsp, current);

    for (;;) {
        uint8_t first_passable = path_find_surface_passable(bsp, walkable, broken, edge[k_edge_left_surface], ignore_permission);
        float *v1 = bsp_vertex(bsp, edge[first_passable]);
        float *v2 = bsp_vertex(bsp, edge[!first_passable]);
        float ex = v2[0] - v1[0];
        float ey = v2[1] - v1[1];
        float length = (float)halo::libm::sqrt(ey * ey + ex * ex);
        float nx = ey;
        float ny = -ex;
        float w1x;
        float w1y;
        float w2x;
        float w2y;
        uint8_t turn = 0;
        int32_t pivot;
        int32_t rotation_start;

        if (!((float)halo::libm::fabs(length) < 0.0001f)) {
            float scale = 1.0f / length;

            nx = ey * scale;
            ny = -ex * scale;
        }
        w1x = v1[0] - (nx * radius + origin->x);
        w1y = v1[1] - (ny * radius + origin->y);
        w2x = v1[0] - (-radius * nx + origin->x);
        w2y = v1[1] - (ny * -radius + origin->y);
        if ((uint8_t)(w1y * ey + w1x * ex < 0.0f) == side && w1x * ey - w1y * ex < 0.0f) {
            turn = 1;
        }
        if (w2x * ey - w2y * ex < 0.0f) {
            turn = 1;
        }
        if (first_pivot == -1) {
            turn = 1;
        }
        pivot = ((uint8_t)(turn != first_passable) != side) ? edge[k_edge_start_vertex] : edge[k_edge_end_vertex];
        if (pivot == previous) {
            out_point->x = bsp_vertex(bsp, pivot)[0];
            out_point->y = bsp_vertex(bsp, pivot)[1];
            return 1;
        }
        if (pivot == first_pivot) {
            return 0;
        }
        if (first_pivot == -1) {
            first_pivot = pivot;
        }

        rotation_start = current;
        for (;;) {
            int32_t index = (pivot == edge[k_edge_end_vertex]) ? 0 : 1;

            if (path_find_surface_passable(bsp, walkable, broken, edge[k_edge_left_surface + index], ignore_permission) == side) {
                break;
            }
            current = edge[k_edge_forward_edge + index];
            edge = bsp_edge(bsp, current);
            if (current == rotation_start) {
                return 0;
            }
        }
        previous = pivot;
    }
}


namespace {

static uint8_t path_find_surface_passable_2(const uint8_t *surface_permissions, uint8_t ignore_permission,
                                          const ModelCollisionGeometryBSP *bsp,
                                          const uint32_t *intact_row, int32_t surface_index)
{
    uint8_t permission = surface_permissions[surface_index];
    uint32_t breakable;

    if (permission == 0) {
        return 0;
    }
    if (ignore_permission != 0 || (int8_t)permission >= 0) {
        return 1;
    }
    breakable = (uint8_t)((const ModelCollisionGeometryBSPSurface *)bsp->surfaces.pointer)[surface_index].breakable_surface;
    return (intact_row[breakable >> 5] & (1u << (breakable & 0x1f))) != 0;
}

}

/**
 * Behaviour of path find trace cluster boundary from vertex, moved unchanged from the original free
 * function.
 *
 * @address 0x43d790
 */
uint8_t PathFindGeometry::trace_cluster_boundary_from_vertex(void *context, uint8_t ignore_permission, real_point2d *point, int32_t start_index, real_vector2d *direction, float max_distance, path_find_boundary_trace_result *out)
{
    ModelCollisionGeometryBSP *bsp = map_collision_bsp(context);
    uint8_t *surface_permissions = map_surface_permissions(context);
    uint32_t *intact_row = breakable_surface_state_typed->active[halo::scenario::globals().structure_bsp_index];
    int32_t surface_index = start_index;
    collision_bsp_boundary_clip clip;

    for (;;) {
        halo::physics::collision_bsp_surface_clip_line_2d(&clip, bsp, surface_index, point, direction);

        if (max_distance < clip.enter.t &&
            path_find_surface_passable_2(surface_permissions, ignore_permission, bsp, intact_row,
                                       clip.enter.surface_index) &&
            clip.enter.surface_index != -1) {
            surface_index = clip.enter.surface_index;
            continue;
        }
        if (clip.exit.t < max_distance &&
            path_find_surface_passable_2(surface_permissions, ignore_permission, bsp, intact_row,
                                       clip.exit.surface_index) &&
            clip.exit.surface_index != -1) {
            surface_index = clip.exit.surface_index;
            continue;
        }
        break;
    }

    if (max_distance < clip.enter.t) {
        out->distance = clip.enter.t;
        out->surface_index = surface_index;
        out->edge_index = clip.enter.edge_index;
        return 1;
    }
    if (clip.exit.t < max_distance) {
        out->distance = clip.exit.t;
        out->surface_index = surface_index;
        out->edge_index = clip.exit.edge_index;
        return 1;
    }
    out->distance = max_distance;
    out->surface_index = surface_index;
    out->edge_index = -1;
    return 0;
}


/**
 * Behaviour of path find validate and record goal, moved unchanged from the original free function.
 *
 * @address 0x43a190
 */
uint8_t PathFindGeometry::validate_and_record_goal(ai_path_candidate_goal *candidate, void *context, uint32_t point_b, uint32_t unused_c, const real_point3d *position)
{
    uint32_t *clear;
    int32_t i;
    real_point3d reached;
    uint8_t reachable;

    (void)unused_c;

    clear = (uint32_t *)candidate;
    for (i = 0x17; i != 0; i = i - 1) {
        *clear = 0;
        clear = clear + 1;
    }

    if (halo::ai::path_find_test_direct_reachability(position, (const real_point3d *)point_b, &reached, context,
                                           &reachable) != 0) {
        candidate->alt_position = reached;
        candidate->flag_19 = 1;
        candidate->unknown_1c = halo::k_dword_none;
        candidate->flag_1a = 0;
        candidate->reachable = reachable;
        candidate->position = *position;
        candidate->unknown_10 = halo::k_dword_none;
        candidate->unknown_14 = 0;
        candidate->valid = 1;
    }
    return candidate->valid;
}

/**
 * Behaviour of path find vertex distance, moved unchanged from the original free function.
 *
 * @address 0x43b130
 */
float PathFindGeometry::vertex_distance(ScenarioStructureBSP *structure_bsp, int32_t surface, real_point3d *point_a, real_point3d *out_point)
{
    ModelCollisionGeometryBSP *collision_bsp =
        (ModelCollisionGeometryBSP *)(uintptr_t)structure_bsp->collision_bsp.pointer;
    ModelCollisionGeometryBSPSurface *surfaces = (ModelCollisionGeometryBSPSurface *)(uintptr_t)collision_bsp->surfaces.pointer;
    real_plane3d *planes = (real_plane3d *)(uintptr_t)collision_bsp->planes.pointer;
    real_point2d closest;
    float dx, dy, dz;

    halo::physics::collision_bsp_surface_closest_edge_point_2d(collision_bsp, surface, 2, 1, (real_point2d *)point_a, &closest);
    halo::math::decal_plane_solve_third_axis(out_point, 1, 2, &planes[surfaces[surface].plane & k_bsp_plane_index_mask], closest);

    dx = out_point->x - point_a->x;
    dy = out_point->y - point_a->y;
    dz = out_point->z - point_a->z;
    return (float)halo::libm::sqrt((double)(dx * dx + dz * dz + dy * dy));
}

}
