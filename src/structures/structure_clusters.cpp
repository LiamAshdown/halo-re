/**
 * @file src/structures/structure_clusters.cpp
 * Cluster portal flooding and the per-cluster object reference chains.
 * The original author notes and decompiles are in docs/original/structures/.
 */

#include "halo/structures/structures.hpp"
#include "halo/math/api.hpp"
#include "halo/memory/api.hpp"

extern "C" {
extern ScenarioStructureBSP *global_structure_bsp;
extern int32_t render_cluster_index;
extern uint32_t *flood_recursion_bits;
extern uint32_t cluster_visible_bits[halo::structures::k_cluster_visible_bit_words];
extern int16_t visible_cluster_count;
extern int16_t cluster_visible_index[halo::structures::k_maximum_flood_clusters];
extern real_bounds *k_default_screen_bounds;
extern structure_bsp_visible_cluster visible_clusters[k_maximum_visible_clusters];
extern uint8_t render_cluster_has_sky;
extern float portal_visibility_tolerance;
extern int16_t halo::math::polygon2d_clip_to_planes(int16_t vertex_count, real_point2d *vertices,
                                         int16_t clip_point_count, real_point2d *clip_points,
                                         int16_t maximum_count, real_point2d *out,
                                         real epsilon);
extern int32_t cluster_flood_stamp;
extern uint8_t cluster_flood_in_progress;
extern int32_t cluster_visit_stamp[halo::structures::k_maximum_flood_clusters];
extern uint8_t halo::math::vector3d_projection_band_test(real_vector3d *axis, real_point3d *point_a, real_point3d *point_b,
    real radius, real max_distance, real sin_angle, real cos_angle);
extern void halo::math::matrix4x3_transform_point(real_point3d *out, real_point3d *point,
                                       real_matrix4x3 *m);
extern int16_t halo::math::polygon3d_clip_to_plane(int16_t count, real_point3d *in, real_plane3d *plane, int16_t max_count, real_point3d *out, uint8_t *clipped_flag, real epsilon, char keep_coplanar);
extern real_plane3d near_clip_plane;
extern double k_plane_side_epsilon;
extern float k_projection_numerator;
extern real_point3d render_camera_global;
extern uint8_t render_frustum_global;
extern uint8_t *game_state_base;
extern int32_t game_state_cursor;
extern uint32_t game_state_crc;
extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size);
}

namespace halo::structures {

void cluster_flood::camera_portal_flood_recursive(int16_t cluster_index, polygon2d *view_polygon)
{
    ScenarioStructureBSPCluster *cluster =
        &((ScenarioStructureBSPCluster *)global_structure_bsp->clusters.pointer)[cluster_index];

    uint32_t bit = bit_array_mask(cluster_index);
    int32_t word = bit_array_word(cluster_index);
    flood_recursion_bits[word] |= bit;

    if ((cluster_visible_bits[word] & bit) == 0) {
        int16_t visible_index = visible_cluster_count;
        cluster_visible_index[cluster_index] = visible_index;
        visible_cluster_count++;
        visible_clusters[visible_index].cluster_index = cluster_index;
        visible_clusters[visible_index].screen_bounds_x = k_default_screen_bounds[0];
        visible_clusters[visible_index].screen_bounds_y = k_default_screen_bounds[1];
    }
    globals().cluster_visible_bits[word] |= bit;

    int16_t visible_index = globals().cluster_visible_index[cluster_index];
    bsp_bounds::polygon2d_bounds_expand((real_bounds *)&globals().visible_clusters[visible_index].screen_bounds_x, view_polygon);

    ScenarioStructureBSPClusterPortalIndex *portal_refs =
        (ScenarioStructureBSPClusterPortalIndex *)cluster->portals.pointer;
    ScenarioStructureBSPClusterPortal *portals =
        (ScenarioStructureBSPClusterPortal *)global_structure_bsp->cluster_portals.pointer;

    for (int32_t i = 0; i < (int32_t)cluster->portals.count; i++) {
        ScenarioStructureBSPClusterPortal *portal = &portals[portal_refs[i].portal];
        uint8_t same_side = (portal->front_cluster == (uint16_t)cluster_index);
        int16_t neighbor = same_side ? (int16_t)portal->back_cluster
                                      : (int16_t)portal->front_cluster;
        if (neighbor < 0 || neighbor >= global_structure_bsp->clusters.count) {
            continue;
        }
        uint32_t neighbor_bit = bit_array_mask(neighbor);
        int32_t neighbor_word = neighbor >> 5;
        if ((globals().flood_recursion_bits[neighbor_word] & neighbor_bit) != 0) {
            continue;
        }

        int32_t row_dwords = bit_array_word_count(global_structure_bsp->clusters.count);
        uint32_t *pvs_row = (uint32_t *)((uint8_t *)global_structure_bsp->cluster_data.pointer +
                                          globals().render_cluster_index * row_dwords * 4);
        if ((pvs_row[neighbor_word] & neighbor_bit) == 0) {
            continue;
        }

        polygon2d portal_polygon;
        polygon2d clipped_polygon;
        uint8_t project_result =
            cluster_flood::portal_test_and_project(same_side, portal_refs[i].portal, &portal_polygon);
        polygon2d *next_polygon = view_polygon;
        if (project_result != 2) {
            if (project_result != 0 ||
                (globals().render_cluster_has_sky == 0 &&

                 structure_bsp_query::points_within_band((real_point3d *)portal->vertices.pointer, (int16_t)portal->vertices.count, portal_visibility_tolerance) == 0)) {
                continue;
            }

            int16_t clipped_count = halo::math::polygon2d_clip_to_planes(
                portal_polygon.point_count, &portal_polygon.points[0],
                view_polygon->point_count, &view_polygon->points[0], k_maximum_clip_polygon_points,
                &clipped_polygon.points[0], 9.99999975e-05f);
            clipped_polygon.point_count = clipped_count;
            if (clipped_count < 1) {
                if (clipped_count != -1) {
                    continue;
                }
            } else {
                next_polygon = &clipped_polygon;
            }
        }
        cluster_flood::camera_portal_flood_recursive(neighbor, next_polygon);
    }

    globals().flood_recursion_bits[word] &= ~bit;
}

int16_t cluster_flood::fill_with_predicate(real_point3d *position, real_vector3d *facing, real max_distance, real sin_angle, real cos_angle, int16_t max_count, int16_t *output, int16_t start_cluster)
{
    int16_t stack[k_maximum_flood_clusters];
    int16_t stack_top = 1;
    int16_t written = 0;

    globals().cluster_flood_stamp++;
    globals().cluster_flood_in_progress = 1;
    globals().cluster_visit_stamp[start_cluster] = globals().cluster_flood_stamp;
    stack[0] = start_cluster;

    do {
        int16_t cluster_index;
        ScenarioStructureBSPCluster *cluster;
        int32_t portal_count;
        int16_t *portal_indices;
        int16_t i;

        if (written >= max_count) {
            break;
        }
        cluster_index = stack[--stack_top];
        cluster = (ScenarioStructureBSPCluster *)global_structure_bsp->clusters.pointer + cluster_index;
        output[written++] = cluster_index;
        portal_count = (int32_t)cluster->portals.count;
        portal_indices = (int16_t *)cluster->portals.pointer;

        for (i = 0; i < portal_count; i++) {
            ScenarioStructureBSPClusterPortal *portal =
                (ScenarioStructureBSPClusterPortal *)global_structure_bsp->cluster_portals.pointer + portal_indices[i];
            int16_t neighbor = ((int16_t)portal->front_cluster == cluster_index) ? (int16_t)portal->back_cluster : (int16_t)portal->front_cluster;

            if (globals().cluster_visit_stamp[neighbor] == globals().cluster_flood_stamp) {
                continue;
            }
            if (!halo::math::vector3d_projection_band_test(facing, position, (real_point3d *)&portal->centroid,
                    portal->bounding_radius, max_distance, sin_angle, cos_angle)) {
                continue;
            }
            globals().cluster_visit_stamp[neighbor] = globals().cluster_flood_stamp;
            stack[stack_top++] = neighbor;
        }
    } while (stack_top > 0);

    globals().cluster_flood_in_progress = 0;
    return written;
}

int32_t cluster_flood::fill_within_radius(int16_t cluster_index, real_point3d *point, float tolerance, int32_t remaining_budget, int16_t *output)
{
    ScenarioStructureBSPCluster *cluster =
        &((ScenarioStructureBSPCluster *)global_structure_bsp->clusters.pointer)[cluster_index];

    if (remaining_budget > 0) {
        *output++ = cluster_index;
    }
    if (globals().cluster_visit_stamp[cluster_index] != globals().cluster_flood_stamp) {
        globals().cluster_visit_stamp[cluster_index] = globals().cluster_flood_stamp;
    }

    int32_t written = 1;
    int32_t next_budget = remaining_budget - 1;
    ScenarioStructureBSPClusterPortalIndex *portal_refs =
        (ScenarioStructureBSPClusterPortalIndex *)cluster->portals.pointer;
    ScenarioStructureBSPClusterPortal *portals =
        (ScenarioStructureBSPClusterPortal *)global_structure_bsp->cluster_portals.pointer;

    for (int32_t i = 0; i < (int32_t)cluster->portals.count; i++) {
        ScenarioStructureBSPClusterPortal *portal = &portals[portal_refs[i].portal];
        int16_t neighbor = (portal->front_cluster == (uint16_t)cluster_index)
                                ? (int16_t)portal->back_cluster
                                : (int16_t)portal->front_cluster;
        if (globals().cluster_visit_stamp[neighbor] == globals().cluster_flood_stamp) {
            continue;
        }
        if (!structure_bsp_view(global_structure_bsp).portal_sphere_test(point, portal_refs[i].portal, tolerance)) {
            continue;
        }
        int32_t child_count =
            cluster_flood::fill_within_radius(neighbor, point, tolerance, next_budget, output);
        written += child_count;
        next_budget -= child_count;
        output += child_count;
    }
    return written;
}

int32_t cluster_flood::seed(real_point3d *point, float radius, int16_t start_cluster, int16_t *output, int16_t max_count)
{
    if (start_cluster == -1) {
        return 0;
    }
    if (radius > 0.0f) {
        globals().cluster_flood_stamp++;
        globals().cluster_flood_in_progress = 1;
        int32_t count = cluster_flood::fill_within_radius(start_cluster, point, radius, max_count, output);
        globals().cluster_flood_in_progress = 0;
        return count;
    }
    if (max_count > 0) {
        *output = start_cluster;
        return 1;
    }
    return 0;
}

uint8_t cluster_flood::portal_project(real_plane3d *plane, void *camera_ref, real_point3d *vertices, void *camera, uint32_t vertex_count, int16_t winding, polygon2d *out)
{
    real_point3d *camera_position = (real_point3d *)camera_ref;
    real_point3d clipped[k_maximum_clip_polygon_points];
    float side;
    int16_t clipped_count;
    int16_t i, stop, step;
    int16_t written;
    uint32_t n;

    out->point_count = 0;

    side = ((camera_position->z * plane->normal.k + camera_position->y * plane->normal.j) +
            camera_position->x * plane->normal.i - plane->d) * (float)(int32_t)winding;
    if (*((int8_t *)camera_ref + 0x24) != 0) {
        winding = -winding;
    }
    if ((side < 0.0f ? -side : side) < k_plane_side_epsilon) {
        return 2;
    }
    if (side <= 0.0f) {
        return 1;
    }

    for (n = 0; (int16_t)vertex_count > 0 && n < (vertex_count & 0xffff); n = n + 1) {
        halo::math::matrix4x3_transform_point(clipped[n], vertices[n],
                                  *(real_matrix4x3 *)((uint8_t *)camera + 0x10));
    }

    clipped_count = halo::math::polygon3d_clip_to_plane(vertex_count, clipped, &near_clip_plane, k_maximum_clip_polygon_points,
                                            clipped, 0, 9.99999975e-05f, 1);
    out->point_count = clipped_count;

    if (winding == 1) {
        i = 0; stop = clipped_count; step = 1;
    } else {
        i = (int16_t)(clipped_count - 1); stop = -1; step = winding;
    }
    written = 0;
    while (i != stop) {
        float inverse_z = k_projection_numerator / clipped[i].z;
        out->points[written].x = inverse_z * clipped[i].x;
        out->points[written].y = inverse_z * clipped[i].y;
        i = (int16_t)(i + step);
        written = (int16_t)(written + 1);
    }
    return (uint8_t)(out->point_count < 3);
}

uint8_t cluster_flood::portal_test_and_project(char same_side, int16_t portal_index, polygon2d *out)
{
    ScenarioStructureBSPClusterPortal *portal =
        &((ScenarioStructureBSPClusterPortal *)global_structure_bsp->cluster_portals.pointer)[portal_index];

    ModelCollisionGeometryBSP *collision_bsp =
        (ModelCollisionGeometryBSP *)global_structure_bsp->collision_bsp.pointer;
    ModelCollisionGeometryBSPPlane *plane =
        &((ModelCollisionGeometryBSPPlane *)collision_bsp->planes.pointer)[portal->plane_index];
    return cluster_flood::portal_project((real_plane3d *)&plane->plane, &render_camera_global, (real_point3d *)portal->vertices.pointer, &render_frustum_global, portal->vertices.count, (int16_t)((same_side == 0) * 2 - 1), out);
}

int16_t cluster_flood::weather_polyhedra_find_within_radius(int16_t *out, float radius)
{
    ScenarioStructureBSP *bsp = global_structure_bsp;
    int16_t found = 0;
    int16_t index;

    for (index = 0; index < (int32_t)bsp->weather_polyhedra.count; index++) {
        ScenarioStructureBSPWeatherPolyhedron *polyhedron =
            (ScenarioStructureBSPWeatherPolyhedron *)(uintptr_t)bsp->weather_polyhedra.pointer + index;
        float dx = polyhedron->bounding_sphere_center.x - render_camera_global.x;
        float dy = polyhedron->bounding_sphere_center.y - render_camera_global.y;
        float dz = polyhedron->bounding_sphere_center.z - render_camera_global.z;
        float reach = radius + polyhedron->bounding_sphere_radius;

        if (dx * dx + dz * dz + dy * dy < reach * reach && found < 8) {
            out[found] = index;
            found++;
        }
    }
    return found;
}

void cluster_references::partition_new(cluster_reference_group *out, char *name)
{
    char format_buffer[256];
    char pool_name[256];
    int32_t size;
    uint8_t *region;

    region = game_state_base + game_state_cursor;
    game_state_cursor = game_state_cursor + k_maximum_cluster_object_references;
    size = k_maximum_cluster_object_references;
    halo::memory::crc32_update(&game_state_crc, (uint8_t *)&size, 4);
    out->cluster_first = (datum_index *)region;

    sprintf(format_buffer, "cluster %s", name);
    sprintf(pool_name, "%s reference", format_buffer);
    out->cluster_object_references = game_state_new(pool_name, k_maximum_cluster_object_references, sizeof(object_cluster_reference));

    sprintf(format_buffer, "%s cluster", name);
    sprintf(pool_name, "%s reference", format_buffer);
    out->object_cluster_references = game_state_new(pool_name, k_maximum_cluster_object_references, sizeof(object_cluster_reference));
}

void cluster_references::add_within_radius(uint32_t light_or_object_handle, datum_index *placement_slot, real_point3d *position, float radius, bsp_leaf_reference *leaf_and_cluster, cluster_reference_group *cluster_list)
{
    int16_t clusters[k_maximum_object_clusters];
    int16_t cluster_count;
    int16_t i;

    cluster_count = 0;
    if (leaf_and_cluster->cluster_index != -1) {
        if (radius <= 0.0f) {
            cluster_count = 1;
            clusters[0] = leaf_and_cluster->cluster_index;
        } else {
            cluster_flood_stamp = cluster_flood_stamp + 1;
            cluster_flood_in_progress = 1;
            cluster_count = cluster_flood::fill_within_radius(leaf_and_cluster->cluster_index, position, radius, k_maximum_object_clusters, clusters);
            cluster_flood_in_progress = 0;
        }
    }

    if (cluster_count > k_maximum_object_clusters) {
        cluster_count = k_maximum_object_clusters;
    }

    for (i = 0; i < cluster_count; i = i + 1) {
        int16_t cluster = clusters[i];
        datum_index handle;

        handle = halo::memory::datum_new(cluster_list->object_cluster_references);
        if (handle != k_datum_index_none) {
            object_cluster_reference *ref = (object_cluster_reference *)
                cluster_list->object_cluster_references->data + datum_slot(handle);
            ref->object_index = cluster;
            ref->next_reference = *placement_slot;
            *placement_slot = handle;
        }

        {
            datum_index *cluster_head = &cluster_list->cluster_first[cluster];
            handle = halo::memory::datum_new(cluster_list->cluster_object_references);
            if (handle != k_datum_index_none) {
                object_cluster_reference *ref = (object_cluster_reference *)
                    cluster_list->cluster_object_references->data + datum_slot(handle);
                ref->object_index = light_or_object_handle;
                ref->next_reference = *cluster_head;
                *cluster_head = handle;
            }
        }
    }
}

void cluster_references::remove_all(uint32_t handle, datum_index *link, cluster_reference_group *cluster_list)
{
    datum_index entry = *link;

    while (entry != k_datum_index_none) {
        object_cluster_reference *own_ref = (object_cluster_reference *)
            cluster_list->object_cluster_references->data + datum_slot(entry);
        int16_t cluster = (int16_t)own_ref->object_index;
        datum_index next_entry;

        halo::memory::datum_delete(cluster_list->object_cluster_references, entry);

        {
            datum_index *scan = &cluster_list->cluster_first[cluster];
            if (*scan != k_datum_index_none) {
                do {
                    object_cluster_reference *cluster_ref = (object_cluster_reference *)
                        cluster_list->cluster_object_references->data + datum_slot(*scan);
                    if (cluster_ref->object_index == handle) {
                        halo::memory::datum_delete(cluster_list->cluster_object_references, *scan);
                        *scan = cluster_ref->next_reference;
                        break;
                    }
                    scan = &cluster_ref->next_reference;
                } while (*scan != k_datum_index_none);
            }
        }

        next_entry = own_ref->next_reference;
        entry = next_entry;
    }

    *link = k_datum_index_none;
}

}  // namespace halo::structures
