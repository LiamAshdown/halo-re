/**
 * @file include/halo/scenario/structure_bsp_switch.hpp
 * Switching the resident structure bsp and the activate/deactivate callback tables.
 */
#pragma once

#include "halo/scenario/scenario_types.hpp"

namespace halo::scenario {

/**
 * Owns the sequence that replaces the resident structure bsp: deactivate callbacks, dispose, load, republish the data
 * pointers, activate callbacks. The callback tables themselves are the engine's fixed global tables.
 */
struct structure_bsp_switcher {
    /**
     * Runs every registered structure-bsp activate procedure in table order, ignoring their results. Called after the new
     * structure bsp is current.
     *
     * @address 0x53e680
     */
    static void activate_callbacks(void);

    /**
     * Runs every registered structure-bsp deactivate procedure in table order, ignoring their results. Called before the
     * current structure bsp is unloaded.
     *
     * @address 0x53e660
     */
    static void deactivate_callbacks(void);

    /**
     * Switches the resident structure bsp to the given index: runs the deactivate table and disposes the old bsp, loads
     * the new one, republishes its tag data and collision bsp pointers, then runs the activate table. A no-op when the
     * index is already resident, negative or out of range. Returns whether the switch happened.
     *
     * @address 0x53eeb0
     */
    static uint8_t switch_to(int16_t structure_bsp_index);

    /**
     * Resyncs the resident structure bsp to the loaded game state after a saved-game load. When the saved bsp index no
     * longer matches the resident one it tears down the old bsp's material vertex buffers and tag data pointer directly,
     * without the callback tables, and then switches.
     *
     * @address 0x53efc0
     */
    static void switch_after_load(void);

    /**
     * Probes the point against the resident collision bsp. While the probe misses it nudges the point's z upward by 0.05
     * and retries, up to 150 times, mutating the point in place. Returns true only when the very first probe already found
     * a leaf.
     *
     * @address 0x53e870
     */
    static uint8_t locate_point_nudge_up(real_point3d *point);

};

}  // namespace halo::scenario
