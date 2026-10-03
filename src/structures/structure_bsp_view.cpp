/**
 * @file src/structures/structure_bsp_view.cpp
 * Operations on a ScenarioStructureBSP tag: visibility expansion, portal tests, surface lookup.
 * The original author notes and decompiles are in docs/original/structures/.
 */

#include "halo/structures/structures.hpp"

extern "C" {
extern int32_t render_cluster_index;
extern uint8_t debug_render_cluster_pvs;
extern structure_bsp_visible_cluster visible_clusters[k_maximum_visible_clusters];
extern int16_t visible_cluster_count;
extern uint32_t surface_visible_bits[k_maximum_visible_surface_bits];
extern int16_t visible_surface_count;
extern uint16_t render_frustum_classify_point_side_planes(void *frustum_or_camera, void *vertex);
extern int16_t render_frustum_test_bounding_box(void *frustum_or_camera, void *box, int32_t flags);
extern ModelCollisionGeometryBSP *global_collision_bsp;
extern int16_t vector3d_major_axis_index(real_vector3d *v);
extern const projection_axis_pair k_projection_axes[6];
extern uint8_t polygon2d_point_inside_tolerance(real_point2d *vertices, int16_t count,
                                                 real_point2d *point, real tolerance);
extern double sqrt(double x);
}

namespace halo::structures {

void structure_bsp_view::expand_visible_clusters_by_plane()
{
    ScenarioStructureBSPCluster *clusters = (ScenarioStructureBSPCluster *)self->clusters.pointer;
    ScenarioStructureBSPLightmap *lightmaps = (ScenarioStructureBSPLightmap *)self->lightmaps.pointer;
    ScenarioStructureBSPSurface *surfaces = (ScenarioStructureBSPSurface *)self->surfaces.pointer;

    for (int16_t i = 0; i < visible_cluster_count; i++) {
        ScenarioStructureBSPCluster *cluster = &clusters[visible_clusters[i].cluster_index];
        void *frustum_or_camera;
        if (debug_render_cluster_pvs != 0 || render_cluster_index == -1) {
            frustum_or_camera = (void *)0x7c3168;
        } else {
            frustum_or_camera = &visible_clusters[i].frustum;
        }

        int32_t *cursor = (int32_t *)cluster->surface_indices.pointer;
        int32_t consumed = 0;
        int32_t run_dwords = cluster->surface_indices.count;
        while (consumed < run_dwords) {
            int32_t lightmap_index = cursor[0];
            int32_t material_index = cursor[1];
            int32_t surface_count = cursor[2];
            cursor += 3;
            consumed += 3;
            ScenarioStructureBSPMaterial *material =
                &((ScenarioStructureBSPMaterial *)lightmaps[lightmap_index].materials.pointer)
                    [material_index];
            ScenarioStructureBSPMaterialCompressedRenderedVertex *vertices =
                (ScenarioStructureBSPMaterialCompressedRenderedVertex *)
                    material->compressed_vertices.pointer;

            int32_t run_end = consumed + surface_count;
            while (consumed < run_end && visible_surface_count < 0x4000) {
                int32_t surface_index = *cursor;
                int32_t word = surface_index >> 5;
                uint32_t mask = 1u << (surface_index & 0x1f);
                cursor++;
                if ((surface_visible_bits[word] & mask) == 0) {
                    ScenarioStructureBSPSurface *surface = &surfaces[surface_index];
                    uint16_t outside0 = render_frustum_classify_point_side_planes(
                        frustum_or_camera, &vertices[surface->vertex0_index]);
                    int rejected = 0;
                    if (outside0 != 0) {
                        uint16_t outside1 = render_frustum_classify_point_side_planes(
                            frustum_or_camera, &vertices[surface->vertex1_index]);
                        if (outside1 != 0) {
                            uint16_t outside2 = render_frustum_classify_point_side_planes(
                                frustum_or_camera, &vertices[surface->vertex2_index]);
                            if (outside2 != 0 && (outside2 & outside0 & 0x3f & outside1) != 0) {
                                rejected = 1;
                            }
                        }
                    }
                    if (!rejected) {
                        surface_visible_bits[word] |= mask;
                        visible_surface_count++;
                    }
                }
                consumed++;
            }
        }
    }
}

void structure_bsp_view::expand_visible_clusters_by_subcluster()
{
    ScenarioStructureBSPCluster *clusters = (ScenarioStructureBSPCluster *)self->clusters.pointer;

    for (int16_t i = 0; i < visible_cluster_count; i++) {
        if (visible_surface_count > 0x3fff) {
            return;
        }
        ScenarioStructureBSPCluster *cluster = &clusters[visible_clusters[i].cluster_index];
        void *frustum_or_camera;
        if (debug_render_cluster_pvs != 0 || render_cluster_index == -1) {
            frustum_or_camera = (void *)0x7c3168;
        } else {
            frustum_or_camera = &visible_clusters[i].frustum;
        }

        for (int32_t j = 0; j < (int32_t)cluster->subclusters.count; j++) {
            if (visible_surface_count > 0x3fff) {
                break;
            }
            ScenarioStructureBSPSubcluster *subcluster =
                &((ScenarioStructureBSPSubcluster *)cluster->subclusters.pointer)[j];
            if (render_frustum_test_bounding_box(frustum_or_camera, subcluster, 0) == 0) {
                continue;
            }
            int32_t *indices = (int32_t *)subcluster->surface_indices.pointer;
            for (int32_t k = 0; k < (int32_t)subcluster->surface_indices.count; k++) {
                int32_t surface = indices[k];
                int32_t word = surface >> 5;
                uint32_t mask = 1u << (surface & 0x1f);
                if ((surface_visible_bits[word] & mask) != 0) {
                    continue;
                }
                if (visible_surface_count > 0x3fff) {
                    break;
                }
                surface_visible_bits[word] |= mask;
                visible_surface_count++;
            }
        }
    }
}

uint8_t structure_bsp_view::portal_sphere_test(real_point3d *point, int16_t portal_index, float tolerance)
{
    ScenarioStructureBSPClusterPortal *portal =
        &((ScenarioStructureBSPClusterPortal *)self->cluster_portals.pointer)[portal_index];
    ModelCollisionGeometryBSP *collision_bsp =
        (ModelCollisionGeometryBSP *)self->collision_bsp.pointer;
    ModelCollisionGeometryBSPPlane *plane =
        &((ModelCollisionGeometryBSPPlane *)collision_bsp->planes.pointer)[portal->plane_index];

    float distance = point->x * plane->plane.vector.i + point->y * plane->plane.vector.j +
                      point->z * plane->plane.vector.k - plane->plane.w;
    if ((distance < 0.0f ? -distance : distance) >= tolerance) {
        return 0;
    }

    float dx = portal->centroid.x - point->x;
    float dy = portal->centroid.y - point->y;
    float dz = portal->centroid.z - point->z;
    float expanded_radius = tolerance + portal->bounding_radius;
    if (dy * dy + dx * dx + dz * dz >= expanded_radius * expanded_radius) {
        return 0;
    }

    Vector3D *normal_raw = &((ModelCollisionGeometryBSPPlane *)global_collision_bsp->planes
                                  .pointer)[portal->plane_index]
                                 .plane.vector;
    real_vector3d *normal = (real_vector3d *)normal_raw;
    int16_t axis = vector3d_major_axis_index(normal);
    int32_t table_index = ((0.0f < ((float *)normal)[axis]) ? 1 : 0) + axis * 2;
    int16_t axis_i = k_projection_axes[table_index].i;
    int16_t axis_j = k_projection_axes[table_index].j;

    real_point3d projected;
    float neg_distance = -distance;
    projected.x = neg_distance * plane->plane.vector.i + point->x;
    projected.y = neg_distance * plane->plane.vector.j + point->y;
    projected.z = neg_distance * plane->plane.vector.k + point->z;
    real_point2d point_2d;
    point_2d.x = ((float *)&projected)[axis_i];
    point_2d.y = ((float *)&projected)[axis_j];

    real_point2d polygon_2d[0x100];
    ScenarioStructureBSPClusterPortalVertex *vertices =
        (ScenarioStructureBSPClusterPortalVertex *)portal->vertices.pointer;
    for (int32_t i = 0; i < (int32_t)portal->vertices.count; i++) {
        polygon_2d[i].x = ((float *)&vertices[i])[axis_i];
        polygon_2d[i].y = ((float *)&vertices[i])[axis_j];
    }

    float remaining = tolerance * tolerance - distance * projected.x;
    float radius_2d = (real)sqrt((double)remaining);
    return polygon2d_point_inside_tolerance(polygon_2d, (int16_t)portal->vertices.count, &point_2d,
                                             radius_2d);
}

void structure_bsp_view::surface_material_locate(int32_t surface_index, int16_t *out_material_index, int16_t *out_lightmap_index)
{
    int16_t low = 0;
    int16_t high = (int16_t)(self->lightmaps.count - 1);

    *out_lightmap_index = 0;
    if (high > 0) {
        do {
            int16_t mid = (int16_t)(((int32_t)high - (int32_t)low) / 2 + low);
            ScenarioStructureBSPLightmap *lightmap =
                (ScenarioStructureBSPLightmap *)self->lightmaps.pointer + mid;
            ScenarioStructureBSPMaterial *materials =
                (ScenarioStructureBSPMaterial *)lightmap->materials.pointer;

            *out_lightmap_index = mid;
            if (surface_index < materials[0].surfaces) {
                high = mid - 1;
                *out_lightmap_index = high;
            } else {
                int32_t last = lightmap->materials.count;
                if (surface_index < materials[last - 1].surfaces + materials[last - 1].surface_count) {
                    break;
                }
                low = mid + 1;
                *out_lightmap_index = low;
            }
        } while (low < high);
    }

    {
        ScenarioStructureBSPLightmap *lightmap =
            (ScenarioStructureBSPLightmap *)self->lightmaps.pointer + *out_lightmap_index;
        ScenarioStructureBSPMaterial *materials =
            (ScenarioStructureBSPMaterial *)lightmap->materials.pointer;
        int16_t low2 = 0;
        int16_t high2 = (int16_t)lightmap->materials.count;

        *out_material_index = 0;
        if (high2 > 0) {
            do {
                int16_t mid2 = (int16_t)(((int32_t)high2 - (int32_t)low2) / 2 + low2);
                ScenarioStructureBSPMaterial *material = &materials[mid2];

                *out_material_index = mid2;
                if (surface_index < material->surfaces) {
                    high2 = mid2 - 1;
                    *out_material_index = high2;
                } else {
                    if (surface_index < material->surfaces + material->surface_count) {
                        return;
                    }
                    low2 = mid2 + 1;
                    *out_material_index = low2;
                }
            } while (low2 < high2);
        }
    }
}

}  // namespace halo::structures
