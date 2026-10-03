/**
 * @file include/halo/structures/structure_clusters.hpp
 * Cluster portal flooding and the per-cluster object reference chains.
 */
#pragma once

#include "halo/structures/structures_types.hpp"

namespace halo::structures {

/**
 * Flood fills and portal tests over the cluster graph of the resident structure bsp.
 */
struct cluster_flood {
    /**
     * Floods from a cluster through its portals, clipping the view polygon against each portal and recording every cluster
     * reached in the visible cluster list.
     *
     * @address 0x5545d0
     */
    static void camera_portal_flood_recursive(int16_t cluster_index, polygon2d *view_polygon);

    /**
     * Flood fills clusters from the start cluster through portals that face the given direction within the sine and cosine
     * limits and the maximum distance, writing up to max_count cluster indices.
     *
     * @address 0x554e30
     */
    static int16_t fill_with_predicate(real_point3d *position, real_vector3d *facing, real max_distance, real sin_angle, real cos_angle, int16_t max_count, int16_t *output, int16_t start_cluster);

    /**
     * Recursive flood fill through portals that come within the tolerance of the point, consuming the remaining budget and
     * writing cluster indices to the output.
     *
     * @address 0x554d10
     */
    static int32_t fill_within_radius(int16_t cluster_index, real_point3d *point, float tolerance, int32_t remaining_budget, int16_t *output);

    /**
     * Starts a radius flood fill from a cluster and returns the number of clusters written.
     *
     * @address 0x554cb0
     */
    static int32_t seed(real_point3d *point, float radius, int16_t start_cluster, int16_t *output, int16_t max_count);

    /**
     * Transforms a portal's vertices, clipping them against the camera plane, and projects them to a 2D polygon in the
     * given winding order.
     *
     * @address 0x554850
     */
    static uint8_t portal_project(real_plane3d *plane, void *camera_ref, real_point3d *vertices, void *camera, uint32_t vertex_count, int16_t winding, polygon2d *out);

    /**
     * Tests the camera against a portal's plane and, when it passes, projects the portal polygon.
     *
     * @address 0x5549c0
     */
    static uint8_t portal_test_and_project(char same_side, int16_t portal_index, polygon2d *out);

    /**
     * Collects up to eight indices of weather polyhedra whose bounding spheres come within the radius of the camera and
     * returns how many were stored.
     *
     * @address 0x458b50
     */
    static int16_t weather_polyhedra_find_within_radius(int16_t *out, float radius);

};

/**
 * The pooled per-cluster object reference chains: creating the partition, linking an object into the clusters it
 * overlaps and unlinking it.
 */
struct cluster_references {
    /**
     * Reserves the game state memory for a cluster reference group: the head table, the reference pool and their
     * checksums.
     *
     * @address 0x551e30
     */
    static void partition_new(cluster_reference_group *out, char *name);

    /**
     * Finds every cluster within the radius of the position (only the location's own cluster for a non-positive radius)
     * and links a new reference into both the object's chain and each cluster's chain.
     *
     * @address 0x551f00
     */
    static void add_within_radius(uint32_t light_or_object_handle, datum_index *placement_slot, real_point3d *position, float radius, bsp_leaf_reference *leaf_and_cluster, cluster_reference_group *cluster_list);

    /**
     * Walks the object's chain, unlinks the matching entry from each cluster's chain, deletes every datum visited and
     * leaves the link at none.
     *
     * @address 0x552020
     */
    static void remove_all(uint32_t handle, datum_index *link, cluster_reference_group *cluster_list);

};

}  // namespace halo::structures
