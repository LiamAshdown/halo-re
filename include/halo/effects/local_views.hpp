#pragma once

#include <cstddef>
#include <cstdint>

#include "halo/effects/types.hpp"

static_assert(offsetof(first_person_weapon_interface, node_matrices) == 0x108c);
static_assert(offsetof(first_person_weapon_interface, weapon_index) == 8);
static_assert(offsetof(player_globals, cluster_pvs) == 0x18);
static_assert(offsetof(real_matrix4x3, position) == 0x28);
static_assert(sizeof(real_matrix4x3) == 0x34);

namespace halo::effects {

/**
 * Marker node matrix of a first person weapon model: the interface keeps 64 real_matrix4x3 nodes.
 */
inline real_matrix4x3 *first_person_marker_node(first_person_weapon_interface *interfaces, int32_t weapon_index, int32_t node_index) noexcept
{
    return reinterpret_cast<real_matrix4x3 *>(interfaces[weapon_index].node_matrices) + node_index;
}

/**
 * Node matrix `node_index` of an object, from the node block the object carries after its header.
 */
inline real_matrix4x3 *object_marker_node(object *owner, int32_t node_index) noexcept
{
    return reinterpret_cast<real_matrix4x3 *>(reinterpret_cast<uint8_t *>(owner) + owner->nodes.offset) + node_index;
}

/**
 * Which of the two cluster visibility bit sets in player_globals a query reads: the one built for
 * deterministic (network consistent) effects, or the one the local players actually see.
 */
enum class cluster_visibility : int32_t {
    deterministic = 0,
    local_view = 0x10,
};

inline uint32_t *cluster_visibility_bits(player_globals *globals, cluster_visibility which) noexcept
{
    return &globals->cluster_pvs[static_cast<int32_t>(which)];
}

}  // namespace halo::effects
