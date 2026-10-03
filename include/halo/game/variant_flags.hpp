#pragma once

#include <cstdint>

#include "halo/core/flags.hpp"

namespace halo::game {

/**
 * Bits of `game_variant::flags` (the option bitfield of a multiplayer game type, offset 0x38).
 *
 * Names follow the handlers that test each bit in bin/halo.exe. players_on_radar (0x4b36c8
 * motion_sensor_object_is_detected: in a multiplayer game an object only shows on the motion sensor with it set;
 * 0x4635e0 game_engine_scores_tracked_individually, the HUD's radar-visible test, ORs it with "indicator is the
 * motion tracker"). friend_indicators (0x4a9a45 hud_update_player: in a team game it lets
 * hud_waypoint_draw_all_for_player draw a waypoint over every teammate). maximum_grenades (0x461498
 * game_engine_apply_player_grenade_counts takes the tag maxima). shields_disabled (0x462c23 returns its
 * complement). invisible_players (0x462cd1 drops camouflage powerups, 0x45ffd4 game_engine_tick). loadout_override
 * (0x461450: skips game_engine_spawn_player_starting_loadout so the map's starting loadout is overridden).
 * hide_radar_blips (0x4b4269 motion_sensor_render skips blip kinds 2 and 4 in a multiplayer game).
 * object_placement_filter (0x462e3a: with the per-map table flag set, weapon-list entries 1 and 0xe are not placed;
 * 0x577a9d reports it to GameSpy). Slayer forces players_on_radar and slayer_default on.
 */
enum class game_variant_flags : uint32_t {
    none = 0,
    players_on_radar = 1u << 0,
    friend_indicators = 1u << 1,
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
