#pragma once

#include "halo/networking/net1_common.hpp"

namespace halo::networking {

/**
 * Behaviour group for the original `network_bandwidth_*` functions; every member is the original function body moved unchanged.
 */
class BandwidthMonitor {
public:
    BandwidthMonitor() = delete;

    static int32_t direction_name_to_index(const char *name);
    static void accumulate_received(int32_t byte_count, int32_t packet_count);
    static void accumulate_sent(int32_t byte_count, int32_t packet_count);
    static uint32_t reset();
    static uint32_t set_units_command(const char *units_name, const char *direction_name);
    static void update_();
    static int32_t unit_name_to_index(const char *name);
};

/**
 * Behaviour group for the original `network_stats_*` functions; every member is the original function body moved unchanged.
 */
class BandwidthGraphView {
public:
    network_bandwidth_graph *self;

    explicit constexpr BandwidthGraphView(network_bandwidth_graph *record) : self(record) {}

    int32_t find_peak_sample(int32_t *out_peak_countdown);
    void instance_history_reset();
    void instance_init(int32_t units_index, int32_t direction_index);
    void instance_update_layout(uint8_t force_refresh);
    void new_sample();
    void tick();
    void update_columns(int32_t new_sample);
    void rate_compute();
    void overlay_draw();
};

}
