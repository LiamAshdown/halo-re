/**
 * @file include/halo/structures/structure_fog.hpp
 * Fog of a structure bsp cluster.
 */
#pragma once

#include "halo/structures/structures_types.hpp"

namespace halo::structures {

/**
 * Resolves the fog tag and builds the fog environment for a cluster of the resident structure bsp.
 */
struct structure_fog {
    /**
     * Resolves the fog tag id of a cluster through its fog region and fog palette entry, or from the first sky's indoor
     * fog screen when use_sky is set. Returns -1 when there is none.
     *
     * @address 0x555270
     */
    static uint32_t resolve_fog_tag(int16_t cluster_index, ScenarioStructureBSP *structure_bsp, uint8_t use_sky);

    /**
     * Fills the fog environment for a cluster: plane mode and plane, colour, density, opaque distance and depth, falling
     * back to the sky fog when the cluster has none.
     *
     * @address 0x555330
     */
    static void build_fog_environment(int16_t cluster_index, structure_fog_environment *out);

};

}  // namespace halo::structures
