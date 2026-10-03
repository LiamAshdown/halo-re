#include "halo/ai/airest_search.hpp"

#include <stdint.h>
#include <string.h>
#include "halo/math/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/scenario/api.hpp"
#include "halo/core/datum.hpp"
#include "halo/core/slot_mask.hpp"
#include "halo/objects/api.hpp"
#include "halo/ai/api.hpp"
#include "halo/core/link.hpp"
#include "halo/ai/vars.hpp"

extern "C" {
extern double sqrt(double x);
extern double fabs(double x);
}
static auto &ai_default_2d_direction = halo::link::ref<real_point2d *>(halo::ai::vars().ai_default_2d_direction);

namespace halo::ai {

/**
 * Behaviour of ai search add node, moved unchanged from the original free function.
 *
 * @address 0x43b5a0
 */
int16_t AiSearch::add_node(int16_t parent, real_point2d *position, int32_t surface_index, int16_t point_id, uint8_t side, float base_cost)
{
    ai_search_context * context = ptr;
    float dx;
    float dy;
    uint8_t goal = 0;
    int16_t index;
    ai_search_node *node;

    if (context->node_count >= 0x80) {
        return -1;
    }
    dx = context->origin.x - position->x;
    dy = context->origin.y - position->y;
    if (parent != -1) {
        int16_t walk = parent;
        ai_search_node *ancestor;

        for (;;) {
            ancestor = &context->nodes[walk];
            if (ancestor->point_id != point_id) {
                break;
            }
            if (ancestor->side != side) {
                return -1;
            }
            walk = ancestor->parent;
            if (walk == -1) {
                ancestor = 0;
                break;
            }
        }
        if (ancestor != 0 && point_id == context->goal_point_id && context->goal_point_id != -1) {
            int16_t *links = &ancestor->side_link;
            int16_t other = links[side == 0];

            goal = 1;
            if (other != -1) {
                ai_search_node *linked = &context->nodes[other];

                if (dy * linked->direction.j + dx * linked->direction.i > 0.0f) {
                    real_vector2d *parent_direction = &context->nodes[parent].direction;
                    float turn = parent_direction->j * linked->direction.i - linked->direction.j * parent_direction->i;
                    float delta_turn = dy * linked->direction.i - dx * linked->direction.j;

                    if (delta_turn * turn < 0.0f) {
                        return -1;
                    }
                }
            }
            if (links[side] == parent || links[side] == -1) {
                links[side] = context->node_count;
            }
        }
    }

    index = context->node_count++;
    node = &context->nodes[index];
    node->position = *position;
    *(int32_t *)&node->z = surface_index;
    node->direction.i = dx;
    node->direction.j = dy;
    node->length = halo::math::vector2d_normalize_with_length(node->direction);
    node->cost = node->length + base_cost;
    node->point_id = point_id;
    node->side = side;
    node->parent = parent;
    (&node->side_link)[0] = -1;
    (&node->side_link)[1] = -1;
    if (goal && node->length < context->best_cost) {
        context->best_cost = node->length;
        context->best_node = index;
    }
    if (context->heap_count < 0x80) {
        int16_t slot = context->heap_count++;

        context->heap[slot] = index;
        halo::ai::ai_search_heap_sift_up(context, slot);
    }
    return index;
}

/**
 * Behaviour of ai search append obstacle, moved unchanged from the original free function.
 *
 * @address 0x43c4b0
 */
uint8_t ObstacleList::append_obstacle(uint16_t flags, uint32_t object_index, real_point2d *position, float radius)
{
    ai_search_obstacle_list * list = ptr;
    ai_search_obstacle *entry;

    if (list->count == 0x80) {
        return 0;
    }

    entry = &list->obstacles[list->count];
    list->count = list->count + 1;
    if ((flags & 1) != 0) {
        list->flagged_count = list->flagged_count + 1;
    }

    entry->flags = flags;
    entry->object_index = object_index;
    entry->link = -1;
    entry->position = *position;
    entry->radius = radius;
    return 1;
}

namespace {

static void ai_search_corner_direction(const real_point2d *point, const real_point2d *corner, real_vector2d *out)
{
    float length;

    out->i = point->x - corner->x;
    out->j = point->y - corner->y;
    length = (float)sqrt(out->j * out->j + out->i * out->i);
    if (!((float)fabs(length) < 0.0001f)) {
        float scale = 1.0f / length;

        out->i *= scale;
        out->j *= scale;
    }
}

}

/**
 * Behaviour of ai search choose shorter corner, moved unchanged from the original free function.
 *
 * @address 0x43d240
 */
uint8_t AiSearchGeometry::choose_shorter_corner(real_point2d *p, real_point2d *corner_a, real_point2d *q, real_point2d *corner_b, real_point2d *r, real_point2d *out_point)
{
    real_vector2d a_p;
    real_vector2d a_q;
    real_vector2d a_r;
    real_vector2d b_p;
    real_vector2d b_q;
    real_vector2d b_r;
    float turn_a;
    float turn_b;

    ai_search_corner_direction(p, corner_a, &a_p);
    ai_search_corner_direction(q, corner_a, &a_q);
    ai_search_corner_direction(r, corner_a, &a_r);
    ai_search_corner_direction(p, corner_b, &b_p);
    ai_search_corner_direction(q, corner_b, &b_q);
    ai_search_corner_direction(r, corner_b, &b_r);
    turn_a = halo::math::vector2d_angle_between(a_r, a_q);
    turn_a = halo::math::vector2d_angle_between(a_q, a_p) + turn_a;
    turn_b = halo::math::vector2d_angle_between(b_r, b_q);
    turn_b = halo::math::vector2d_angle_between(b_q, b_p) + turn_b;
    if (-turn_b < turn_a) {
        *out_point = *corner_a;
        return 1;
    }
    *out_point = *corner_b;
    return 0;
}

/**
 * Behaviour of ai search compute point tangents, moved unchanged from the original free function.
 *
 * @address 0x43c9a0
 */
void ObstacleList::compute_point_tangents(int16_t point_index, real_point2d *position, real_vector2d *edge_neg, float radius, real_vector2d *out_a, real *out_b)
{
    ai_search_obstacle_list * list = ptr;
    ai_search_obstacle *point = &list->obstacles[point_index];
    real_vector2d direction;
    float dx = point->position.x - position->x;
    float dy = point->position.y - position->y;
    float distance = (float)sqrt(dx * dx + dy * dy);

    if (fabs(distance) < (double)0.0001f) {
        distance = 0.0f;
    } else {
        double inv = 1.0 / distance;
        dx = (float)(dx * inv);
        dy = (float)(dy * inv);
    }
    direction.i = dx;
    direction.j = dy;

    halo::math::vector2d_tangent_edge_directions(direction, *out_a, *edge_neg, distance, radius + point->radius + 0.00390625f, *out_b);
}

/**
 * Behaviour of ai search context init, moved unchanged from the original free function.
 *
 * @address 0x43b790
 */
void AiSearch::context_init(uint8_t ignores_glass, uint32_t search_radius_bits, ai_search_obstacle_list *obstacles, real_point2d *origin, uint32_t structure_bsp, real_point2d *position, int32_t surface_index, uint32_t origin_surface_index, uint8_t final_leg, uint8_t ignore_flagged_obstacles)
{
    ai_search_context * context = ptr;
    int16_t covering_point;

    context->search_radius = search_radius_bits;
    context->structure_bsp = structure_bsp;
    context->ignores_glass = ignores_glass;
    context->obstacles = (uint32_t)(uintptr_t)obstacles;
    context->complete = 0;
    context->origin = *origin;
    context->origin_surface_index = origin_surface_index;

    covering_point = halo::ai::ai_search_find_covering_point(obstacles, origin, -1, *(float *)&search_radius_bits);
    context->goal_point_id = (covering_point == -1) ? -1 : obstacles->obstacles[covering_point].link;

    context->final_leg = final_leg;
    context->ignore_flagged_obstacles = ignore_flagged_obstacles;
    context->result_node = -1;
    context->best_cost = 3.4028235e+38f;
    context->best_node = -1;
    context->node_count = 0;
    context->heap_count = 0;

    halo::ai::ai_search_add_node(context, -1, position, surface_index, -1, 0, 0.0f); // "z" is the start surface
}

/**
 * Behaviour of ai search evaluate edge cost, moved unchanged from the original free function.
 *
 * @address 0x43b830
 */
uint8_t AiSearchGeometry::evaluate_edge_cost(void *context, uint8_t ignore_permission, ai_search_obstacle_list *obstacle_list, int16_t exclude_index, real_point2d *point, int32_t start_surface_index, float distance, float base_cost, uint8_t skip_direct, uint8_t apply_offset, uint8_t require_unflagged, ai_search_edge_result *out_result, real_vector2d *direction)
{
    path_find_boundary_trace_result trace;
    ai_search_nearest_point_result nearest;
    real_vector2d perpendicular;
    real_point2d offset;
    uint8_t hit;

    out_result->cost = base_cost;
    out_result->surface_index = -1;
    out_result->edge_index = -1;
    out_result->point_id = -1;
    out_result->link = -1;
    if (apply_offset) {
        out_result->cost = base_cost - distance;
    }
    if (!skip_direct) {
        if (halo::ai::path_find_trace_cluster_boundary_from_vertex(context, ignore_permission, point, start_surface_index,
                direction, out_result->cost, &trace) != 0 && out_result->cost > trace.distance) {
            out_result->cost = trace.distance;
            out_result->edge_index = trace.edge_index;
        }
        perpendicular.i = -direction->j;
        perpendicular.j = direction->i;
        offset.x = perpendicular.i * distance + point->x;
        offset.y = perpendicular.j * distance + point->y;
        halo::ai::path_find_trace_cluster_boundary_from_vertex(context, ignore_permission, point, start_surface_index,
            &perpendicular, distance, &trace);
        if (halo::ai::path_find_trace_cluster_boundary_from_vertex(context, ignore_permission, &offset, trace.surface_index,
                direction, out_result->cost, &trace) != 0 && out_result->cost > trace.distance) {
            out_result->cost = trace.distance;
            out_result->edge_index = trace.edge_index;
        }
        offset.x = perpendicular.i * -distance + point->x;
        offset.y = -distance * perpendicular.j + point->y;
        halo::ai::path_find_trace_cluster_boundary_from_vertex(context, ignore_permission, point, start_surface_index,
            &perpendicular, distance, &trace);
        if (halo::ai::path_find_trace_cluster_boundary_from_vertex(context, ignore_permission, &offset, trace.surface_index,
                direction, out_result->cost, &trace) != 0 && out_result->cost > trace.distance) {
            out_result->cost = trace.distance;
            out_result->edge_index = trace.edge_index;
        }
    }
    if (halo::ai::ai_search_find_nearest_visible_point(obstacle_list, exclude_index, point, direction, distance,
            out_result->cost, require_unflagged, &nearest) != 0 && out_result->cost > nearest.distance) {
        out_result->cost = nearest.distance;
        out_result->edge_index = -1;
        out_result->point_id = nearest.point_id;
        out_result->link = nearest.link;
    }
    if (out_result->edge_index == -1 && out_result->point_id == -1) {
        out_result->cost = base_cost;
        hit = 0;
    } else {
        hit = 1;
    }
    halo::ai::path_find_trace_cluster_boundary_from_vertex(context, ignore_permission, point, start_surface_index, direction,
        out_result->cost, &trace);
    out_result->surface_index = trace.surface_index;
    return hit;
}

/**
 * Behaviour of ai search expand point neighbors, moved unchanged from the original free function.
 *
 * @address 0x43ba60
 */
void AiSearch::expand_point_neighbors(int16_t node_index, int16_t start_point_id)
{
    ai_search_context * context = ptr;
    ai_search_obstacle_list *list = (ai_search_obstacle_list *)(uintptr_t)context->obstacles;
    void *map = (void *)(uintptr_t)context->structure_bsp;
    float radius = *(float *)&context->search_radius;
    ai_search_node *node = &context->nodes[node_index];
    uint32_t visited[8];
    int16_t worklist[0x78];
    int16_t pending = 1;

    memset(visited, 0, ((list->count + 0x1f) >> 5) * 4);
    visited[start_point_id >> 5] |= 1u << (start_point_id & 0x1f);
    worklist[0] = start_point_id;
    do {
        int16_t point = worklist[--pending];
        int16_t link = (point != -1) ? list->obstacles[point].link : -1;
        real_vector2d directions[2];
        float tangent_distance;
        int16_t side;

        halo::ai::ai_search_compute_point_tangents(list, point, &node->position, &directions[0], radius, &directions[1],
            &tangent_distance);
        if (tangent_distance < radius) {
            tangent_distance = radius;
        }
        for (side = 0; side < 2; side++) {
            ai_search_edge_result edge;

            halo::ai::ai_search_evaluate_edge_cost(map, context->ignores_glass, list, point, &node->position,
                *(int32_t *)&node->z, radius, radius + radius + tangent_distance, 0, 0, context->ignore_flagged_obstacles, &edge,
                &directions[side]);
            if (edge.point_id != -1 && (visited[edge.point_id >> 5] & (1u << (edge.point_id & 0x1f))) == 0) {
                visited[edge.point_id >> 5] |= 1u << (edge.point_id & 0x1f);
                worklist[pending++] = edge.point_id;
            }
            if (edge.cost > tangent_distance && edge.link != link) {
                float half = (edge.cost + tangent_distance) * 0.5f;
                path_find_boundary_trace_result trace;
                real_point2d position;

                halo::ai::path_find_trace_cluster_boundary_from_vertex(map, context->ignores_glass, &node->position,
                    *(int32_t *)&node->z, &directions[side], half, &trace);
                position.x = half * directions[side].i + node->position.x;
                position.y = half * directions[side].j + node->position.y;
                halo::ai::ai_search_add_node(context, node_index, &position, trace.surface_index, link, (uint8_t)side,
                    (node->cost - node->length) + half);
            }
        }
    } while (pending > 0);
}

/**
 * Behaviour of ai search find circle portal crossing, moved unchanged from the original free function.
 *
 * @address 0x43d100
 */
void AiSearchGeometry::find_circle_portal_crossing(real_point2d *center, real_point2d *portal, real_point2d *out_point, real_point2d *fallback_reference, float radius)
{
    float denom = (portal[1].y - center->y) * (portal[0].x - center->x) -
                  (portal[0].y - center->y) * (portal[1].x - center->x);

    if (0.0001 <= fabs(denom)) {
        float scale = (radius * radius) / denom;
        float y = ((portal[0].x - center->x) - (portal[1].x - center->x)) * scale + center->y;
        out_point->x = center->x - ((portal[0].y - center->y) - (portal[1].y - center->y)) * scale;
        out_point->y = y;

        {
            float dy = y - center->y;
            if ((out_point->x - center->x) * (out_point->x - center->x) + dy * dy <= radius * radius * 4.0f) {
                return;
            }
        }
    }

    {
        float dx = portal[0].x - fallback_reference->x;
        float dy = portal[0].y - fallback_reference->y;
        float len = (float)sqrt(dx * dx + dy * dy);
        if ((0.0001 <= fabs(len)) && (len != 0.0f)) {
            dx = dx * (1.0f / len);
            dy = dy * (1.0f / len);
        } else {
            dx = ai_default_2d_direction->x;
            dy = ai_default_2d_direction->y;
        }
        out_point->x = dx * radius + portal[0].x;
        out_point->y = dy * radius + portal[0].y;
    }
}

/**
 * Behaviour of ai search find circle tangent point, moved unchanged from the original free function.
 *
 * @address 0x43cf60
 */
void AiSearchGeometry::find_circle_tangent_point(real_point2d *center, real_point2d *target, real_point2d *out_point, float radius, uint8_t side)
{
    float dx = target->x - center->x;
    float dy = target->y - center->y;
    float dist2 = dy * dy + dx * dx;
    float inv = radius / dist2;
    float discriminant = dist2 - radius * radius;

    if (0.0f < discriminant) {
        float root = (float)sqrt(discriminant);
        float tangent_pts[4];
        float cross_dy = dy * root;
        float cross_dx = dx * root;
        uint32_t which;

        tangent_pts[0] = (dx * radius + cross_dy) * inv + center->x;
        tangent_pts[1] = (dy * radius - cross_dx) * inv + center->y;
        tangent_pts[2] = (dx * radius - cross_dy) * inv + center->x;
        tangent_pts[3] = (cross_dx + dy * radius) * inv + center->y;

        which = (uint32_t)(0.0f < (tangent_pts[3] - target->y) * (tangent_pts[0] - target->x) -
                         (tangent_pts[2] - target->x) * (tangent_pts[1] - target->y)) != (uint32_t)side;

        out_point->x = tangent_pts[which * 2];
        out_point->y = tangent_pts[which * 2 + 1];
        return;
    }

    {
        float len = (float)sqrt(dx * dx + dy * dy);
        if ((0.0001 <= fabs(len)) && (len != 0.0f)) {
            dx = (1.0f / len) * dx;
            dy = dy * (1.0f / len);
        } else {
            dx = ai_default_2d_direction->x;
            dy = ai_default_2d_direction->y;
        }
        out_point->x = dx * radius + center->x;
        out_point->y = dy * radius + center->y;
    }
}

/**
 * Behaviour of ai search find covering point, moved unchanged from the original free function.
 *
 * @address 0x43c890
 */
int16_t ObstacleList::find_covering_point(real_point2d *position, int16_t exclude_index, float extra_radius)
{
    ai_search_obstacle_list * list = ptr;
    int16_t i;

    for (i = 0; i < list->count; i = i + 1) {
        if (i != exclude_index) {
            float radius = extra_radius + list->obstacles[i].radius;
            float dx = list->obstacles[i].position.x - position->x;
            float dy = list->obstacles[i].position.y - position->y;
            if (dy * dy + dx * dx <= radius * radius) {
                return i;
            }
        }
    }
    return -1;
}

/**
 * Behaviour of ai search find nearest visible point, moved unchanged from the original free function.
 *
 * @address 0x43c8f0
 */
uint8_t ObstacleList::find_nearest_visible_point(int16_t exclude_index, real_point2d *origin, real_vector2d *direction, float radius, float max_distance, uint8_t require_unflagged, ai_search_nearest_point_result *out_result)
{
    ai_search_obstacle_list * list = ptr;
    float distance = max_distance; // 0x43c942: the max_distance slot doubles as the ray's out distance
    int16_t i;

    out_result->distance = max_distance;
    out_result->point_id = -1;
    out_result->link = -1;
    for (i = 0; i < list->count; i++) {
        ai_search_obstacle *obstacle = &list->obstacles[i];

        if (i == exclude_index || (require_unflagged && (obstacle->flags & 1) != 0)) {
            continue;
        }
        if (halo::math::ray2d_intersect_circle_distance(*direction, *origin, obstacle->position, distance,
                radius + obstacle->radius) != 0 &&
            out_result->distance > distance) {
            out_result->distance = distance;
            out_result->point_id = i;
            out_result->link = obstacle->link;
        }
    }
    return (uint8_t)(out_result->point_id != -1);
}

/**
 * Behaviour of ai search flood fill group, moved unchanged from the original free function.
 *
 * @address 0x43ca40
 */
void ObstacleList::flood_fill_group(float radius, uint32_t *out_bitmask, int16_t start_index)
{
    ai_search_obstacle_list * list = ptr;
    int16_t worklist[128];
    int16_t worklist_count;
    int32_t words = (list->count + 0x1f) >> 5;
    int32_t i;

    for (i = 0; i < words; i = i + 1) {
        out_bitmask[i] = 0;
    }

    if (start_index != -1) {
        worklist[0] = start_index;
        worklist_count = 1;
        out_bitmask[start_index >> 5] |= 1u << (start_index & 0x1f);

        do {
            int16_t j;
            worklist_count = worklist_count - 1;
            {
                ai_search_obstacle *cur = &list->obstacles[worklist[worklist_count]];

                for (j = 0; j < list->count; j = j + 1) {
                    uint32_t bit = 1u << (j & 0x1f);
                    if ((out_bitmask[j >> 5] & bit) == 0) {
                        float combined_radius = (radius + list->obstacles[j].radius) + (radius + cur->radius);
                        float dx = list->obstacles[j].position.x - cur->position.x;
                        float dy = list->obstacles[j].position.y - cur->position.y;
                        if (dy * dy + dx * dx <= combined_radius * combined_radius) {
                            out_bitmask[j >> 5] |= bit;
                            worklist[worklist_count] = j;
                            worklist_count = worklist_count + 1;
                        }
                    }
                }
            }
        } while (0 < worklist_count);
    }
}

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)halo::objects::globals().object_data->data)[(h) & halo::k_slot_mask].data)
#define TAG_DATA(t) ((uint8_t *)halo::cache::globals().tag_instances[(t) & halo::k_slot_mask].data)
/**
 * Behaviour of ai search gather obstacles, moved unchanged from the original free function.
 *
 * @address 0x43c510
 */
void ObstacleList::gather_obstacles(real_point3d *center, float radius, real_vector3d *direction, uint32_t self_object_a, uint32_t self_object_b)
{
    ai_search_obstacle_list * list = ptr;
    datum_index found[0x100];
    int16_t count;
    int16_t f;

    count = halo::objects::object_find_in_sphere(1, 0xc3, OBJECT_DATA(self_object_a) + 0x98, center, radius, found, 0x100);
    for (f = 0; f < count; f++) {
        datum_index object_index = found[f];
        uint8_t *object = OBJECT_DATA(object_index);
        uint8_t *object_tag;
        uint8_t *collision;
        real_matrix4x3 world;
        int32_t s;

        if (object_index == self_object_a || object_index == self_object_b || (object[0x10] & 1) != 0) {
            continue;
        }
        if (((struct object *)object)->type == 0 && (object[0x106] & 4) != 0) {
            continue;
        }
        if (((struct object *)object)->type == 7) {
            uint16_t machine_flags = *(uint16_t *)(TAG_DATA(*(datum_index *)object) + 0x292);

            if ((machine_flags & 1) == 0) {
                continue;
            }
            if ((machine_flags & 2) != 0 && *(float *)(object + 0x208) == 1.0f) {
                continue;
            }
        }
        if (!halo::math::point3d_within_radius(*(real_point3d *)(object + 0xa0), *center, radius + ((struct object *)object)->bounding_radius)) {
            continue;
        }
        object_tag = TAG_DATA(*(datum_index *)object);
        collision = TAG_DATA(*(datum_index *)(object_tag + 0x7c));
        if ((object_tag[2] & 8) != 0 || *(int32_t *)(collision + 0x280) <= 0) {
            continue;
        }
        halo::objects::object_get_world_matrix(object_index, &world);
        for (s = 0; s < *(int32_t *)(collision + 0x280); s++) {
            uint8_t *sphere = *(uint8_t **)(collision + 0x284) + s * 0x20;
            int16_t node = *(int16_t *)sphere;
            real_point3d point;
            float sphere_radius;
            float dx;
            float dy;
            float dz;
            float reach;
            uint16_t flags = 0;

            object = OBJECT_DATA(object_index);
            if (node != -1) {
                real_matrix4x3 *matrix = (real_matrix4x3 *)(object + ((struct object *)object)->nodes.offset + node * 0x34);

                halo::math::matrix4x3_transform_point(point, *(real_point3d *)(sphere + 0x10), *matrix);
                sphere_radius = *(float *)(sphere + 0x1c) * matrix->scale;
            } else {
                halo::math::matrix4x3_transform_point(point, *(real_point3d *)(sphere + 0x10), world);
                sphere_radius = world.scale * *(float *)(sphere + 0x1c);
            }
            if (!(point.z + sphere_radius + 0.5f >= center->z) && direction->k > -0.2f) {
                continue;
            }
            if (point.z - sphere_radius - 0.5f > center->z && direction->k < 0.2f) {
                continue;
            }
            dx = point.x - center->x;
            dy = point.y - center->y;
            dz = point.z - center->z;
            reach = sphere_radius + radius;
            if (reach * reach < dz * dz * 4.0f + dy * dy + dx * dx) {
                continue;
            }
            if (((struct object *)object)->type == 0 && dy * direction->j + dx * direction->i + dz * direction->k > 0.0f &&
                ((struct object *)object)->velocity.k * direction->k + ((struct object *)object)->velocity.j * direction->j +
                        ((struct object *)object)->velocity.i * direction->i > 0.06666667f) {
                flags = 1;
            }
            halo::ai::ai_search_append_obstacle(list, flags, object_index, (real_point2d *)&point, sphere_radius);
        }
    }
}

#undef OBJECT_DATA
#undef TAG_DATA

/**
 * Behaviour of ai search heap sift down, moved unchanged from the original free function.
 *
 * @address 0x43b4d0
 */
void AiSearch::heap_sift_down(int16_t index)
{
    ai_search_context * context = ptr;
    int16_t count = ((struct ai_search_context *)context)->heap_count;
    int16_t left, right, smallest;

    if (index < count) {
        for (;;) {
            left = index * 2 + 1;
            right = index * 2 + 2;
            smallest = index;

            if ((left < count) &&
                (context->nodes[context->heap[left]].cost < context->nodes[context->heap[index]].cost)) {
                smallest = left;
            }
            if ((right < count) &&
                (context->nodes[context->heap[right]].cost < context->nodes[context->heap[smallest]].cost)) {
                smallest = right;
            }
            if (smallest == index) {
                break;
            }

            {
                int16_t tmp = context->heap[smallest];
                context->heap[smallest] = context->heap[index];
                context->heap[index] = tmp;
            }
            index = smallest;
        }
    }
}

/**
 * Behaviour of ai search heap sift up, moved unchanged from the original free function.
 *
 * @address 0x43b450
 */
void AiSearch::heap_sift_up(int16_t index)
{
    ai_search_context * context = ptr;
    int16_t parent;
    int16_t node;
    int16_t parent_node;

    while (0 < index) {
        parent = (index - 1) >> 1;
        node = context->heap[index];
        parent_node = context->heap[parent];
        if (context->nodes[parent_node].cost <= context->nodes[node].cost) {
            break;
        }
        context->heap[index] = parent_node;
        context->heap[parent] = node;
        index = parent;
    }
}

/**
 * Behaviour of ai search partition into groups, moved unchanged from the original free function.
 *
 * @address 0x43cb60
 */
void ObstacleList::partition_into_groups(float radius)
{
    ai_search_obstacle_list * list = ptr;
    uint32_t group_bitmask[4];
    int16_t i;

    list->group_count = 0;
    for (i = 0; i < list->count; i = i + 1) {
        list->obstacles[i].link = -1;
    }

    for (i = 0; i < list->count; i = i + 1) {
        if (list->obstacles[i].link == -1) {
            int16_t group_id = list->group_count;
            int16_t j;

            list->group_count = group_id + 1;
            halo::ai::ai_search_flood_fill_group(list, radius, group_bitmask, i);

            for (j = 0; j < list->count; j = j + 1) {
                if ((group_bitmask[j >> 5] & (1u << (j & 0x1f))) != 0) {
                    list->obstacles[j].link = group_id;
                }
            }
        }
    }
}

/**
 * Behaviour of ai search run, moved unchanged from the original free function.
 *
 * @address 0x43be20
 */
uint8_t AiSearch::run(uint8_t ignores_glass, ai_search_obstacle_list *obstacles, uint32_t search_radius_bits, real_point2d *position, int32_t surface_index, real_point2d *origin, uint32_t origin_surface_index, uint8_t final_leg, uint8_t ignore_flagged_obstacles)
{
    ai_search_context * context = ptr;
    halo::ai::ai_search_context_init(context, ignores_glass, search_radius_bits, obstacles, origin,
        (uint32_t)halo::scenario::globals().structure_bsp, position, surface_index, origin_surface_index, final_leg, ignore_flagged_obstacles);

    while (halo::ai::ai_search_step(context) != 0) {
    }

    if (context->result_node != -1) {
        context->complete = 1;
        return context->result_node != -1;
    }

    if (context->best_node != -1) {
        context->result_node = context->best_node;
    }
    return context->result_node != -1;
}

/**
 * Behaviour of ai search step, moved unchanged from the original free function.
 *
 * @address 0x43bcb0
 */
uint8_t AiSearch::step()
{
    ai_search_context * context = ptr;
    if (context->heap_count > 0) {
        int16_t index;

        context->heap_count--;
        index = context->heap[0];
        context->heap[0] = context->heap[context->heap_count];
        halo::ai::ai_search_heap_sift_down(context, 0);
        if (index != -1) {
            ai_search_node *node = &context->nodes[index];
            ai_search_edge_result edge;

            halo::ai::ai_search_evaluate_edge_cost((void *)(uintptr_t)context->structure_bsp, context->ignores_glass,
                (ai_search_obstacle_list *)(uintptr_t)context->obstacles, -1, &node->position, *(int32_t *)&node->z,
                *(float *)&context->search_radius, node->length, (uint8_t)(node->parent == -1), 1, context->ignore_flagged_obstacles,
                &edge, &node->direction);
            if (edge.edge_index == -1) {
                if (edge.point_id == -1) {
                    if (edge.surface_index == (int32_t)context->origin_surface_index ||
                        halo::ai::path_find_heights_are_close((ScenarioStructureBSP *)(uintptr_t)context->structure_bsp,
                            &context->origin, (int32_t)context->origin_surface_index, edge.surface_index)) {
                        real_point2d position;

                        position.x = edge.cost * node->direction.i + node->position.x;
                        position.y = edge.cost * node->direction.j + node->position.y;
                        context->result_node = halo::ai::ai_search_add_node(context, index, &position, edge.surface_index, -1, 0,
                            (node->cost - node->length) + edge.cost);
                    }
                } else {
                    if (edge.link == context->goal_point_id && node->length < context->best_cost) {
                        context->best_cost = node->length;
                        context->best_node = index;
                    }
                    halo::ai::ai_search_expand_point_neighbors(context, index, edge.point_id);
                }
            }
        }
    }
    return (uint8_t)(context->result_node == -1 && context->heap_count > 0);
}

}
