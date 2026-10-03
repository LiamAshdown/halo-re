/**
 * @file include/halo/units/flags.hpp
 * Bit-flag enums for the unit, biped and vehicle runtime records (generated from the named bits in types/units.h).
 */
#pragma once

#include <cstdint>
#include "halo/core/flags.hpp"
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
    unknown_10 = 0x00000010,
    unknown_20 = 0x00000020,
    unknown_40 = 0x40,
    disoriented = 0x00000080,
    permutation_dirty = 0x00000100,
    unknown_200 = 0x00000200,
    unknown_400 = 0x400,
    unknown_800 = 0x800,
    unknown_1000 = 0x00001000,
    unknown_2000 = 0x00002000,
    unknown_4000 = 0x00004000,
    detached = 0x00008000,
    unknown_10000 = 0x10000,
    permutation_chosen = 0x00020000,
    unknown_40000 = 0x40000,
    unknown_80000 = 0x00080000,
    delete_when_dropped = 0x00100000,
    unknown_200000 = 0x200000,
    unknown_400000 = 0x400000,
    unknown_800000 = 0x00800000,
    unknown_1000000 = 0x01000000,
    idle_turn_seeded = 0x02000000,
    unknown_4000000 = 0x04000000,
    unknown_8000000 = 0x08000000,
    unknown_10000000 = 0x10000000,
    unknown_20000000 = 0x20000000,
    unknown_40000000 = 0x40000000,
    unknown_80000000 = 0x80000000,
};

/** unit_data.control_flags and persistent_control_flags. */
enum class unit_control_flag : uint32_t {
    none = 0,
    crouch = 0x0001,
    jump = 0x0002,
    unknown_4 = 0x0004,
    unknown_8 = 0x0008,
    unknown_10 = 0x0010,
    exact_facing = 0x0020,
    action = 0x0040,
    unknown_80 = 0x0080,
    look_dont_turn = 0x0100,
    force_alert = 0x0200,
    reload = 0x0400,
    primary_trigger = 0x0800,
    secondary_trigger = 0x1000,
    grenade = 0x2000,
    exchange_weapon = 0x4000,
    unknown_8000 = 0x8000,
    unknown_10000 = 0x10000,
    unknown_20000 = 0x20000,
    unknown_40000 = 0x40000,
    unknown_80000 = 0x80000,
    unknown_100000 = 0x100000,
    unknown_200000 = 0x200000,
    unknown_400000 = 0x400000,
    unknown_800000 = 0x800000,
    unknown_1000000 = 0x1000000,
    unknown_2000000 = 0x2000000,
    unknown_4000000 = 0x4000000,
    unknown_8000000 = 0x8000000,
    unknown_10000000 = 0x10000000,
    unknown_20000000 = 0x20000000,
    unknown_40000000 = 0x40000000,
    unknown_80000000 = 0x80000000,
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
    unknown_100 = 0x100,
    unknown_200 = 0x200,
    unknown_400 = 0x400,
    unknown_800 = 0x800,
    unknown_1000 = 0x1000,
    unknown_2000 = 0x2000,
    unknown_4000 = 0x4000,
    unknown_8000 = 0x8000,
};

/** biped_movement_solver_data.flags. */
enum class biped_movement_solver_flag : uint32_t {
    none = 0,
    airborne = 0x0001,
    jumping = 0x0002,
    crouching = 0x0004,
    crouch_began = 0x0008,
    flying = 0x0010,
    unknown_20 = 0x0020,
    unknown_40 = 0x0040,
    dead = 0x0080,
    passes_through_bipeds = 0x0100,
    climbs_any_surface = 0x0200,
    unknown_400 = 0x400,
    unknown_800 = 0x800,
    unknown_1000 = 0x1000,
    unknown_2000 = 0x2000,
    unknown_4000 = 0x4000,
    unknown_8000 = 0x8000,
    unknown_10000 = 0x10000,
    unknown_20000 = 0x20000,
    unknown_40000 = 0x40000,
    unknown_80000 = 0x80000,
    unknown_100000 = 0x100000,
    unknown_200000 = 0x200000,
    unknown_400000 = 0x400000,
    unknown_800000 = 0x800000,
    unknown_1000000 = 0x1000000,
    unknown_2000000 = 0x2000000,
    unknown_4000000 = 0x4000000,
    unknown_8000000 = 0x8000000,
    unknown_10000000 = 0x10000000,
    unknown_20000000 = 0x20000000,
    unknown_40000000 = 0x40000000,
    unknown_80000000 = 0x80000000,
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
    unknown_4 = 0x04,
    unknown_8 = 0x08,
    landing_latch = 0x10,
    ground_adjust_dirty = 0x20,
    unknown_40 = 0x40,
    unknown_80 = 0x80,
    unknown_100 = 0x100,
    unknown_200 = 0x200,
    unknown_400 = 0x400,
    unknown_800 = 0x800,
    unknown_1000 = 0x1000,
    unknown_2000 = 0x2000,
    unknown_4000 = 0x4000,
    unknown_8000 = 0x8000,
    unknown_10000 = 0x10000,
    unknown_20000 = 0x20000,
    unknown_40000 = 0x40000,
    unknown_80000 = 0x80000,
    unknown_100000 = 0x100000,
    unknown_200000 = 0x200000,
    unknown_400000 = 0x400000,
    unknown_800000 = 0x800000,
    unknown_1000000 = 0x1000000,
    unknown_2000000 = 0x2000000,
    unknown_4000000 = 0x4000000,
    unknown_8000000 = 0x8000000,
    unknown_10000000 = 0x10000000,
    unknown_20000000 = 0x20000000,
    unknown_40000000 = 0x40000000,
    unknown_80000000 = 0x80000000,
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
    unknown_100 = 0x100,
    unknown_200 = 0x200,
    unknown_400 = 0x400,
    unknown_800 = 0x800,
    unknown_1000 = 0x1000,
    unknown_2000 = 0x2000,
    unknown_4000 = 0x4000,
    unknown_8000 = 0x8000,
};

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
