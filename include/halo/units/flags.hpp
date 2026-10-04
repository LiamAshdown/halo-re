/**
 * @file include/halo/units/flags.hpp
 * Bit-flag enums for the unit, biped and vehicle runtime records (generated from the named bits in types/units.h).
 */
#pragma once

#include <cstdint>
#include "halo/core/flags.hpp"
#include "halo/core/collision_flags.hpp"
#include "halo/core/flag_bits.hpp"

namespace halo::units {

using halo::operator|;
using halo::operator&;
using halo::operator^;
using halo::operator~;
using halo::operator|=;
using halo::operator&=;
using halo::operator^=;
using halo::has;
using halo::any;
using halo::to_bits;

/** unit_data.flags (the dword at unit +0x204). */
enum class unit_flag : uint32_t {
    none = 0,
    unattended = 0x00000001,
    unknown_2 = 0x2,
    unknown_4 = 0x4,
    unknown_8 = 0x8,
    active_camouflaged = 0x00000010,
    super_camouflaged = 0x00000020,
    unknown_40 = 0x40,
    disoriented = 0x00000080,
    permutation_dirty = 1u << 8,
    unknown_200 = 1u << 9,
    ignored_by_actors = 1u << 10,
    preferred_target = 1u << 11,
    no_falling_damage = 1u << 12,
    feign_death_allowed = 1u << 13,
    aim_without_turning = 1u << 14,
    detached = 1u << 15,
    not_enterable_by_player = 1u << 16,
    permutation_chosen = 1u << 17,
    unknown_40000 = 1u << 18,
    integrated_light_on = 1u << 19,
    delete_when_dropped = 1u << 20,
    unknown_200000 = 1u << 21,
    cannot_blink = 1u << 22,
    impervious = 1u << 23,
    suspended = 1u << 24,
    idle_turn_seeded = 1u << 25,
    integrated_night_vision_on = 1u << 26,
    possessed_by_recording = 1u << 27,
    desired_integrated_light_on = 1u << 28,
    desired_integrated_light_off = 1u << 29,
    unknown_40000000 = 1u << 30,
    unknown_80000000 = 1u << 31,
};

/** unit_data.control_flags and persistent_control_flags. */
enum class unit_control_flag : uint32_t {
    none = 0,
    crouch = 0x0001,
    jump = 0x0002,
    user_animation_1 = 0x0004,
    user_animation_2 = 0x0008,
    integrated_light = 0x0010,
    exact_facing = 0x0020,
    action = 0x0040,
    use_equipment = 0x0080,
    look_dont_turn = 1u << 8,
    force_alert = 1u << 9,
    reload = 1u << 10,
    primary_trigger = 1u << 11,
    secondary_trigger = 1u << 12,
    grenade = 1u << 13,
    exchange_weapon = 1u << 14,
    unknown_8000 = 1u << 15,
    unknown_10000 = 1u << 16,
    unknown_20000 = 1u << 17,
    unknown_40000 = 1u << 18,
    unknown_80000 = 1u << 19,
    unknown_100000 = 1u << 20,
    unknown_200000 = 1u << 21,
    unknown_400000 = 1u << 22,
    unknown_800000 = 1u << 23,
    unknown_1000000 = 1u << 24,
    unknown_2000000 = 1u << 25,
    unknown_4000000 = 1u << 26,
    unknown_8000000 = 1u << 27,
    unknown_10000000 = 1u << 28,
    unknown_20000000 = 1u << 29,
    unknown_40000000 = 1u << 30,
    unknown_80000000 = 1u << 31,
};

/** unit_data.animation_state_flags. */
enum class unit_animation_state_flag : uint16_t {
    none = 0,
    action_active = 0x0001,
    aiming_enabled = 0x0002,
    unknown_4 = 0x0004,
    unknown_8 = 0x0008,
    unknown_10 = 0x10,
    unknown_20 = 0x20,
    unknown_40 = 0x40,
    unknown_80 = 0x80,
    unknown_100 = 1u << 8,
    unknown_200 = 1u << 9,
    unknown_400 = 1u << 10,
    unknown_800 = 1u << 11,
    unknown_1000 = 1u << 12,
    unknown_2000 = 1u << 13,
    unknown_4000 = 1u << 14,
    unknown_8000 = 1u << 15,
};

/** biped_movement_solver_data.flags. */
enum class biped_movement_solver_flag : uint32_t {
    none = 0,
    airborne = 0x0001,
    jumping = 0x0002,
    crouching = 0x0004,
    crouch_began = 0x0008,
    flying = 0x0010,
    absolute_movement = 0x0020,
    no_collision = 0x0040,
    dead = 0x0080,
    passes_through_bipeds = 1u << 8,
    climbs_any_surface = 1u << 9,
    unknown_400 = 1u << 10,
    unknown_800 = 1u << 11,
    unknown_1000 = 1u << 12,
    unknown_2000 = 1u << 13,
    unknown_4000 = 1u << 14,
    unknown_8000 = 1u << 15,
    unknown_10000 = 1u << 16,
    unknown_20000 = 1u << 17,
    unknown_40000 = 1u << 18,
    unknown_80000 = 1u << 19,
    unknown_100000 = 1u << 20,
    unknown_200000 = 1u << 21,
    unknown_400000 = 1u << 22,
    unknown_800000 = 1u << 23,
    unknown_1000000 = 1u << 24,
    unknown_2000000 = 1u << 25,
    unknown_4000000 = 1u << 26,
    unknown_8000000 = 1u << 27,
    unknown_10000000 = 1u << 28,
    unknown_20000000 = 1u << 29,
    unknown_40000000 = 1u << 30,
    unknown_80000000 = 1u << 31,
};

/** biped_movement_solver_data.result_flags. */
enum class biped_movement_result_flag : uint8_t {
    none = 0,
    airborne = 0x01,
    jumping = 0x02,
    landed = 0x04,
    unknown_8 = 0x8,
    moving = 0x10,
    unknown_20 = 0x20,
    unknown_40 = 0x40,
    unknown_80 = 0x80,
};

/** biped_data.flags (the dword at biped +0x4cc). */
enum class biped_flag : uint32_t {
    none = 0,
    airborne = 0x01,
    jumping = 0x02,
    absolute_movement = 0x04,
    no_collision = 0x08,
    passes_through_bipeds = 0x10,
    ground_adjust_dirty = 0x20,
    unknown_40 = 0x40,
    unknown_80 = 0x80,
    unknown_100 = 1u << 8,
    unknown_200 = 1u << 9,
    unknown_400 = 1u << 10,
    unknown_800 = 1u << 11,
    unknown_1000 = 1u << 12,
    unknown_2000 = 1u << 13,
    unknown_4000 = 1u << 14,
    unknown_8000 = 1u << 15,
    unknown_10000 = 1u << 16,
    unknown_20000 = 1u << 17,
    unknown_40000 = 1u << 18,
    unknown_80000 = 1u << 19,
    unknown_100000 = 1u << 20,
    unknown_200000 = 1u << 21,
    unknown_400000 = 1u << 22,
    unknown_800000 = 1u << 23,
    unknown_1000000 = 1u << 24,
    unknown_2000000 = 1u << 25,
    unknown_4000000 = 1u << 26,
    unknown_8000000 = 1u << 27,
    unknown_10000000 = 1u << 28,
    unknown_20000000 = 1u << 29,
    unknown_40000000 = 1u << 30,
    unknown_80000000 = 1u << 31,
};

/** vehicle_data.flags (the word at vehicle +0x4cc). */
enum class vehicle_flag : uint16_t {
    none = 0,
    over_blur_speed = 0x01,
    unknown_2 = 0x2,
    has_ground_contact = 0x04,
    hovering = 0x08,
    controls_active = 0x10,
    unknown_20 = 0x20,
    unknown_40 = 0x40,
    unknown_80 = 0x80,
    unknown_100 = 1u << 8,
    unknown_200 = 1u << 9,
    unknown_400 = 1u << 10,
    unknown_800 = 1u << 11,
    unknown_1000 = 1u << 12,
    unknown_2000 = 1u << 13,
    unknown_4000 = 1u << 14,
    unknown_8000 = 1u << 15,
};


/** What a dead unit's movement query collides with: structure bsp, nearby objects, scenery and machines (0xc0a0). */
inline constexpr uint32_t k_dead_unit_collision_flags =
    halo::to_bits(halo::collision_test_flag::structure_bsp | halo::collision_test_flag::nearby_objects | halo::collision_test_flag::object_scenery |
                  halo::collision_test_flag::object_machine);

/** The movement query of a unit that passes through bipeds: the dead set plus vehicles (0xc2a0). */
inline constexpr uint32_t k_pass_through_bipeds_collision_flags =
    k_dead_unit_collision_flags | halo::to_bits(halo::collision_test_flag::object_vehicle);

/** The movement query of an ordinary unit: bipeds as well (0x20c3a0, the top bit is not named yet). */
inline constexpr uint32_t k_unit_collision_flags =
    k_pass_through_bipeds_collision_flags | halo::to_bits(halo::collision_test_flag::object_biped | halo::collision_test_flag::unknown_200000);

/** The sphere query that gathers the ground a biped adjusts to: the dead set that ignores invisible surfaces (0xc0a8). */
inline constexpr uint32_t k_ground_adjust_query_flags = k_dead_unit_collision_flags | halo::to_bits(halo::collision_test_flag::ignore_invisible);

static_assert(k_dead_unit_collision_flags == 0xc0a0 && k_pass_through_bipeds_collision_flags == 0xc2a0 && k_unit_collision_flags == 0x20c3a0 && k_ground_adjust_query_flags == 0xc0a8);

}  // namespace halo::units

namespace halo {
template <> struct enable_bit_flags<units::unit_flag> : std::true_type {};
template <> struct enable_bit_flags<units::unit_control_flag> : std::true_type {};
template <> struct enable_bit_flags<units::unit_animation_state_flag> : std::true_type {};
template <> struct enable_bit_flags<units::biped_movement_solver_flag> : std::true_type {};
template <> struct enable_bit_flags<units::biped_movement_result_flag> : std::true_type {};
template <> struct enable_bit_flags<units::biped_flag> : std::true_type {};
template <> struct enable_bit_flags<units::vehicle_flag> : std::true_type {};
}  // namespace halo
