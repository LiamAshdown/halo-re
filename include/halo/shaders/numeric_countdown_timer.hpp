/**
 * @file include/halo/shaders/numeric_countdown_timer.hpp
 * The millisecond countdown displayed by numeric transparent_chicago shaders.
 */
#pragma once

#include "halo/shaders/shaders_types.hpp"

namespace halo::shaders {

/**
 * The single numeric countdown timer driven by the hs script functions and displayed digit by digit by
 * transparent_chicago shaders. Its state is the engine's fixed globals; the class only groups the operations.
 */
struct numeric_countdown_timer {
    /**
     * Returns one decimal digit (or the raw millisecond count for the raw index) of the remaining time. The result is the
     * bitmap frame index the chicago draws use; an unknown index returns 0.
     *
     * @address 0x5400c0
     */
    static int16_t get_digit(int16_t digit_index);

    /**
     * Counts the timer down by the milliseconds elapsed since the previous call, clamping at 0. Does nothing while the
     * timer is not running. Called once per tick from game_effects_update.
     *
     * @address 0x540240
     */
    static void update(void);

};

}  // namespace halo::shaders
