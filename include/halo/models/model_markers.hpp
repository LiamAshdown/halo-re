/**
 * @file include/halo/models/model_markers.hpp
 * Marker group lookup of model tags.
 */
#pragma once

#include "halo/models/models_types.hpp"

namespace halo::models {

/**
 * Stateless marker lookups on model tags addressed by tag index.
 */
struct model_markers {
    /**
     * Binary-searches the model's sorted marker group table for a case-insensitive name. Returns the index, or -1 for an
     * invalid tag, a NULL or empty name or no match.
     *
     * @address 0x4d77c0
     */
    static int16_t group_index_from_name(datum_index model_tag_id, const char *name);

    /**
     * Resolves a marker group by name and fills object_marker records for every instance in it, optionally filtered by
     * region permutations and remapped through a node remap table, negating the left axis when mirrored. Returns the
     * number written.
     *
     * @address 0x4d7850
     */
    static int16_t get_by_name(datum_index model_tag_id, const char *name, uint8_t *region_permutations, int16_t *node_remap, real_matrix4x3 *node_matrices, uint8_t mirrored, object_marker *out, int16_t maximum);

};

}  // namespace halo::models
