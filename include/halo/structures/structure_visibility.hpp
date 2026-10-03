/**
 * @file include/halo/structures/structure_visibility.hpp
 * Per-frame visibility of clusters, surfaces and objects from the render camera.
 */
#pragma once

#include "halo/structures/structures_types.hpp"

typedef uint32_t (*structure_bsp_object_iterate_begin_fn)(uint32_t *cursor, int16_t cluster_index);

typedef uint32_t (*structure_bsp_object_iterate_next_fn)(uint32_t *cursor);

typedef uint8_t (*structure_bsp_object_predicate_fn)(uint32_t handle);

typedef void (*structure_bsp_object_get_bounds_fn)(uint32_t handle, real_point3d *center_out, float *radius_out);

typedef void (*structure_bsp_object_accept_fn)(uint32_t handle);

namespace halo::structures {

/**
 * Computes which clusters, surfaces and objects of the resident structure bsp are visible from the render camera.
 */
struct structure_visibility {
    /**
     * Floods the cluster portals from the camera's cluster with the screen clip polygon, then builds a frustum for every
     * visible cluster.
     *
     * @address 0x5544f0
     */
    static void camera_visibility_pass(void);

    /**
     * Resets the visible cluster and surface bit sets, runs the camera visibility pass (or loads the cluster's potentially
     * visible set when debugging) and expands the visible clusters to visible surfaces.
     *
     * @address 0x5537c0
     */
    static void cluster_visibility_update(void);

    /**
     * Collects the handles of the objects in the visible clusters that pass the predicate and the cluster frustum, calling
     * accept for each, up to max_count. Returns the number collected.
     *
     * @address 0x554420
     */
    static int16_t collect_visible_objects(int32_t *out_handles, int16_t max_count, structure_bsp_object_iterate_begin_fn iterate_begin, structure_bsp_object_iterate_next_fn iterate_next, structure_bsp_object_get_bounds_fn get_bounds, structure_bsp_object_predicate_fn predicate, structure_bsp_object_accept_fn accept);

    /**
     * Resolves the render camera's bsp leaf and cluster, falling back to the last known leaf when the probe fails but that
     * leaf is still valid, and caches whether the cluster has a sky with a model.
     *
     * @address 0x553490
     */
    static void render_camera_update_leaf_and_cluster(real_point3d *camera_position);

    /**
     * Finds a mirror surface visible from the camera and fills the result with its plane and shader environment. Returns 1
     * when a mirror was found.
     *
     * @address 0x553560
     */
    static uint8_t mirror_query(void *camera_ref, void *camera, structure_bsp_mirror_result *out);

};

}  // namespace halo::structures
