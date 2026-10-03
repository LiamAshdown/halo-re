/**
 * @file include/halo/structures/structure_bsp_view.hpp
 * Operations on a ScenarioStructureBSP tag: visibility expansion, portal tests, surface lookup.
 */
#pragma once

#include "halo/structures/structures_types.hpp"

namespace halo::structures {

/**
 * Non-owning view of a ScenarioStructureBSP tag.
 */
class structure_bsp_view {
public:
    explicit structure_bsp_view(ScenarioStructureBSP *p) : self(p) {}

    /**
     * Marks the surfaces of every visible cluster as visible unless all three vertices of a surface lie outside one of the
     * cluster's frustum planes. Stops at 0x4000 visible surfaces.
     *
     * @address 0x553a70
     */
    void expand_visible_clusters_by_plane();

    /**
     * Marks the surfaces of every visible cluster's subclusters whose bounding box passes the cluster frustum as visible.
     * Stops at 0x4000 visible surfaces.
     *
     * @address 0x553920
     */
    void expand_visible_clusters_by_subcluster();

    /**
     * Tests whether the point is within the tolerance of a cluster portal: close to its plane, inside its bounding sphere grown by the tolerance, and within its polygon edges.
     *
     * @address 0x554b00
     */
    uint8_t portal_sphere_test(real_point3d *point, int16_t portal_index, float tolerance);

    /**
     * Binary-searches the bsp's lightmaps and then the lightmap's materials for the one whose surface range contains the
     * surface, writing both indices.
     *
     * @address 0x552110
     */
    void surface_material_locate(int32_t surface_index, int16_t *out_material_index, int16_t *out_lightmap_index);

private:
    ScenarioStructureBSP *self;
};

}  // namespace halo::structures
