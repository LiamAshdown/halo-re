#pragma once

#include <cstdint>

#include "halo/core/flags.hpp"

namespace halo::game {

/**
 * Bits of `game_variant::flags` (the option bitfield of a multiplayer game type, offset 0x38).
 *
 * Names follow the engine's use sites: shields_disabled makes game_engine_clear_unit_shields_when_disabled strip
 * unit shields, maximum_grenades makes the spawn loadout take the tag maxima, and so on. Slayer forces
 * individual_scoring and slayer_default on.
 */
enum class game_variant_flags : uint32_t {
    none = 0,
    individual_scoring = 1u << 0,
    reserved_1 = 1u << 1,
    maximum_grenades = 1u << 2,
    shields_disabled = 1u << 3,
    invisible_players = 1u << 4,
    loadout_override = 1u << 5,
    hide_radar_blips = 1u << 6,
    object_placement_filter = 1u << 7,
    slayer_default = 1u << 8,
};

}  // namespace halo::game

namespace halo {
template <> struct enable_bit_flags<game::game_variant_flags> : std::true_type {};
}  // namespace halo

namespace halo::game {

/** True when `flag` is set in the raw `game_variant::flags` word. */
constexpr bool variant_flag_set(uint32_t flags, game_variant_flags flag) noexcept {
    return (flags & to_bits(flag)) != 0;
}

}  // namespace halo::game
