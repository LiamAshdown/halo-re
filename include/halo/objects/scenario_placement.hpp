/**
 * @file include/halo/objects/scenario_placement.hpp
 * The leading fields every scenario object placement record shares (ScenarioScenery, ScenarioVehicle, ScenarioEquipment, ...), the
 * palette entry that names the definition tag, and typed access to the blocks the object factory walks by a per-type stride.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include "tags.h"
#include "halo/core/flags.hpp"
#include "halo/objects/record_access.hpp"

namespace halo::objects {

/** Bits of a placement record's not_placed field (types/tags.h `ScenarioSpawnNotPlaced`). */
enum class scenario_not_placed_flag : uint16_t {
    none = 0,
    automatically = 0x1,
    on_easy = 0x2,
    on_normal = 0x4,
    on_hard = 0x8,
    use_player_appearance = 0x10,
};

/** Placement record prefix: the first 0x24 bytes are laid out identically in every object type's placement block. */
struct scenario_placement_header {
    int16_t type;
    int16_t name;
    uint16_t not_placed;
    int16_t desired_permutation;
    Point3D position;
    Euler3D rotation;
    uint16_t bsp_indices;
    uint8_t pad_22[2];
};

static_assert(sizeof(scenario_placement_header) == 0x20 + 4);
static_assert(offsetof(scenario_placement_header, position) == offsetof(ScenarioScenery, position));
static_assert(offsetof(scenario_placement_header, rotation) == offsetof(ScenarioScenery, rotation));
static_assert(offsetof(scenario_placement_header, bsp_indices) == offsetof(ScenarioScenery, bsp_indices));
static_assert(offsetof(scenario_placement_header, bsp_indices) == 0x20);
static_assert(offsetof(ScenarioVehicle, multiplayer_team_index) == 0x58);
static_assert(offsetof(ScenarioVehicle, multiplayer_spawn_flags) == 0x5a);
static_assert(sizeof(ScenarioSceneryPalette) == 0x30 && offsetof(ScenarioSceneryPalette, name.tag_id) == 0xc);
static_assert(sizeof(ScenarioObjectName) == 0x24 && offsetof(ScenarioObjectName, object_type) == 0x20);

/** Placement record `index` of a block whose records are `stride` bytes long. */
inline scenario_placement_header &placement_at(const TagReflexive &block, int32_t index, int32_t stride)
{
    return *reinterpret_cast<scenario_placement_header *>(reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(block.pointer)) +
                                                          static_cast<ptrdiff_t>(index) * stride);
}

/** Definition tag the palette entry `type` names. Every palette block has the same 0x30-byte entry. */
inline datum_index palette_tag_of(const TagReflexive &palette, int16_t type)
{
    return tag_handle(block_element<ScenarioSceneryPalette>(palette, type).name);
}

}  // namespace halo::objects

namespace halo {
template <> struct enable_bit_flags<objects::scenario_not_placed_flag> : std::true_type {};
}  // namespace halo
