/**
 * @file include/halo/structures/detail_object_system.hpp
 * Detail object (grass, debris sprites) game state and render list.
 */
#pragma once

#include "halo/structures/structures_types.hpp"

namespace halo::structures {

/**
 * The detail object game state block and its per-frame render list.
 */
struct detail_object_system {
    /**
     * Reserves the detail object game state block and seeds its default z reference vector (0, 0, 1, 0).
     *
     * @address 0x552260
     */
    static void globals_allocate(void);

    /**
     * Marks the cached detail object cell as invalid so the next update rebuilds the render list.
     *
     * @address 0x5522c0
     */
    static void invalidate(void);

    /**
     * Rebuilds the per-layer batch lists from the detail object cells within a 3x3 neighbourhood of the camera's
     * 8-world-unit cell when the camera moved cell or a rebuild is forced, then submits and draws the frame's render list.
     *
     * @address 0x5522d0
     */
    static void update_render_list(void);

    /**
     * Binary search over a sorted cell range returning the first cell not ordered before the key.
     *
     * @address 0x552710
     */
    static ScenarioStructureBSPGlobalDetailObjectCell * cell_lower_bound(ScenarioStructureBSPGlobalDetailObjectCell *begin, ScenarioStructureBSPGlobalDetailObjectCell *end, detail_object_cell_key *key);

    /**
     * Binary search over a sorted cell range returning the first cell ordered after the key.
     *
     * @address 0x552780
     */
    static ScenarioStructureBSPGlobalDetailObjectCell * cell_upper_bound(ScenarioStructureBSPGlobalDetailObjectCell *begin, ScenarioStructureBSPGlobalDetailObjectCell *end, detail_object_cell_key *key);

};

}  // namespace halo::structures
