/**
 * @file include/halo/structures/structure_decals.hpp
 * Runtime decals attached to structure bsp clusters.
 */
#pragma once

#include "halo/structures/structures_types.hpp"

namespace halo::structures {

/**
 * Runtime decals of the resident structure bsp: switch group transitions, eviction and suppression.
 */
struct structure_decals {
    /**
     * Detects per-cluster bit transitions between two switch group arrays. Entering a group notifies the object decals;
     * leaving recomputes each runtime decal's orientation and spawns it through collision unless decals are suppressed.
     *
     * @address 0x5530d0
     */
    static void update_switch_transitions(uint32_t *switch_group_a, uint32_t *switch_group_b, int16_t cluster_count);

    /**
     * Evicts the object decals of every cluster that has runtime decals. Does nothing when the bsp has none.
     *
     * @address 0x553070
     */
    static void runtime_decals_evict(void);

    /**
     * Sets the runtime decals suppressed flag.
     *
     * @address 0x553060
     */
    static void runtime_decals_mark_dirty(void);

};

}  // namespace halo::structures
