#include "halo/networking/net1_bandwidth.hpp"
#include <stdio.h>
#include "halo/cseries/api.hpp"

extern "C" {
extern const char *network_bandwidth_direction_label_table[2];
extern network_bandwidth_graph network_bandwidth_graph_globals;
extern uint32_t network_bandwidth_graph_default_interval_ms;
extern void network_bandwidth_graph_instance_update_layout(network_bandwidth_graph *graph, uint8_t force_refresh);
extern uint8_t network_bandwidth_overlay_enabled;
extern int32_t network_bandwidth_unit_name_to_index(const char *name);
extern int32_t network_bandwidth_direction_name_to_index(const char *name);
extern void network_bandwidth_graph_instance_init(network_bandwidth_graph *graph, int32_t units_index, int32_t direction_index);
extern network_screen_point game_window_top_left;
extern network_screen_point game_window_bottom_right;
extern void network_bandwidth_graph_instance_history_reset(network_bandwidth_graph *graph);
extern void network_stats_overlay_draw(network_bandwidth_graph *graph);
extern const char *network_bandwidth_units_label_table[2];
extern int64_t performance_frequency;
extern void network_bandwidth_graph_update_columns(int32_t new_sample, network_bandwidth_graph *graph);
extern void network_bandwidth_graph_new_sample(network_bandwidth_graph *graph);
extern int32_t network_bandwidth_graph_find_peak_sample(int32_t *out_peak_countdown, network_bandwidth_graph *graph);
extern void ***rasterizer_device;
extern uint32_t renderer_unknown_6e1af0;
extern uint32_t renderer_unknown_6e1af8;
extern uint32_t renderer_unknown_69e468;
extern uint8_t rasterizer_software_vertex_processing;
extern network_screen_point network_stats_overlay_text_rect_min;
extern network_screen_point network_stats_overlay_text_rect_max;
extern float hud_text_draw_color_a;
extern float hud_text_draw_color_r;
extern float hud_text_draw_color_g;
extern float hud_text_draw_color_b;
extern uint16_t hud_text_draw_background_mode;
extern const char decimal_format_string[];
extern void rasterizer_set_shader_stage_config(int32_t stage);
extern int32_t hud_text_draw_configure(int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, int32_t f);
extern void chimera__draw_8_bit_text(int32_t x, int32_t y, const char *text);
}

namespace halo::networking {

/**
 * out/phase4/networking_functions.md summary ("Converts a direction-name string (e.g.
 * 'sent'/'recv') into the corresponding index for the network bandwidth debug graph"); identical
 * shape to network_bandwidth_unit_name_to_index (0x4d8a20) one function above it, indexing
 * network_bandwidth_direction_label_table, already named from
 * src/networking/network_bandwidth_graph_instance_update_layout.c and
 * src/networking/network_bandwidth_graph_update.c (0x0065d430, indexed by
 * network_bandwidth_graph::direction_index). The two extra globals Ghidra lists
 *
 * @address 0x4d8a50
 */
int32_t BandwidthMonitor::direction_name_to_index(const char *name)
{
    int32_t i;

    i = 0;
    do {
        if (_stricmp(name, network_bandwidth_direction_label_table[i]) == 0) {
            return i;
        }
        i = i + 1;
    } while (i < 2);
    return -1;
}

/**
 * out/phase4/networking_functions.md summary ("Accumulates incoming (received) byte
 * counts for the network bandwidth debug graph, updating the displayed total only when
 * 'received' is the active view"); mirrors network_bandwidth_graph_accumulate_sent.c exactly,
 * with direction_index == 1 and bits_received instead of direction_index == 0 and bits_sent.
 *
 * @address 0x4d7a50
 */
void BandwidthMonitor::accumulate_received(int32_t byte_count, int32_t packet_count)
{
    if (network_bandwidth_graph_globals.direction_index == 1) {
        if (network_bandwidth_graph_globals.units_index == 0) {
            network_bandwidth_graph_globals.pending_sample += byte_count;
        } else if (network_bandwidth_graph_globals.units_index == 1) {
            network_bandwidth_graph_globals.pending_sample += packet_count;
        }
        if (network_bandwidth_graph_globals.needs_layout != 0) {
            network_bandwidth_graph_globals.last_sample_ms = halo::cseries::time_query_performance_counter_ms();
            network_bandwidth_graph_globals.needs_layout = 0;
        }
    }
    network_bandwidth_graph_globals.bits_received += byte_count * 8;
    if (network_bandwidth_graph_globals.rate_base_ms == 0) {
        network_bandwidth_graph_globals.rate_base_ms = halo::cseries::time_query_performance_counter_ms();
    }
}

/**
 * out/phase4/networking_functions.md summary ("Accumulates outgoing (sent) byte
 * counts for the network bandwidth debug graph, updating the displayed total only when 'sent'
 * is the active view"); types/networking.h network_bandwidth_graph (units_index,
 * direction_index, pending_sample, needs_layout, last_sample_ms, bits_sent, rate_base_ms).
 *
 * @address 0x4d79d0
 */
void BandwidthMonitor::accumulate_sent(int32_t byte_count, int32_t packet_count)
{
    if (network_bandwidth_graph_globals.direction_index == 0) {
        if (network_bandwidth_graph_globals.units_index == 0) {
            network_bandwidth_graph_globals.pending_sample += byte_count;
        } else if (network_bandwidth_graph_globals.units_index == 1) {
            network_bandwidth_graph_globals.pending_sample += packet_count;
        }
        if (network_bandwidth_graph_globals.needs_layout != 0) {
            network_bandwidth_graph_globals.last_sample_ms = halo::cseries::time_query_performance_counter_ms();
            network_bandwidth_graph_globals.needs_layout = 0;
        }
    }
    network_bandwidth_graph_globals.bits_sent += byte_count * 8;
    if (network_bandwidth_graph_globals.rate_base_ms == 0) {
        network_bandwidth_graph_globals.rate_base_ms = halo::cseries::time_query_performance_counter_ms();
    }
}

/**
 * out/phase4/networking_functions.md summary ("Resets the global network
 * bandwidth-graph debug overlay's counters and mode selectors to their default state");
 * types/networking.h network_bandwidth_graph and network_bandwidth_graph_globals /
 * network_bandwidth_graph_default_interval_ms.
 *
 * @address 0x4d7980
 */
uint32_t BandwidthMonitor::reset()
{
    network_bandwidth_graph_globals.last_sample_ms = 0;
    network_bandwidth_graph_globals.units_index = 0;
    network_bandwidth_graph_globals.bits_sent = 0;
    network_bandwidth_graph_globals.bits_received = 0;
    network_bandwidth_graph_globals.rate_base_ms = 0;
    network_bandwidth_graph_globals.needs_layout = 1;
    network_bandwidth_graph_globals.sample_interval_ms = network_bandwidth_graph_default_interval_ms;
    network_bandwidth_graph_globals.direction_index = 1;
    network_bandwidth_graph_instance_update_layout(&network_bandwidth_graph_globals, 1);
    return 1;
}

/**
 * out/phase2/results/networking_01.json / symbols/review_queue.txt)
 * address 0x4d7d90, size 73 bytes
 * name confidence: 0.45   rewrite confidence: 0.6
 * out/phase4/networking_functions.md summary ("Parses a units/direction
 * command-line pair (e.g. 'bytes sent') and, if valid, (re)configures the network bandwidth
 * debug graph to display them"); out/phase2/results/networking_01.json evidence ("only
 * proceeds if the overlay is enabled (network_bandwidth_overlay_enabled), resolves a units token via
 *
 * @address 0x4d7d90
 */
uint32_t BandwidthMonitor::set_units_command(const char *units_name, const char *direction_name)
{
    int32_t units_index;
    int32_t direction_index;

    if (!network_bandwidth_overlay_enabled) {
        return 0;
    }

    units_index = network_bandwidth_unit_name_to_index(units_name);
    direction_index = network_bandwidth_direction_name_to_index(direction_name);
    if (units_index != -1 && direction_index != -1) {
        network_bandwidth_graph_instance_init(&network_bandwidth_graph_globals, units_index, direction_index);
        return 1;
    }
    return 0;
}

/**
 * out/phase4/networking_functions.md summary ("Recomputes the on-screen layout and
 * label of the global network bandwidth debug graph whenever the screen size changes, then
 * redraws it"); every field this function touches is the SAME layout this batch's
 * network_bandwidth_graph_instance_update_layout.c (0x4d7e20) computes for a per-instance
 * graph, just inlined against the global singleton's absolute addresses instead of taking a
 * pointer -- 0x00719cf4 is exactly network_bandwidth_graph_globals + 0x14, and the final
 * snprintf destination 0x0071c0c0 is exactly network_bandwidth_graph_globals + 0x23e0, the same
 *
 * @address 0x4d7ad0
 */
void BandwidthMonitor::update_()
{
    network_bandwidth_graph *graph = &network_bandwidth_graph_globals;
    uint8_t *base = (uint8_t *)graph;

    if (network_bandwidth_overlay_enabled != 0) {
        int32_t width = (int32_t)game_window_bottom_right.x - (int32_t)game_window_top_left.x;
        int32_t height = (int32_t)game_window_bottom_right.y - (int32_t)game_window_top_left.y;

        if (*(int32_t *)(base + 0x14) != width || *(int32_t *)(base + 0x18) != height) {
            float x_scale = (float)width * 0.2f;
            float y_scale = (float)height * 0.4f;
            float right_raw = (float)(game_window_bottom_right.y - 0x40);
            float right_minus_yscale = right_raw - y_scale;
            float baseline_raw = (float)(game_window_bottom_right.x - 0x40);
            float baseline_minus_xscale = baseline_raw - x_scale;
            float box_x1, box_y1, box_x0, box_y0;
            int16_t field24_v;
            float r1, r2;
            int32_t i;

            *(float *)(base + 0x1c) = x_scale;
            *(float *)(base + 0x20) = y_scale;
            *(int32_t *)(base + 0x14) = width;
            *(int32_t *)(base + 0x18) = height;

            graph->left = (int16_t)right_minus_yscale;
            field24_v = (int16_t)baseline_minus_xscale;
            *(int16_t *)(base + 0x24) = field24_v;
            graph->right = (int16_t)right_raw;
            graph->baseline = (int16_t)baseline_raw;

            for (i = 0; i < 0x780; i++) {
                ((int32_t *)(base + 0x5d8))[i] = 0;
            }
            for (i = 0; i < 0x1e; i++) {
                ((float *)(base + 0x44))[i] = 0.0f;
            }

            network_bandwidth_graph_instance_history_reset(graph);

            box_x0 = right_minus_yscale - 1.0f;
            *(uint32_t *)(base + 0x50) = 0xffffff00;
            *(uint32_t *)(base + 0x68) = 0xffffff00;
            *(uint32_t *)(base + 0x80) = 0xffffff00;
            *(uint32_t *)(base + 0x98) = 0xffffff00;
            *(float *)(base + 0x44) = box_x0;
            box_y0 = baseline_minus_xscale - 1.0f;
            *(uint32_t *)(base + 0xb0) = 0xffffff00;
            *(float *)(base + 0x48) = box_y0;
            box_y1 = right_raw + 1.0f;
            box_x1 = baseline_raw + 1.0f;
            *(float *)(base + 0x5c) = box_y1;
            *(float *)(base + 0x60) = box_y0;
            *(float *)(base + 0x74) = box_y1;
            *(float *)(base + 0x78) = box_x1;
            *(float *)(base + 0x8c) = box_x0;
            *(float *)(base + 0x90) = box_x1;
            *(float *)(base + 0xa4) = box_x0;
            *(float *)(base + 0xa8) = box_y0;

            r1 = 640.0f / (float)height;
            r2 = 480.0f / (float)width;
            {
                int16_t v2c = (int16_t)((float)field24_v * r2);
                int16_t v3x = (int16_t)((float)graph->left * r1);

                *(int16_t *)(base + 0x2e) = v3x;
                *(int16_t *)(base + 0x2c) = v2c;
                *(int16_t *)(base + 0x32) = 0x280;
                *(int16_t *)(base + 0x30) = 0x1e0;
                *(int16_t *)(base + 0x34) = v2c;

                *(int16_t *)(base + 0x36) = (int16_t)((float)graph->right * r1);
                *(int16_t *)(base + 0x3a) = 0x280;
                *(int16_t *)(base + 0x38) = 0x1e0;
                *(int16_t *)(base + 0x3e) = *(int16_t *)(base + 0x36);

                *(int16_t *)(base + 0x3c) = (int16_t)(((x_scale * 0.5f) + (float)field24_v) * r2);
                *(int16_t *)(base + 0x42) = 0x280;
                *(int16_t *)(base + 0x40) = 0x1e0;
            }

            _snprintf((char *)(base + 0x23e0), 0x200, "%s %s",
                network_bandwidth_units_label_table[graph->units_index],
                network_bandwidth_direction_label_table[graph->direction_index]);
        }
        network_stats_overlay_draw(graph);
    }
}

/**
 * already named)
 * address 0x4d8a20, size 44 bytes
 * name confidence: 0.55   rewrite confidence: 0.75
 * out/phase4/networking_functions.md summary ("Converts a 'bytes'/'packets' unit-name
 * string into the corresponding index for the network bandwidth debug graph"); the two strings
 * referenced ("bytes", "packets") match network_bandwidth_units_label_table, already named from
 * src/networking/network_bandwidth_graph_instance_update_layout.c and
 *
 * @address 0x4d8a20
 */
int32_t BandwidthMonitor::unit_name_to_index(const char *name)
{
    int32_t i;

    i = 0;
    do {
        if (_stricmp(name, network_bandwidth_units_label_table[i]) == 0) {
            return i;
        }
        i = i + 1;
    } while (i < 2);
    return -1;
}

/**
 * out/phase2/results/networking_01.json / symbols/review_queue.txt)
 * address 0x4d8140, size 114 bytes
 * name confidence: 0.4   rewrite confidence: 0.75
 * out/phase4/networking_functions.md summary ("Finds the index of the largest
 * sample in the bandwidth-graph history, used to rescale the graph's vertical axis");
 * out/phase2/results/networking_01.json evidence ("scans the graph's sample array in groups
 * of 5 across its full 0x140-entry length tracking the running maximum value and its index,
 *
 * @address 0x4d8140
 */
int32_t BandwidthGraphView::find_peak_sample(int32_t *out_peak_countdown)
{
    network_bandwidth_graph *graph = self;
    int32_t peak_value = 1;
    int32_t peak_index = 319;
    int32_t i;

    for (i = 0; i < 320; i++) {
        if (peak_value <= graph->history[i]) {
            peak_value = graph->history[i];
            peak_index = i;
        }
    }

    if (out_peak_countdown != 0) {

        *out_peak_countdown = peak_index + 1;
    }
    return peak_value;
}

/**
 * out/phase4/networking_functions.md summary ("Resets a network bandwidth-graph
 * instance's sample ring buffer and history array to an empty starting state"); every field
 * this function touches matches types/networking.h network_bandwidth_graph exactly
 * (columns[].color/x/y, bits_sent/bits_received/rate_base_ms/rate_sent/rate_received/
 * pending_sample/unknown_00d4, history[320], peak_scale, displayed_rate, last_sample_ms).
 *
 * @address 0x4d8080
 */
void BandwidthGraphView::instance_history_reset()
{
    network_bandwidth_graph *graph = self;
    int16_t baseline = graph->baseline;
    int16_t right = graph->right;
    int16_t left = graph->left;
    int32_t accumulator = 0;
    int32_t i;

    for (i = 0; i < 320; i++) {
        int32_t x_step = accumulator / 320;

        graph->columns[i].color = 0xffffffff;
        accumulator = accumulator + ((int32_t)right - (int32_t)left);
        graph->columns[i].x = (float)(x_step + left);
        graph->columns[i].y = (float)(int32_t)baseline;
    }

    graph->pending_sample = 0;
    for (i = 0; i < 320; i++) {
        graph->history[i] = 0;
    }
    graph->peak_scale = 1;
    graph->displayed_rate = 0.0f;
    graph->last_sample_ms = 0;
    graph->bits_received = 0;
    graph->bits_sent = 0;
    graph->rate_received = 0.0f;
    graph->rate_sent = 0.0f;
    graph->rate_base_ms = 0;
    graph->needs_layout = 1;
}

/**
 * already named)
 * address 0x4d7de0, size 58 bytes
 * name confidence: 0.5   rewrite confidence: 0.75
 * out/phase4/networking_functions.md summary ("Initializes a per-instance network
 * bandwidth-graph object with the given units/direction mode and forces its layout to be
 * recomputed"); types/networking.h network_bandwidth_graph.needs_layout (+0x0),
 * .last_sample_ms (+0x4), .sample_interval_ms (+0x8), .units_index (+0xc),
 *
 * @address 0x4d7de0
 */
void BandwidthGraphView::instance_init(int32_t units_index, int32_t direction_index)
{
    network_bandwidth_graph *graph = self;
    graph->needs_layout = 1;
    graph->last_sample_ms = 0;
    graph->sample_interval_ms = network_bandwidth_graph_default_interval_ms;
    graph->units_index = units_index;
    graph->direction_index = direction_index;
    graph->bits_sent = 0;
    graph->bits_received = 0;
    graph->rate_base_ms = 0;
    network_bandwidth_graph_instance_update_layout(graph, 1);
}

/**
 * out/phase4/networking_functions.md summary ("Recomputes the on-screen layout and
 * label text of one network bandwidth-graph instance object when the screen size changes or a
 * refresh is forced"); types/networking.h network_bandwidth_graph for the named fields
 * (width/height at +0x14/+0x18 fall inside its documented unknown_0014[0x12], the label/layout
 * scratch touched here falls inside its documented unknown_002c[0x90]); this function's own
 * summary's naming hints ("update_for_resolution_change") and symbols/functions.txt
 * rasterizer_resize_game_window for the 0x0069c634/0x0069c638 screen client-area corners.
 *
 * @address 0x4d7e20
 */
void BandwidthGraphView::instance_update_layout(uint8_t force_refresh)
{
    network_bandwidth_graph *graph = self;
    uint8_t *base = (uint8_t *)graph;
    int32_t width = (int32_t)game_window_bottom_right.x - (int32_t)game_window_top_left.x;
    int32_t height = (int32_t)game_window_bottom_right.y - (int32_t)game_window_top_left.y;

    if (*(int32_t *)(base + 0x14) != width || *(int32_t *)(base + 0x18) != height || force_refresh != 0) {
        float x_scale = (float)width * 0.2f;
        float y_scale = (float)height * 0.4f;
        float right_raw = (float)(game_window_bottom_right.y - 0x40);
        float baseline_raw = (float)(game_window_bottom_right.x - 0x40);
        float box_y1, box_x1, box_x0, box_y0;
        int16_t field24_v;
        float r1, r2;
        int32_t i;

        *(int32_t *)(base + 0x18) = height;
        *(int32_t *)(base + 0x14) = width;
        *(float *)(base + 0x1c) = x_scale;
        *(float *)(base + 0x20) = y_scale;

        graph->left = (int16_t)(right_raw - y_scale);
        field24_v = (int16_t)(baseline_raw - x_scale);
        *(int16_t *)(base + 0x24) = field24_v;
        graph->right = (int16_t)right_raw;
        graph->baseline = (int16_t)baseline_raw;

        for (i = 0; i < 0x780; i++) {
            ((int32_t *)(base + 0x5d8))[i] = 0;
        }

        for (i = 0; i < 0x1e; i++) {
            ((float *)(base + 0x44))[i] = 0.0f;
        }

        network_bandwidth_graph_instance_history_reset(graph);

        box_x0 = (right_raw - y_scale) - 1.0f;
        *(uint32_t *)(base + 0x50) = 0xffffff00;
        *(uint32_t *)(base + 0x68) = 0xffffff00;
        *(float *)(base + 0x44) = box_x0;
        *(uint32_t *)(base + 0x80) = 0xffffff00;
        *(uint32_t *)(base + 0x98) = 0xffffff00;
        box_y0 = (baseline_raw - x_scale) - 1.0f;
        *(uint32_t *)(base + 0xb0) = 0xffffff00;
        *(float *)(base + 0x48) = box_y0;
        box_y1 = right_raw + 1.0f;
        *(float *)(base + 0x5c) = box_y1;
        *(float *)(base + 0x60) = box_y0;
        *(float *)(base + 0x74) = box_y1;
        box_x1 = baseline_raw + 1.0f;
        *(float *)(base + 0x78) = box_x1;
        *(float *)(base + 0x8c) = box_x0;
        *(float *)(base + 0x90) = box_x1;
        *(float *)(base + 0xa4) = box_x0;
        *(float *)(base + 0xa8) = box_y0;

        r1 = 640.0f / (float)height;
        r2 = 480.0f / (float)width;
        {
            int16_t v2c = (int16_t)((float)field24_v * r2);
            int16_t v36 = (int16_t)((float)graph->right * r1);

            *(int16_t *)(base + 0x2e) = (int16_t)((float)graph->left * r1);
            *(int16_t *)(base + 0x2c) = v2c;
            *(int16_t *)(base + 0x32) = 0x280;
            *(int16_t *)(base + 0x30) = 0x1e0;

            *(int16_t *)(base + 0x36) = v36;
            *(int16_t *)(base + 0x34) = v2c;
            *(int16_t *)(base + 0x3a) = 0x280;
            *(int16_t *)(base + 0x38) = 0x1e0;
            *(int16_t *)(base + 0x3e) = v36;

            *(int16_t *)(base + 0x3c) = (int16_t)(((x_scale * 0.5f) + (float)field24_v) * r2);
            *(int16_t *)(base + 0x42) = 0x280;
            *(int16_t *)(base + 0x40) = 0x1e0;
        }

        _snprintf((char *)(base + 0x23e0), 0x200, "%s %s",
            network_bandwidth_units_label_table[graph->units_index],
            network_bandwidth_direction_label_table[graph->direction_index]);
    }
}

/**
 * out/phase2/results/networking_01.json / symbols/review_queue.txt)
 * address 0x4d8430, size 145 bytes
 * name confidence: 0.4   rewrite confidence: 0.6
 * out/phase4/networking_functions.md summary ("Records a new bandwidth sample into
 * a graph instance, updating its smoothed displayed rate and last-update timestamp");
 * out/phase2/results/networking_01.json evidence ("calls
 * network_bandwidth_graph_update_columns (0x4d81c0), averages the five most recent raw sample
 *
 * @address 0x4d8430
 */
void BandwidthGraphView::new_sample()
{
    network_bandwidth_graph *graph = self;
    large_integer counter;
    int32_t recent_sum;

    network_bandwidth_graph_update_columns(graph->pending_sample, graph);

    recent_sum = graph->history[319] + graph->history[318] + graph->history[317] +
                 graph->history[316] + graph->history[315];
    graph->displayed_rate = (float)recent_sum * 0.25f;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    graph->last_sample_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    graph->pending_sample = 0;
}

/**
 * out/phase2/results/networking_01.json / symbols/review_queue.txt)
 * address 0x4d84d0, size 100 bytes
 * name confidence: 0.45   rewrite confidence: 0.65
 * out/phase4/networking_functions.md summary ("Advances a network bandwidth-graph
 * instance forward by any elapsed sampling intervals since it was last updated");
 * out/phase2/results/networking_01.json evidence ("computes elapsed ms since the instance's
 * last update (+4) via QueryPerformanceCounter, and while the elapsed time exceeds the
 *
 * @address 0x4d84d0
 */
void BandwidthGraphView::tick()
{
    network_bandwidth_graph *graph = self;
    large_integer counter;
    uint32_t elapsed_ms;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    if (graph->needs_layout != 0) {
        return;
    }

    elapsed_ms = (uint32_t)((counter.quad_part * 1000) / performance_frequency) -
                 (uint32_t)graph->last_sample_ms;
    while (graph->sample_interval_ms <= elapsed_ms) {
        network_bandwidth_graph_new_sample(graph);
        elapsed_ms -= graph->sample_interval_ms;
    }
}

/**
 * out/phase2/results/networking_01.json / symbols/review_queue.txt)
 * address 0x4d81c0, size 605 bytes
 * name confidence: 0.35   rewrite confidence: 0.45
 * out/phase4/networking_functions.md summary ("Feeds a new sample into a network
 * bandwidth-graph instance and recomputes the interpolated column heights used to draw the
 * scrolling line graph"); out/phase2/results/networking_01.json evidence ("shifts the +0xd8
 * history window down by one and records the new sample (in_EAX) at +0x5d4; when the current
 *
 * @address 0x4d81c0
 */
void BandwidthGraphView::update_columns(int32_t new_sample)
{
    network_bandwidth_graph *graph = self;
    uint8_t *base = (uint8_t *)graph;
    float scale = *(float *)(base + 0x1c);
    int32_t old_peak_scale = graph->peak_scale;
    int32_t i;

    for (i = 0; i < 319; i++) {
        graph->history[i] = graph->history[i + 1];
    }
    graph->history[319] = new_sample;

    if (new_sample < graph->peak_scale) {
        graph->peak_samples_remaining -= 1;
        if (graph->peak_samples_remaining == 0) {
            graph->peak_scale = network_bandwidth_graph_find_peak_sample(&graph->peak_samples_remaining, graph);
        }
    } else {
        graph->peak_scale = new_sample;
        graph->peak_samples_remaining = 0x140;
    }

    if (old_peak_scale == graph->peak_scale) {

        for (i = 0; i < 319; i++) {
            graph->columns[i].y = graph->columns[i + 1].y;
        }
        graph->columns[319].y = (float)(int32_t)graph->baseline -
            ((float)graph->history[319] / (float)graph->peak_scale) * scale;
        return;
    }

    for (i = 0; i < 320; i++) {
        graph->columns[i].y = (float)(int32_t)graph->baseline -
            ((float)graph->history[i] / (float)graph->peak_scale) * scale;
    }
}

namespace {

static float as_unsigned_float(int32_t value)
{
    float result = (float)value;
    if (value < 0) {
        result = result + 4294967296.0f;
    }
    return result;
}

}

/**
 * out/phase4/networking_functions.md summary ("Computes the smoothed sent/received
 * bits-per-second rates for the network bandwidth debug overlay from accumulated byte counters
 * and elapsed time"); types/networking.h network_bandwidth_graph.rate_base_ms (+0xc4),
 * .bits_sent (+0xbc), .bits_received (+0xc0), .rate_sent (+0xc8), .rate_received (+0xcc); the
 * QueryPerformanceCounter/__allmul/__alldiv shape is the same one folded into plain int64_t
 * arithmetic in src/math/random_seed_generate.c and src/networking/network_update.c.
 *
 * @address 0x4d8540
 */
void BandwidthGraphView::rate_compute()
{
    network_bandwidth_graph *graph = self;
    large_integer counter;
    int32_t base_ms = graph->rate_base_ms;
    int32_t now_ms;
    float elapsed_seconds;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    elapsed_seconds = as_unsigned_float(now_ms - base_ms) * 0.001f;

    if (base_ms == 0) {
        graph->rate_sent = 0.0f;
        graph->rate_received = 0.0f;
        return;
    }

    if (elapsed_seconds < 1.0f) {
        elapsed_seconds = 1.0f / elapsed_seconds;
    }
    graph->rate_sent = as_unsigned_float(graph->bits_sent) * (1.0f / elapsed_seconds);
    graph->rate_received = as_unsigned_float(graph->bits_received) * (1.0f / elapsed_seconds);
}

namespace {

static void device_call1(void **device, uint32_t vtable_offset, int32_t arg1)
{
    (*(void (__stdcall **)(void *, int32_t))((uint8_t *)device + vtable_offset))(device, arg1);
}

static void device_call2(void **device, uint32_t vtable_offset, int32_t arg1, int32_t arg2)
{
    (*(void (__stdcall **)(void *, int32_t, int32_t))((uint8_t *)device + vtable_offset))(device, arg1, arg2);
}

static void device_set_render_state(void **device, int32_t state, int32_t value)
{
    device_call2(device, 0xe4, state, value);
}

static void device_set_sampler_state(void **device, int32_t sampler, int32_t type, int32_t value)
{
    (*(void (__stdcall **)(void *, int32_t, int32_t, int32_t))((uint8_t *)device + 0x10c))(device, sampler, type, value);
}

static void device_draw_primitive_up(void **device, int32_t primitive_type, int32_t primitive_count, const void *vertex_data, int32_t stride)
{
    (*(void (__stdcall **)(void *, int32_t, int32_t, const void *, int32_t))((uint8_t *)device + 0x14c))(
        device, primitive_type, primitive_count, vertex_data, stride);
}

static void device_call_mode_ptr_count(void **device, uint32_t vtable_offset, int32_t mode, const void *ptr, int32_t count)
{
    (*(void (__stdcall **)(void *, int32_t, const void *, int32_t))((uint8_t *)device + vtable_offset))(
        device, count, ptr, mode);
}

}

/**
 * out/phase4/networking_functions.md summary ("Renders the network bandwidth debug
 * overlay: draws the scrolling line graph via the Direct3D device and overlays the
 * sent/received bits-per-second text"); types/networking.h network_bandwidth_graph.columns
 * (+0x5d8), .peak_scale (+0x23d8), .displayed_rate (+0x23dc), .rate_sent (+0xc8),
 * .rate_received (+0xcc), the label text at +0x23e0 (see network_bandwidth_graph_instance_
 * update_layout, 0x4d7e20, already committed, for why that offset has no header field).
 * Ghidra's own decompile of every indirect call through the renderer's vtable
 *
 * @address 0x4d8620
 */
void BandwidthGraphView::overlay_draw()
{
    network_bandwidth_graph *graph = self;
    void **device = *rasterizer_device;
    float delta_y, delta_x;
    float label_quad[13] = { 0 };

    device_call1(device, 0x15c, (int32_t)renderer_unknown_6e1af0);

    {
        uint32_t flag = ((rasterizer_software_vertex_processing != 0) ? 0x10u : 0u) & 0x10u;
        flag = (flag | renderer_unknown_6e1af8) & 0x10u;
        device_call1(device, 0x134, (int32_t)flag);
    }

    device_call1(device, 0x170, (int32_t)renderer_unknown_69e468);

    delta_y = (float)(int16_t)(network_stats_overlay_text_rect_max.y - network_stats_overlay_text_rect_min.y);
    delta_x = (float)(int16_t)(network_stats_overlay_text_rect_max.x - network_stats_overlay_text_rect_min.x);
    label_quad[3] = 2.0f * (1.0f / delta_y);
    label_quad[6] = -1.0f - (1.0f / delta_y);
    label_quad[8] = -2.0f * (1.0f / delta_x);
    label_quad[10] = (1.0f / delta_x) + 1.0f;
    device_call_mode_ptr_count(device, 0x178, 5, label_quad, 0xd);

    device_call1(device, 0x1ac, 0);
    rasterizer_set_shader_stage_config(0);

    device_set_render_state(device, 0x16, 1);
    device_set_render_state(device, 0xa8, 0xf);
    device_set_render_state(device, 0x1b, 0);
    device_set_render_state(device, 0xf, 0);
    device_set_render_state(device, 0x7, 0);
    device_set_render_state(device, 0xe, 0);
    device_set_render_state(device, 0x1c, 0);

    device_set_sampler_state(device, 0, 1, 3);
    device_set_sampler_state(device, 0, 3, 0);
    device_set_sampler_state(device, 0, 4, 3);
    device_set_sampler_state(device, 0, 6, 0);
    device_set_sampler_state(device, 1, 1, 1);
    device_set_sampler_state(device, 1, 4, 1);

    device_call2(device, 0x104, 0, 0);

    device_set_render_state(device, 0x9c, 1);
    device_set_render_state(device, 0x9d, 0);

    device_draw_primitive_up(device, 3, 0x13f, graph->columns, sizeof(network_graph_vertex));

    device_draw_primitive_up(device, 3, 4, (uint8_t *)graph + 0x44, sizeof(network_graph_vertex));

    hud_text_draw_configure(1, -1, 0, 0, 5, 0);
    hud_text_draw_color_a = 1.0f;
    hud_text_draw_color_r = 1.0f;
    hud_text_draw_color_g = 1.0f;
    hud_text_draw_color_b = 1.0f;
    hud_text_draw_background_mode = 0;

    {
        char text[128];

        sprintf(text, "%s|n%.2f bps sent|n%.2f bps recv", (char *)graph + 0x23e0,
            (double)graph->rate_sent, (double)graph->rate_received);
        chimera__draw_8_bit_text(0, 0, text);

        sprintf(text, decimal_format_string, graph->peak_scale);
        chimera__draw_8_bit_text(0, 0, text);

        sprintf(text, decimal_format_string, (int32_t)graph->displayed_rate);
        chimera__draw_8_bit_text(0, 0, text);
    }

    device_call1(device, 0x134, (int32_t)rasterizer_software_vertex_processing);
}

}
