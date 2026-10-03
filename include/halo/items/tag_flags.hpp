#pragma once

#include <cstdint>

#include "halo/core/flags.hpp"

namespace halo::items {

/**
 * Bits of WeaponTrigger.flags (the tag bitfield), in tag order.
 */
enum class weapon_trigger_tag_flag : uint32_t {
    none = 0,
    tracks_fired_projectile = 1u << 0,
    random_firing_effects = 1u << 1,
    can_fire_with_partial_ammo = 1u << 2,
    does_not_repeat_automatically = 1u << 3,
    locks_in_on_off_state = 1u << 4,
    projectiles_use_weapon_origin = 1u << 5,
    sticks_when_dropped = 1u << 6,
    ejects_during_chamber = 1u << 7,
    discharging_spews = 1u << 8,
    analog_rate_of_fire = 1u << 9,
    use_error_when_unzoomed = 1u << 10,
    projectile_vector_cannot_be_adjusted = 1u << 11,
    projectiles_have_identical_error = 1u << 12,
    projectile_is_client_side_only = 1u << 13,
    use_original_unit_adjust_projectile_ray = 1u << 14,
};

/**
 * Bits of Weapon.weapon_flags (the tag bitfield), in tag order.
 */
enum class weapon_tag_flag : uint32_t {
    none = 0,
    vertical_heat_display = 1u << 0,
    mutually_exclusive_triggers = 1u << 1,
    attacks_automatically_on_bump = 1u << 2,
    must_be_readied = 1u << 3,
    doesnt_count_toward_maximum = 1u << 4,
    aim_assists_only_when_zoomed = 1u << 5,
    prevents_grenade_throwing = 1u << 6,
    must_be_picked_up = 1u << 7,
    holds_triggers_when_dropped = 1u << 8,
    prevents_melee_attack = 1u << 9,
    detonates_when_dropped = 1u << 10,
    cannot_fire_at_maximum_age = 1u << 11,
    secondary_trigger_overrides_grenades = 1u << 12,
};

}  // namespace halo::items

namespace halo {
template <>
struct enable_bit_flags<items::weapon_trigger_tag_flag> : std::true_type {};
template <>
struct enable_bit_flags<items::weapon_tag_flag> : std::true_type {};
}  // namespace halo

namespace halo::items {

inline bool trigger_has(uint32_t tag_flags, weapon_trigger_tag_flag flag) noexcept
{
    return has(static_cast<weapon_trigger_tag_flag>(tag_flags), flag);
}

inline bool weapon_has(uint32_t tag_flags, weapon_tag_flag flag) noexcept
{
    return has(static_cast<weapon_tag_flag>(tag_flags), flag);
}

}  // namespace halo::items
