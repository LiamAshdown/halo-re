/**
 * @file src/structures/structure_bsp_query.cpp
 * Spatial queries over the resident structure bsp: surfaces in a box, leaf walks and surface picking.
 * The original author notes and decompiles are in docs/original/structures/.
 */

#include "halo/structures/structures.hpp"

extern "C" {
extern ScenarioStructureBSP *global_structure_bsp;
extern uint32_t surface_visible_bits[k_maximum_visible_surface_bits];
extern float k_cluster_query_radius_threshold;
extern ModelCollisionGeometryBSP *global_collision_bsp;
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp,
                                      real_point3d *point);
extern real_point3d render_camera_global;
extern real_vector3d camera_forward_x;
extern uint8_t triangle_point_barycentric_2d(real_point3d *a, real_point3d *v_ecx, real_point3d *v_edx, real_point3d *p,
    real *out_u, real *out_v);
extern float k_surface_resolve_step;
extern uint8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin,
    real_vector3d *delta, uint32_t exclude_object_index, collision_result *result);
}

namespace halo::structures {

int16_t structure_bsp_query::node_query_recursive(int32_t node_index, real_rectangle3d *parent_bounds, uint32_t *visited_bits, int32_t *output_array, int32_t max_count, real_point3d *point, float radius, real_rectangle3d *query_box, int16_t plane_count, real_plane3d *planes, int16_t inherited_classification)
{
    ModelCollisionGeometryBSP *collision_bsp =
        (ModelCollisionGeometryBSP *)global_structure_bsp->collision_bsp.pointer;
    ScenarioStructureBSPNode *compressed_bounds =
        (ScenarioStructureBSPNode *)global_structure_bsp->nodes.pointer + node_index;
    real_rectangle3d node_bounds;
    int16_t classification = inherited_classification;
    int32_t written = 0;

    bsp_bounds::bsp3d_node_bounds_decompress(parent_bounds, (uint8_t *)compressed_bounds, &node_bounds);

    if (inherited_classification != _structure_bsp_overlap_contained) {
        structure_bsp_overlap box_result = bsp_bounds::aabb_overlap_classify(query_box, &node_bounds);
        classification = (int16_t)box_result;
        if (box_result != _structure_bsp_overlap_none) {
            structure_bsp_overlap plane_result =
                bsp_bounds::frustum_planes_classify_box(&node_bounds, planes, plane_count);
            if (plane_result == _structure_bsp_overlap_contained) {
                plane_count = 0;
            }
            classification = (int16_t)((plane_result < box_result) ? plane_result : box_result);
        }
    }

    if (classification != _structure_bsp_overlap_none) {
        ModelCollisionGeometryBSP3DNode *node =
            (ModelCollisionGeometryBSP3DNode *)collision_bsp->bsp3d_nodes.pointer + node_index;
        ModelCollisionGeometryBSPPlane *plane =
            (ModelCollisionGeometryBSPPlane *)collision_bsp->planes.pointer + node->plane;
        float distance = point->x * plane->plane.vector.i + point->y * plane->plane.vector.j +
            point->z * plane->plane.vector.k - plane->plane.w;

        uint8_t visit[2];
        uint32_t *children = &node->back_child;
        int32_t i;

        visit[0] = (uint8_t)!(radius <= distance);
        visit[1] = (uint8_t)!(distance <= -radius);

        for (i = 0; i < 2; i = i + 1) {
            if (visit[i]) {
                int32_t child = (int32_t)children[i];
                int16_t added;
                if (child < 0) {
                    if (child == -1) {
                        continue;
                    }
                    added = structure_bsp_query::leaf_query(child, classification, &node_bounds, visited_bits, output_array + written, max_count - written, query_box, plane_count, planes);
                } else {
                    added = structure_bsp_query::node_query_recursive(child, &node_bounds, visited_bits, output_array + written, max_count - written, point, radius, query_box, plane_count, planes, classification);
                }
                written = written + added;
            }
        }
    }

    return (int16_t)written;
}

int16_t structure_bsp_query::leaf_query(int32_t raw_child, int16_t inherited_classification, real_rectangle3d *parent_bounds, uint32_t *visited_bits, int32_t *output_array, int32_t max_count, real_rectangle3d *query_box, int16_t plane_count, real_plane3d *planes)
{
    int32_t leaf_index = raw_child & k_leaf_index_mask;
    ScenarioStructureBSPLeaf *leaf =
        &((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[leaf_index];
    int32_t written = 0;

    real_rectangle3d leaf_box;
    bsp_bounds::bsp3d_node_bounds_decompress(parent_bounds, (uint8_t *)leaf, &leaf_box);

    int16_t classification = inherited_classification;
    if (inherited_classification != 2) {
        structure_bsp_overlap aabb_result = bsp_bounds::aabb_overlap_classify(query_box, &leaf_box);
        structure_bsp_overlap frustum_result =
            bsp_bounds::frustum_planes_classify_box(&leaf_box, planes, plane_count);
        classification = (int16_t)((aabb_result <= frustum_result) ? aabb_result : frustum_result);
    }
    if (classification == 0) {
        return 0;
    }

    ScenarioStructureBSPSurfaceReference *leaf_surfaces =
        (ScenarioStructureBSPSurfaceReference *)global_structure_bsp->leaf_surfaces.pointer;
    int32_t first = leaf->surface_references;
    int32_t end = first + leaf->surface_reference_count;
    for (int32_t i = first; i < end; i++) {
        int32_t surface = leaf_surfaces[i].surface;
        int32_t word = bit_array_word(surface);
        uint32_t mask = bit_array_mask(surface);
        if ((surface_visible_bits[word] & mask) == 0) {
            continue;
        }
        if ((visited_bits[word] & mask) != 0) {
            continue;
        }
        if (written >= max_count) {
            break;
        }
        visited_bits[word] |= mask;
        output_array[written] = surface;
        written++;
    }
    return written;
}

int32_t structure_bsp_query::collect_surfaces_in_clusters(int32_t *out_surfaces, int16_t max_count, real_rectangle3d *query_box, int16_t plane_count, real_plane3d *planes, uint32_t *visited_bits, int16_t cluster_count, int16_t *cluster_indices)
{
    int16_t written = 0;

    for (int16_t c = 0; c < cluster_count; c++) {
        if (written >= max_count) {
            break;
        }
        ScenarioStructureBSPCluster *cluster =
            &((ScenarioStructureBSPCluster *)global_structure_bsp->clusters.pointer)[cluster_indices[c]];

        for (int32_t s = 0; s < (int32_t)cluster->subclusters.count; s++) {
            if (written >= max_count) {
                break;
            }
            ScenarioStructureBSPSubcluster *subcluster =
                &((ScenarioStructureBSPSubcluster *)cluster->subclusters.pointer)[s];

            if (bsp_bounds::aabb_overlap_classify((real_rectangle3d *)subcluster, query_box) ==
                    _structure_bsp_overlap_none) {
                continue;
            }
            if (bsp_bounds::frustum_planes_classify_box((real_rectangle3d *)subcluster, planes, plane_count) ==
                _structure_bsp_overlap_none) {
                continue;
            }
            int32_t *indices = (int32_t *)subcluster->surface_indices.pointer;
            for (int32_t k = 0; k < (int32_t)subcluster->surface_indices.count; k++) {
                int32_t surface = indices[k];
                int32_t word = surface >> 5;
                uint32_t mask = bit_array_mask(surface);
                if ((surface_visible_bits[word] & mask) != 0 && (visited_bits[word] & mask) == 0) {
                    if (written >= max_count) {
                        break;
                    }
                    visited_bits[word] |= mask;
                    out_surfaces[written] = surface;
                    written++;
                }
            }
        }
    }
    return written;
}

int16_t structure_bsp_query::query_surfaces(real_rectangle3d *query_box, real_point3d *query_point, int32_t *out_surfaces, int32_t max_count, float radius, int16_t plane_count, real_plane3d *planes, int16_t cluster_count, int16_t *cluster_indices)
{
    uint32_t visited_bits[k_maximum_visible_surface_bits];
    int32_t visited_dwords = bit_array_word_count(global_structure_bsp->surfaces.count);
    for (int32_t i = 0; i < visited_dwords; i++) {
        visited_bits[i] = 0;
    }

    real_rectangle3d built_box;
    if (query_box == 0) {
        built_box.x.lower = query_point->x - radius; built_box.x.upper = query_point->x + radius;
        built_box.y.lower = query_point->y - radius; built_box.y.upper = query_point->y + radius;
        built_box.z.lower = query_point->z - radius; built_box.z.upper = query_point->z + radius;
        query_box = &built_box;
    }

    if (radius >= k_cluster_query_radius_threshold) {
        if (cluster_indices != 0) {
            return (int16_t)structure_bsp_query::collect_surfaces_in_clusters(out_surfaces, (int16_t)max_count, query_box, plane_count, planes, visited_bits, cluster_count, cluster_indices);
        }

        int32_t leaf = bsp3d_node_find_leaf(0, global_collision_bsp,
                                             query_point);
        if (leaf != -1) {
            int32_t leaf_index = leaf & k_leaf_index_mask;
            uint16_t leaf_cluster =
                ((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[leaf_index].cluster;
            if (leaf_cluster != k_word_none) {
                int16_t flood_clusters[k_maximum_flood_clusters];
                int32_t flood_count = cluster_flood::seed(query_point, radius, (int16_t)leaf_cluster, flood_clusters, k_maximum_flood_clusters);
                return (int16_t)structure_bsp_query::collect_surfaces_in_clusters(out_surfaces, (int16_t)max_count, query_box, plane_count, planes, visited_bits, (int16_t)flood_count, flood_clusters);
            }
        }
    }

    return structure_bsp_query::node_query_recursive(0, (real_rectangle3d *)&global_structure_bsp->world_bounds_x, visited_bits, out_surfaces, max_count, query_point, radius, query_box, plane_count, planes, _structure_bsp_overlap_partial);
}

uint8_t structure_bsp_query::points_within_band(real_point3d *points, int16_t point_count, float tolerance)
{
    for (int16_t i = 0; i < point_count; i++) {
        real_point3d *p = &points[i];
        float distance = camera_forward_x.i * (p->x - render_camera_global.x) +
                          camera_forward_x.j * (p->y - render_camera_global.y) +
                          camera_forward_x.k * (p->z - render_camera_global.z);
        if (distance <= tolerance) {
            return 1;
        }
    }
    return 0;
}

uint8_t structure_bsp_query::leaf_find_material_surface(real_point3d *point, int32_t accepted_plane, int16_t *out_lightmap_index, int16_t *out_material_index, int32_t *out_surface, void *out_barycentric_u, void *out_barycentric_v, int32_t raw_child)
{
    int32_t leaf_index = raw_child & k_leaf_index_mask;
    ScenarioStructureBSPLeaf *leaf =
        &((ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer)[leaf_index];
    ScenarioStructureBSPSurfaceReference *leaf_surfaces =
        (ScenarioStructureBSPSurfaceReference *)global_structure_bsp->leaf_surfaces.pointer;
    ModelCollisionGeometryBSP *collision_bsp =
        (ModelCollisionGeometryBSP *)global_structure_bsp->collision_bsp.pointer;
    ModelCollisionGeometryBSP3DNode *bsp3d_nodes =
        (ModelCollisionGeometryBSP3DNode *)collision_bsp->bsp3d_nodes.pointer;
    ScenarioStructureBSPLightmap *lightmaps =
        (ScenarioStructureBSPLightmap *)global_structure_bsp->lightmaps.pointer;

    int32_t first = leaf->surface_references;
    int32_t end = first + leaf->surface_reference_count;
    int32_t i;

    for (i = first; i < end; i++) {
        int32_t node_value = leaf_surfaces[i].node;
        int32_t surface_index;
        ScenarioStructureBSPMaterial *material;
        ScenarioStructureBSPSurface *surface;
        real_point3d triangle[3];
        int32_t corner;

        if (node_value == -1 || (int32_t)bsp3d_nodes[node_value].plane != accepted_plane) {
            continue;
        }
        surface_index = leaf_surfaces[i].surface;
        surface = (ScenarioStructureBSPSurface *)global_structure_bsp->surfaces.pointer + surface_index;

        structure_bsp_view(global_structure_bsp).surface_material_locate(surface_index, out_material_index, out_lightmap_index);
        material = &((ScenarioStructureBSPMaterial *)
            lightmaps[*out_lightmap_index].materials.pointer)[*out_material_index];

        if (material->rendered_vertices_type == vertextype_structure_bsp_compressed_rendered_vertices) {
            uint8_t *vertices = (uint8_t *)material->compressed_vertices.pointer;
            for (corner = 0; corner < 3; corner = corner + 1) {
                float *v = (float *)(vertices + (&surface->vertex0_index)[corner] * sizeof(ScenarioStructureBSPMaterialCompressedRenderedVertex));
                triangle[corner].x = v[0];
                triangle[corner].y = v[1];
                triangle[corner].z = v[2];
            }
        } else if (material->rendered_vertices_type == vertextype_structure_bsp_uncompressed_rendered_vertices ||
                   material->rendered_vertices_type == k_vertex_type_uncompressed_rendered_alias) {
            uint8_t *vertices = (uint8_t *)material->uncompressed_vertices.pointer;
            for (corner = 0; corner < 3; corner = corner + 1) {
                float *v = (float *)(vertices + (&surface->vertex0_index)[corner] * sizeof(ScenarioStructureBSPMaterialUncompressedRenderedVertex));
                triangle[corner].x = v[0];
                triangle[corner].y = v[1];
                triangle[corner].z = v[2];
            }
        } else {
            continue;
        }

        if (triangle_point_barycentric_2d(&triangle[0], &triangle[2], &triangle[1], point,
                                          (real *)out_barycentric_u, (real *)out_barycentric_v)) {
            *out_surface = surface_index;
            return 1;
        }
    }
    return 0;
}

uint8_t structure_bsp_query::resolve_position_to_surface(real_point3d *start_position, real_point3d *position, int16_t *out_lightmap_index, void *out_barycentric_v, real_vector3d *direction, int16_t *out_material_index, int32_t *out_surface, void *out_barycentric_u)
{
    collision_result result;

    *position = *start_position;

    for (;;) {
        ScenarioStructureBSPLightmap *lightmaps;

        if (!collision_test_movement_segment(to_bits(k_structure_surface_query), position, direction, k_dword_none, &result)) {
            return 0;
        }
        *position = result.point;

        if (structure_bsp_query::leaf_find_material_surface(position, (int32_t)(result.plane_index & k_index_magnitude_mask), out_lightmap_index, out_material_index, out_surface, out_barycentric_u, out_barycentric_v, result.leaf.leaf_index)) {
            lightmaps = (ScenarioStructureBSPLightmap *)global_structure_bsp->lightmaps.pointer;
            if (lightmaps[*out_lightmap_index].bitmap != k_word_none) {
                return 1;
            }
        }

        if ((result.surface_flags & 1) == 0) {
            return 0;
        }
        position->x = direction->i * k_surface_resolve_step + position->x;
        position->y = direction->j * k_surface_resolve_step + position->y;
        position->z = direction->k * k_surface_resolve_step + position->z;
    }
}

uint8_t structure_bsp_query::cluster_sound_distance_lookup(int16_t cluster_a, int16_t cluster_b, ScenarioStructureBSP *structure_bsp)
{
    if (cluster_b != cluster_a) {
        int16_t lo = cluster_b;
        int16_t hi = cluster_a;
        int32_t index;
        uint8_t *sound_pas;

        if (hi < lo) {
            lo = cluster_a;
            hi = cluster_b;
        }

        index = (int32_t)(uint16_t)(structure_bsp->clusters.count - 1) * lo -
                ((int32_t)(lo + 1) * lo) / 2 - 1 + hi;
        sound_pas = (uint8_t *)structure_bsp->sound_pas_data.pointer;
        return sound_pas[index];
    }
    return 0;
}

}  // namespace halo::structures
