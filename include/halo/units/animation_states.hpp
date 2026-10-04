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
    gesture = 0x01,
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
    slide_front = 0x0c,
    slide_back = 0x0d,
    slide_left = 0x0e,
    slide_right = 0x0f,
    flying_front = 0x10,
    flying_back = 0x11,
    flying_left = 0x12,
    flying_right = 0x13,
    airborne = 0x14,
    soft_landing = 0x15,
    hard_landing = 0x16,
    hard_ping = 0x17,
    dying_airborne = 0x18,
    dying = 0x19,
    seat_enter = 0x1a,
    seat_exit = 0x1b,
    custom_animation = 0x1c,
    scripted_action = 0x1d,
    melee_attack = 0x1e,
    melee_airborne = 0x1f,
    melee_continuous = 0x20,
    throwing_grenade = 0x21,
    resurrect_front = 0x22,
    resurrect_back = 0x23,
    feeding = 0x24,
    opening = 0x25,
    closing = 0x26,
    leap_start = 0x27,
    leap_airborne = 0x28,
    leap_melee = 0x29,
};

/** Number of unit animation states (the last defined state is 0x2b, hovering). */
inline constexpr int k_unit_animation_state_count = 0x2c;

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
    case unit_animation_state_id::hard_ping:
    case unit_animation_state_id::dying_airborne:
    case unit_animation_state_id::dying:
    case unit_animation_state_id::seat_enter:
    case unit_animation_state_id::seat_exit:
    case unit_animation_state_id::scripted_action:
    case unit_animation_state_id::melee_attack:
    case unit_animation_state_id::melee_airborne:
    case unit_animation_state_id::melee_continuous:
    case unit_animation_state_id::throwing_grenade:
    case unit_animation_state_id::resurrect_front:
    case unit_animation_state_id::resurrect_back:
    case unit_animation_state_id::leap_start:
    case unit_animation_state_id::leap_melee:
        return true;
    default:
        return false;
    }
}

}  // namespace halo::units
