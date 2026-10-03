/**
 * @file src/shaders/numeric_countdown_timer.cpp
 * The millisecond countdown displayed by numeric transparent_chicago shaders.
 * The original author notes and decompiles are in docs/original/shaders/.
 */

#include "halo/shaders/shaders.hpp"
#include "halo/shaders/api.hpp"


namespace halo::shaders {

int16_t numeric_countdown_timer::get_digit(int16_t digit_index)
{
    switch (digit_index) {
    case _numeric_countdown_timer_raw:
        return (int16_t)globals().numeric_countdown_timer_remaining_ms;
    case _numeric_countdown_timer_millisecond_ones:
        return (int16_t)(globals().numeric_countdown_timer_remaining_ms % 10);
    case _numeric_countdown_timer_millisecond_tens:
        return (int16_t)(globals().numeric_countdown_timer_remaining_ms / 10 % 10);
    case _numeric_countdown_timer_millisecond_hundreds:
        return (int16_t)(globals().numeric_countdown_timer_remaining_ms / 100 % 10);
    case _numeric_countdown_timer_second_ones:
        return (int16_t)(globals().numeric_countdown_timer_remaining_ms / 1000 % 10);
    case _numeric_countdown_timer_second_tens:
        return (int16_t)(globals().numeric_countdown_timer_remaining_ms / 10000 % 6);
    case _numeric_countdown_timer_minute_ones:
        return (int16_t)(globals().numeric_countdown_timer_remaining_ms / 60000 % 10);
    case _numeric_countdown_timer_minute_tens:
        return (int16_t)(globals().numeric_countdown_timer_remaining_ms / 600000 % 6);
    case _numeric_countdown_timer_hour_ones:
        return (int16_t)(globals().numeric_countdown_timer_remaining_ms / 3600000 % 10);
    case _numeric_countdown_timer_hour_tens:
        return (int16_t)(globals().numeric_countdown_timer_remaining_ms / 36000000 % 10);
    default:
        return 0;
    }
}

void numeric_countdown_timer::update(void)
{
    int32_t new_time;

    if (!globals().numeric_countdown_timer_running) {
        return;
    }

    new_time = (globals().game_time->game_time * k_numeric_countdown_timer_milliseconds_per_second) /
               k_numeric_countdown_timer_ticks_per_second;

    if (globals().numeric_countdown_timer_last_update_ms <= new_time) {
        globals().numeric_countdown_timer_remaining_ms +=
            (globals().numeric_countdown_timer_last_update_ms - new_time);
        if (globals().numeric_countdown_timer_remaining_ms < 0) {
            globals().numeric_countdown_timer_remaining_ms = 0;
        }
    }

    globals().numeric_countdown_timer_last_update_ms = new_time;
}

}  // namespace halo::shaders
