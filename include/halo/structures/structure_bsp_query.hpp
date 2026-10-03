/**
 * @file include/halo/structures/structure_bsp_query.hpp
 * Spatial queries over the resident structure bsp: surfaces in a box, leaf walks and surface picking.
 */
#pragma once

#include "halo/structures/structures_types.hpp"

namespace halo::structures {

/**
 * Spatial queries over the resident structure bsp. State is the engine's fixed globals (the bsp tag and the visible
 * surface bit set).
 */
struct structure_bsp_query {
    /**
     * Recursively descends the bsp3d node tree, decompressing each node's bounds, classifying them against the query box
     * and clip planes and descending into the children the query sphere can reach. Reached leaves append their unseen
     * surfaces to the output array.
     *
     * @address 0x553f10
     */
    static int16_t node_query_recursive(int32_t node_index, real_rectangle3d *parent_bounds, uint32_t *visited_bits, int32_t *output_array, int32_t max_count, real_point3d *point, float radius, real_rectangle3d *query_box, int16_t plane_count, real_plane3d *planes, int16_t inherited_classification);

    /**
     * Collects the surfaces of one bsp leaf that are inside the query box and clip planes and not yet marked visited,
     * appends them to the output array and returns how many were written.
     *
     * @address 0x5540c0
     */
    static int16_t leaf_query(int32_t raw_child, int16_t inherited_classification, real_rectangle3d *parent_bounds, uint32_t *visited_bits, int32_t *output_array, int32_t max_count, real_rectangle3d *query_box, int16_t plane_count, real_plane3d *planes);

    /**
     * Collects the surfaces of the listed clusters that pass the query box and clip planes into the output array, marking
     * them in the visited bit set. Returns the number written.
     *
     * @address 0x553c40
     */
    static int32_t collect_surfaces_in_clusters(int32_t *out_surfaces, int16_t max_count, real_rectangle3d *query_box, int16_t plane_count, real_plane3d *planes, uint32_t *visited_bits, int16_t cluster_count, int16_t *cluster_indices);

    /**
     * Collects the surface indices within the query box and sphere, either through the bsp3d tree or through the supplied
     * clusters. Returns the number of indices written.
     *
     * @address 0x553d80
     */
    static int16_t query_surfaces(real_rectangle3d *query_box, real_point3d *query_point, int32_t *out_surfaces, int32_t max_count, float radius, int16_t plane_count, real_plane3d *planes, int16_t cluster_count, int16_t *cluster_indices);

    /**
     * Returns 1 when any of the points is at or in front of the camera plane within the tolerance along the camera forward
     * axis, otherwise 0.
     *
     * @address 0x554a20
     */
    static uint8_t points_within_band(real_point3d *points, int16_t point_count, float tolerance);

    /**
     * Walks one bsp leaf's surface references for a surface on the accepted plane whose triangle contains the point. On a
     * match writes the surface index, its lightmap and material indices and the barycentric coordinates.
     *
     * @address 0x554fa0
     */
    static uint8_t leaf_find_material_surface(real_point3d *point, int32_t accepted_plane, int16_t *out_lightmap_index, int16_t *out_material_index, int32_t *out_surface, void *out_barycentric_u, void *out_barycentric_v, int32_t raw_child);

    /**
     * Steps from the start position along the direction, casting a collision segment each step, until a hit resolves to a
     * bsp surface whose lightmap has a bitmap. The position ends at the contact point; failed steps nudge it 1/4096
     * further.
     *
     * @address 0x555190
     */
    static uint8_t resolve_position_to_surface(real_point3d *start_position, real_point3d *position, int16_t *out_lightmap_index, void *out_barycentric_v, real_vector3d *direction, int16_t *out_material_index, int32_t *out_surface, void *out_barycentric_u);

    /**
     * Reads the sound potentially-audible-set byte for a pair of clusters from the triangular table in the bsp. Returns 0
     * for the same cluster.
     *
     * @address 0x552210
     */
    static uint8_t cluster_sound_distance_lookup(int16_t cluster_a, int16_t cluster_b, ScenarioStructureBSP *structure_bsp);

};

}  // namespace halo::structures
