#include "halo/networking/net1_timer.hpp"
#include "halo/cseries/api.hpp"
#include "halo/networking/api.hpp"


namespace halo::networking {

/**
 * Advances timer->last_tick_ms to now and, if time has actually elapsed since the previous
 * tick, subtracts that elapsed time from timer->remaining_ms (floored at 0).
 *
 * @address 0x4deb50
 */
void TimerView::advance()
{
    network_timer_pair *timer = self;
    large_integer counter;
    int32_t now_ms;
    int32_t previous_tick_ms;
    int32_t elapsed;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    previous_tick_ms = timer->last_tick_ms;
    timer->last_tick_ms = now_ms;
    if (previous_tick_ms < now_ms) {
        elapsed = now_ms - previous_tick_ms;
        if (elapsed < timer->remaining_ms) {
            timer->remaining_ms = timer->remaining_ms - elapsed;
        } else {
            timer->remaining_ms = 0;
        }
    }
}

/**
 * out/phase4/networking_functions.md: "Register-based helper that advances the shared
 * timer via network_timer_advance then decrements *in_EAX by unaff_EDI, floored at zero." Uses the same
 * [remaining_ms, last_tick_ms] layout as network_timer_advance.c.
 *
 * @address 0x4debd0
 */
void TimerView::decrement_floored(int32_t decrement)
{
    network_timer_pair *timer = self;
    halo::networking::network_timer_advance(timer);
    if (decrement < timer->remaining_ms) {
        timer->remaining_ms = timer->remaining_ms - decrement;
        return;
    }
    timer->remaining_ms = 0;
}

/**
 * out/phase4/networking_functions.md: "Register-based helper that advances the shared
 * timer via network_timer_advance then adds an increment to *in_EAX, clamping the result to an upper
 * bound." Uses the same [remaining_ms, last_tick_ms] layout as network_timer_advance.c; `timer`
 * is passed straight through to that call.
 *
 * @address 0x4debb0
 */
void TimerView::increment_clamped(int32_t upper_bound, int32_t increment)
{
    network_timer_pair *timer = self;
    int32_t sum;

    halo::networking::network_timer_advance(timer);
    sum = timer->remaining_ms + increment;
    if (sum < increment) {
        timer->remaining_ms = upper_bound;
        return;
    }
    timer->remaining_ms = sum;
    if (upper_bound < sum) {
        sum = upper_bound;
    }
    timer->remaining_ms = sum;
}

/**
 * out/phase4/networking_functions.md: "Initialises a timer pair pointed to by
 * unaff_ESI with the current performance-counter time and the requested duration parameter."
 * Uses the same [remaining_ms, last_tick_ms] layout as network_timer_advance.c.
 *
 * @address 0x4debf0
 */
void TimerView::start(int32_t duration_ms)
{
    network_timer_pair *timer = self;
    large_integer counter;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    timer->remaining_ms = duration_ms;
    timer->last_tick_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
}

}
