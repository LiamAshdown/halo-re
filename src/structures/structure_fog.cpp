/**
 * @file src/structures/structure_fog.cpp
 * Fog of a structure bsp cluster.
 * The original author notes and decompiles are in docs/original/structures/.
 */

#include "halo/structures/structures.hpp"
#include "halo/cache/api.hpp"

extern "C" {
extern Scenario *global_scenario;
extern ScenarioStructureBSP *global_structure_bsp;
}

namespace halo::structures {

uint32_t structure_fog::resolve_fog_tag(int16_t cluster_index, ScenarioStructureBSP *structure_bsp, uint8_t use_sky)
{
    if (cluster_index == -1) {
        return k_dword_none;
    }

    if (use_sky) {
        Scenario *scenario = global_scenario;
        uint32_t sky_tag_id = k_dword_none;
        if (scenario->skies.count > 0) {
            sky_tag_id = ((ScenarioSky *)scenario->skies.pointer)[0].sky.tag_id.index |
                         ((uint32_t)((ScenarioSky *)scenario->skies.pointer)[0].sky.tag_id.id
                          << 16);
        }
        if (sky_tag_id != k_dword_none) {
            Sky *sky = (Sky *)halo::cache::globals().tag_instances[datum_slot(sky_tag_id)].data;
            if (sky != 0) {
                return *(uint32_t *)&sky->indoor_fog_screen.tag_id;
            }
        }
        return k_dword_none;
    }

    ScenarioStructureBSPCluster *cluster =
        &((ScenarioStructureBSPCluster *)structure_bsp->clusters.pointer)[cluster_index];
    uint16_t fog = cluster->fog;
    if (fog == k_word_none) {
        return k_dword_none;
    }

    uint16_t region_index;
    if ((int16_t)fog < 0) {
        ScenarioStructureBSPFogPlane *plane =
            &((ScenarioStructureBSPFogPlane *)structure_bsp->fog_planes.pointer)[fog & k_cluster_fog_index_mask];
        region_index = plane->front_region;
    } else {
        region_index = fog & k_cluster_fog_index_mask;
    }
    if (region_index == k_word_none) {
        return k_dword_none;
    }

    int16_t palette_index =
        ((ScenarioStructureBSPFogRegion *)structure_bsp->fog_regions.pointer)[region_index].fog;
    if (palette_index == -1) {
        return k_dword_none;
    }
    ScenarioStructureBSPFogPalette *palette =
        &((ScenarioStructureBSPFogPalette *)structure_bsp->fog_palette.pointer)[palette_index];
    return *(uint32_t *)&palette->fog.tag_id;
}

void structure_fog::build_fog_environment(int16_t cluster_index, structure_fog_environment *out)
{
    out->plane_mode = _structure_fog_plane_none;
    out->fog_flags = 0;
    out->screen_parameters = 0;

    uint32_t fog_tag_id = structure_fog::resolve_fog_tag(cluster_index, global_structure_bsp, 0);
    uint8_t from_sky;
    if (fog_tag_id == k_dword_none) {
        fog_tag_id = structure_fog::resolve_fog_tag(cluster_index, global_structure_bsp, 1);
        from_sky = 1;
        if (fog_tag_id == k_dword_none) {
            return;
        }
    } else {
        from_sky = 0;
    }

    Fog *fog = (Fog *)halo::cache::globals().tag_instances[datum_slot(fog_tag_id)].data;
    ScenarioStructureBSPCluster *cluster =
        &((ScenarioStructureBSPCluster *)global_structure_bsp->clusters.pointer)[cluster_index];

    if (from_sky) {
        out->flags |= 1;
    } else {
        if ((cluster->fog & k_cluster_fog_plane_flag) == 0) {
            out->plane_mode = _structure_fog_plane_unbounded;
        } else {
            out->plane_mode = _structure_fog_plane_bounded;
            ScenarioStructureBSPFogPlane *fog_plane =
                &((ScenarioStructureBSPFogPlane *)global_structure_bsp->fog_planes.pointer)
                    [cluster->fog & k_cluster_fog_index_mask];
            out->plane.normal.i = fog_plane->plane.vector.i;
            out->plane.normal.j = fog_plane->plane.vector.j;
            out->plane.normal.k = fog_plane->plane.vector.k;
            out->plane.d = fog_plane->plane.w;
        }
        out->color_red = fog->color.red;
        out->color_green = fog->color.green;
        out->color_blue = fog->color.blue;
        out->maximum_density = fog->maximum_density;
        out->opaque_distance = fog->opaque_distance;
        out->opaque_depth = fog->opaque_depth;

        if ((cluster->fog & k_cluster_fog_plane_flag) != 0) {
            float seed = *(float *)((uint8_t *)fog + k_fog_plane_seed_offset) * 0.0f;
            out->plane.d = seed + out->plane.d;
            globals().fog_plane_vector.i = seed * out->plane.normal.i;
            globals().fog_plane_vector.j = seed * out->plane.normal.j;
            globals().fog_plane_vector.k = seed * out->plane.normal.k;
            globals().fog_plane_vector_valid = 1;
        }
    }

    out->fog_flags = fog->flags;
    out->screen_parameters = &fog->flags_1;
}

}  // namespace halo::structures
