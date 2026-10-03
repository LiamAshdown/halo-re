/**
 * @file src/structures/structure_visibility.cpp
 * Per-frame visibility of clusters, surfaces and objects from the render camera.
 * The original author notes and decompiles are in docs/original/structures/.
 */

#include "halo/structures/structures.hpp"

extern "C" {
extern int32_t render_cluster_index;
extern uint8_t render_frustum_global[];
extern uint8_t render_camera_global[];
extern uint32_t *flood_recursion_bits;
extern int16_t visible_cluster_count;
extern structure_bsp_visible_cluster visible_clusters[k_maximum_visible_clusters];
extern void render_frustum_compute_screen_clip_bounds(float *out, void *camera);
extern uint32_t render_camera_compute_frustum_bounds(void *camera, float bounds_out[4], float bounds_in[4]);
extern void chimera__render_camera_build_frustum(float *frustum_bounds, void *camera, void *frustum,
    uint8_t build_projection);
extern ScenarioStructureBSP *global_structure_bsp;
extern uint32_t cluster_visible_bits[halo::structures::k_cluster_visible_bit_words];
extern uint32_t surface_visible_bits[k_maximum_visible_surface_bits];
extern int16_t visible_surface_count;
extern uint8_t debug_render_cluster_pvs;
extern int16_t cluster_visible_index[halo::structures::k_maximum_flood_clusters];
extern uint8_t no_subcluster_path_taken;
extern int16_t render_frustum_test_sphere(void *frustum, real_point3d *center, float radius);
extern ModelCollisionGeometryBSP *global_collision_bsp;
extern Scenario *global_scenario;
extern tag_instance *tag_instances;
extern int32_t render_leaf_index;
extern uint8_t render_cluster_has_sky;
extern int16_t render_cluster_sky_index;
extern int32_t bsp3d_node_find_leaf(int32_t node_index, void *bsp, real_point3d *point);
extern int16_t polygon2d_clip_to_planes(int16_t vertex_count, real_point2d *vertices,
                                         int16_t clip_point_count, real_point2d *clip_points,
                                         int16_t maximum_count, real_point2d *out,
                                         real epsilon);
}

namespace halo::structures {

void structure_visibility::camera_visibility_pass(void)
{
    if (render_cluster_index == -1) {
        return;
    }

    float screen_bounds[4];
    render_frustum_compute_screen_clip_bounds(screen_bounds, (void *)render_frustum_global);

    polygon2d clip_polygon;
    clip_polygon.point_count = 4;
    clip_polygon.points[0].x = screen_bounds[0]; clip_polygon.points[0].y = screen_bounds[2];
    clip_polygon.points[1].x = screen_bounds[1]; clip_polygon.points[1].y = screen_bounds[2];
    clip_polygon.points[2].x = screen_bounds[1]; clip_polygon.points[2].y = screen_bounds[3];
    clip_polygon.points[3].x = screen_bounds[0]; clip_polygon.points[3].y = screen_bounds[3];

    uint32_t recursion_bits[k_cluster_visible_bit_words];
    for (int i = 0; i < k_cluster_visible_bit_words; i++) {
        recursion_bits[i] = 0;
    }
    flood_recursion_bits = recursion_bits;

    visible_cluster_count = 0;
    cluster_flood::camera_portal_flood_recursive(render_cluster_index, &clip_polygon);

    for (int16_t i = 0; i < visible_cluster_count; i++) {
        uint8_t *cluster = (uint8_t *)&visible_clusters[i];
        render_camera_compute_frustum_bounds((void *)render_camera_global, screen_bounds, (float *)(cluster + k_visible_cluster_screen_bounds_offset));
        chimera__render_camera_build_frustum(screen_bounds, (void *)render_camera_global, cluster + k_visible_cluster_frustum_offset, 0);
    }
}

void structure_visibility::cluster_visibility_update(void)
{
    ScenarioStructureBSP *tag = global_structure_bsp;

    uint32_t fill = (render_cluster_index != -1) ? 0 : k_dword_none;
    int32_t cluster_dwords = bit_array_word_count(tag->clusters.count);
    for (int32_t i = 0; i < cluster_dwords; i++) {
        cluster_visible_bits[i] = fill;
    }

    visible_surface_count = 0;
    int32_t surface_dwords = bit_array_word_count(tag->surfaces.count);
    for (int32_t i = 0; i < surface_dwords; i++) {
        surface_visible_bits[i] = 0;
    }

    visible_cluster_count = 0;
    structure_visibility::camera_visibility_pass();

    if (debug_render_cluster_pvs != 0) {
        visible_cluster_count = 0;
        int32_t row_dwords = bit_array_word_count(tag->clusters.count);
        uint32_t *pvs_row = (uint32_t *)((uint8_t *)tag->cluster_data.pointer +
                                          render_cluster_index * row_dwords * 4);
        for (int32_t i = 0; i < row_dwords; i++) {
            cluster_visible_bits[i] = pvs_row[i];
        }
        if (tag->clusters.count > 0) {
            for (int16_t cluster_index = 0; cluster_index < tag->clusters.count; cluster_index++) {
                if ((cluster_visible_bits[bit_array_word(cluster_index)] & bit_array_mask(cluster_index)) == 0) {
                    continue;
                }
                int16_t visible_index = visible_cluster_count++;
                cluster_visible_index[cluster_index] = visible_index;
                visible_clusters[visible_index].cluster_index = cluster_index;
                render_frustum_compute_screen_clip_bounds((float *)&visible_clusters[visible_index].screen_bounds_x,
                             (void *)render_frustum_global);
            }
        }
    }

    ScenarioStructureBSPCluster *clusters = (ScenarioStructureBSPCluster *)tag->clusters.pointer;
    if (clusters[0].subclusters.count == 0) {
        if (no_subcluster_path_taken == 0) {
            no_subcluster_path_taken = 1;
        }
        structure_bsp_view(tag).expand_visible_clusters_by_plane();
        return;
    }
    structure_bsp_view(tag).expand_visible_clusters_by_subcluster();
}

int16_t structure_visibility::collect_visible_objects(int32_t *out_handles, int16_t max_count, structure_bsp_object_iterate_begin_fn iterate_begin, structure_bsp_object_iterate_next_fn iterate_next, structure_bsp_object_get_bounds_fn get_bounds, structure_bsp_object_predicate_fn predicate, structure_bsp_object_accept_fn accept)
{
    int16_t written = 0;

    for (int16_t i = 0; i < visible_cluster_count; i++) {
        uint32_t cursor;
        uint32_t handle = iterate_begin(&cursor, visible_clusters[i].cluster_index);
        while (handle != k_dword_none) {
            if (predicate(handle)) {
                float radius;
                real_point3d center;
                get_bounds(handle, &center, &radius);
                if (written < max_count &&
                    (render_cluster_index == -1 ||
                     render_frustum_test_sphere(&visible_clusters[i].frustum, &center,
                                                 radius) != 0)) {
                    out_handles[written] = (int32_t)handle;
                    written++;
                    accept(handle);
                }
            }
            handle = iterate_next(&cursor);
        }
    }
    return written;
}

void structure_visibility::render_camera_update_leaf_and_cluster(real_point3d *camera_position)
{
    int32_t leaf = bsp3d_node_find_leaf(0, (void *)(uintptr_t)global_structure_bsp->collision_bsp.pointer, camera_position);

    if (leaf == -1 && render_leaf_index < global_structure_bsp->leaves.count) {
        leaf = render_leaf_index;
    }
    render_leaf_index = leaf;
    render_cluster_index = -1;
    render_cluster_sky_index = -1;
    render_cluster_has_sky = 0;

    if (render_leaf_index != -1) {
        ScenarioStructureBSPLeaf *leaves = (ScenarioStructureBSPLeaf *)global_structure_bsp->leaves.pointer;
        ScenarioStructureBSPCluster *clusters = (ScenarioStructureBSPCluster *)global_structure_bsp->clusters.pointer;
        TagID sky_tag_id;
        int have_sky_tag_id = 0;

        render_cluster_index = leaves[render_leaf_index & k_leaf_index_mask].cluster;
        render_cluster_sky_index = clusters[render_cluster_index].sky;

        if (render_cluster_sky_index > -1 && render_cluster_sky_index < global_scenario->skies.count) {
            ScenarioSky *skies = (ScenarioSky *)global_scenario->skies.pointer;
            if (skies[render_cluster_sky_index].sky.tag_id.index != k_word_none) {
                sky_tag_id = skies[render_cluster_sky_index].sky.tag_id;
                have_sky_tag_id = 1;
            }
        }

        if (have_sky_tag_id) {
            Sky *sky = (Sky *)tag_instances[sky_tag_id.index].data;
            if (sky != 0 && sky->model.tag_id.index != k_word_none) {
                render_cluster_has_sky = 1;
            }
        }
    }
}

static ShaderEnvironment *mirror_shader_environment(ScenarioStructureBSPMirror *mirror)
{
    tag_instance *instance = &tag_instances[mirror->shader.tag_id.index];
    return (ShaderEnvironment *)instance->data;
}

uint8_t structure_visibility::mirror_query(void *camera_ref, void *camera, structure_bsp_mirror_result *out)
{
    uint8_t found = 0;
    float screen_bounds[4];

    real_point2d clip_points[k_maximum_clip_polygon_points];
    polygon2d clip_polygon;
    polygon2d project_out;

    if (render_cluster_index == -1) {
        return 0;
    }

    render_frustum_compute_screen_clip_bounds(screen_bounds, camera);
    clip_points[0].x = screen_bounds[0]; clip_points[0].y = screen_bounds[2];
    clip_points[1].x = screen_bounds[1]; clip_points[1].y = screen_bounds[2];
    clip_points[2].x = screen_bounds[1]; clip_points[2].y = screen_bounds[3];
    clip_points[3].x = screen_bounds[0]; clip_points[3].y = screen_bounds[3];

    int32_t cluster_count = global_structure_bsp->clusters.count;
    if (cluster_count <= 0) {
        return found;
    }

    int32_t row_dwords = bit_array_word_count(cluster_count);
    uint32_t *pvs_row = (uint32_t *)((uint8_t *)global_structure_bsp->cluster_data.pointer +
                                      row_dwords * render_cluster_index * 4);

    for (int16_t cluster_index = 0; cluster_index < cluster_count; pvs_row++) {
        if (*pvs_row == 0) {
            cluster_index = (int16_t)(cluster_index + k_bit_array_word_bits);
            continue;
        }
        for (int bit = 0; bit < (int)k_bit_array_word_bits && cluster_index < cluster_count; bit++, cluster_index++) {
            if ((*pvs_row & (1u << bit)) == 0) {
                continue;
            }
            ScenarioStructureBSPCluster *cluster =
                &((ScenarioStructureBSPCluster *)global_structure_bsp->clusters.pointer)[cluster_index];
            for (int32_t m = 0; m < (int32_t)cluster->mirrors.count; m++) {
                ScenarioStructureBSPMirror *mirror =
                    &((ScenarioStructureBSPMirror *)cluster->mirrors.pointer)[m];
                int16_t project_result = cluster_flood::portal_project((real_plane3d *)&mirror->plane, camera_ref, (real_point3d *)mirror->vertices.pointer, camera, mirror->vertices.count, 1, &project_out);
                int16_t clip_result = 0;
                if (project_result == 0) {
                    clip_result = polygon2d_clip_to_planes(project_out.point_count,
                                        &project_out.points[0], 4, clip_points, k_maximum_clip_polygon_points,
                                        &clip_polygon.points[0], 9.99999975e-05f);
                    clip_polygon.point_count = clip_result;
                } else if (project_result == 2) {
                    clip_result = 1;
                } else {
                    continue;
                }
                if (clip_result == 0) {
                    continue;
                }
                ShaderEnvironment *shader_env = mirror_shader_environment(mirror);
                if (shader_env->base.shader_type == shadertype_environment) {
                    out->shader_mirror_value_0 = shader_env->runtime_mirror_value_0;
                    out->shader_mirror_value_1 = shader_env->runtime_mirror_value_1;
                } else {
                    out->shader_mirror_value_0 = 0.0f;
                    out->shader_mirror_value_1 = 0.0f;
                }
                out->plane.normal.i = mirror->plane.vector.i;
                out->plane.normal.j = mirror->plane.vector.j;
                out->plane.normal.k = mirror->plane.vector.k;
                out->plane.d = mirror->plane.w;
                out->cluster_index = cluster_index;
                found = 1;
            }
        }
    }
    return found;
}

}  // namespace halo::structures
