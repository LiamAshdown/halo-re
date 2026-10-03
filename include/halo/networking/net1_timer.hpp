#pragma once

#include "halo/networking/net1_common.hpp"

namespace halo::networking {

/**
 * Behaviour group for the original `network_timer_*` functions; every member is the original function body moved unchanged.
 */
class TimerView {
public:
    network_timer_pair *self;

    explicit constexpr TimerView(network_timer_pair *record) : self(record) {}

    void advance();
    void decrement_floored(int32_t decrement);
    void increment_clamped(int32_t upper_bound, int32_t increment);
    void start(int32_t duration_ms);
};

}
