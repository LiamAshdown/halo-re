#pragma once

#include <cstdint>

#include "tags.h"
#include "halo/core/datum.hpp"
#include "halo/scenario/api.hpp"

namespace halo::scenario {

/**
 * Cluster index of a structure BSP leaf. The leaf handle carries a "no leaf" sign bit that is
 * masked off here; callers test for -1 themselves.
 */
inline int16_t structure_leaf_cluster(uint32_t leaf) noexcept
{
    return static_cast<int16_t>(
        reinterpret_cast<ScenarioStructureBSPLeaf *>(globals().structure_bsp->leaves.pointer)[leaf & k_leaf_index_mask].cluster);
}

}  // namespace halo::scenario
