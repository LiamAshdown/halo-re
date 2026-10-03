/**
 * @file include/halo/structures/limits.hpp
 * Size limits of the structure bsp visibility tables, shared with the render module.
 */
#pragma once

#include <cstdint>

namespace halo::structures {

/** Largest number of clusters the flood fills track; also the size of the per-cluster scratch tables. */
inline constexpr int32_t k_maximum_flood_clusters = 0x200;

/** Dwords of the per-cluster visibility bit array (one bit per cluster). */
inline constexpr int32_t k_cluster_visible_bit_words = 0x10;

/** Largest number of surfaces the visible surface list can hold. */
inline constexpr int32_t k_maximum_visible_surfaces = 0x4000;

}  // namespace halo::structures
