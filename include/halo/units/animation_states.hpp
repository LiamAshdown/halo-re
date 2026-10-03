/**
 * @file include/halo/units/animation_states.hpp
 * The unit animation state machine states (unit_data.animation_state, the signed byte at unit +0x2a3) as an enum class.
 */
#pragma once

#include <cstdint>

namespace halo::units {

/**
 * Values of unit_data.animation_state. 0x04..0x07 are the walking states chosen from the throttle (forward, backward,
 * left, right) and 0x08..0x0b the same four while the unit is hurt (biped_update adds 4); the 0x17..0x29 range holds the
 * scripted/action animations that block locomotion (see is_scripted_animation_state).
 */
enum class unit_animation_state_id : int8_t {
    none = -1,
    idle = 0x00,
    unknown_01 = 0x01,
    turn_in_place_a = 0x02,
    turn_in_place_b = 0x03,
    move_front = 0x04,
    move_back = 0x05,
    move_left = 0x06,
    move_right = 0x07,
    hurt_move_front = 0x08,
    hurt_move_back = 0x09,
    hurt_move_left = 0x0a,
    hurt_move_right = 0x0b,
    unknown_0c = 0x0c,
    unknown_0d = 0x0d,
    unknown_0e = 0x0e,
    unknown_0f = 0x0f,
    unknown_10 = 0x10,
    unknown_11 = 0x11,
    unknown_12 = 0x12,
    unknown_13 = 0x13,
    unknown_14 = 0x14,
    soft_landing = 0x15,
    hard_landing = 0x16,
    unknown_17 = 0x17,
    unknown_18 = 0x18,
    ready_weapon = 0x19,
    seat_enter = 0x1a,
    seat_exit = 0x1b,
    custom_animation = 0x1c,
    scripted_action = 0x1d,
    unknown_1e = 0x1e,
    unknown_1f = 0x1f,
    unknown_20 = 0x20,
    throwing_grenade = 0x21,
    unknown_22 = 0x22,
    unknown_23 = 0x23,
    unknown_24 = 0x24,
    unknown_25 = 0x25,
    unknown_26 = 0x26,
    unknown_27 = 0x27,
    unknown_28 = 0x28,
    unknown_29 = 0x29,
};

/** The animation state of a raw byte/word as stored in the unit record. */
constexpr unit_animation_state_id animation_state_id(int value) noexcept
{
    return static_cast<unit_animation_state_id>(static_cast<int8_t>(value));
}

/** The numeric value of a state, for the places that still compare or store it as an integer. */
constexpr int animation_state_value(unit_animation_state_id state) noexcept
{
    return static_cast<int8_t>(state);
}

/** True for the states that play a scripted or action animation (the unit cannot start melee, grenades or locomotion in them). */
constexpr bool is_scripted_animation_state(unit_animation_state_id state) noexcept
{
    switch (state) {
    case unit_animation_state_id::unknown_17:
    case unit_animation_state_id::unknown_18:
    case unit_animation_state_id::ready_weapon:
    case unit_animation_state_id::seat_enter:
    case unit_animation_state_id::seat_exit:
    case unit_animation_state_id::scripted_action:
    case unit_animation_state_id::unknown_1e:
    case unit_animation_state_id::unknown_1f:
    case unit_animation_state_id::unknown_20:
    case unit_animation_state_id::throwing_grenade:
    case unit_animation_state_id::unknown_22:
    case unit_animation_state_id::unknown_23:
    case unit_animation_state_id::unknown_27:
    case unit_animation_state_id::unknown_29:
        return true;
    default:
        return false;
    }
}

}  // namespace halo::units
