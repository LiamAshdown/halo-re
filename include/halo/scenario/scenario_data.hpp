/**
 * @file include/halo/scenario/scenario_data.hpp
 * Scenario loading, object name lookup and the globals material table.
 */
#pragma once

#include "halo/scenario/scenario_types.hpp"

namespace halo::scenario {

/**
 * Non-owning view of the scenario tag.
 */
class scenario_view {
public:
    explicit scenario_view(Scenario *p) : self(p) {}

    /**
     * Returns the index of the scenario object name equal to the given string, or -1 when none matches.
     *
     * @address 0x53ebb0
     */
    int16_t object_name_find_index(char *name);

private:
    Scenario *self;
};

/**
 * Loads a map cache file and publishes the scenario and globals tags.
 */
struct scenario_loader {
    /**
     * Loads the map cache file at the path, resolves the scenario and globals tags, switches in structure bsp 0 and on
     * success resets the runtime item handle of every netgame equipment entry to -1. Returns nonzero on success. The
     * failure path walks an always-empty buffer and returns 0, exactly as the original.
     *
     * @address 0x53e6a0
     */
    static uint8_t load(char *path);

    /**
     * Returns the globals material for the index, or the static fallback material when the index is out of range (clearing
     * its melee hit sound reference the first time that happens).
     *
     * @address 0x53e7c0
     */
    static GlobalsMaterial * globals_material_get(int16_t material_index);

};

}  // namespace halo::scenario
