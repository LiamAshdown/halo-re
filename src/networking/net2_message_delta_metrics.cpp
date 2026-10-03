/**
 * @file src/networking/net2_message_delta_metrics.cpp
 * Message-delta size sampling and metrics.
 */
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "halo/networking/net2_message_delta_metrics.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/networking/vars.hpp"

static auto &message_delta_metrics_filename_suffix = halo::link::ref<char []>(halo::networking::vars().message_delta_metrics_filename_suffix);
extern "C" {
extern int32_t snprintf(char *dest, uint32_t count, const char *format, ...);
extern void console_printf_verbose(const char *format, ...);
}


namespace halo::networking {

void DeltaMetrics::metrics_dump(char *suffix)
{
    char path[260];

    if (suffix != 0) {
        int32_t len;
        for (len = 0; suffix[len] != 0; len++) {
        }
        (void)len;
    }
    snprintf(path, 0x104, "%s\\%s %s", "message metrics", message_delta_metrics_filename_suffix, suffix);
    halo::interface::console_printf_verbose((ColorARGB *)0, (char *)("Wrote network message metrics to %s"), path);
}

void DeltaMetrics::sample_record_and_append(int32_t a, int32_t c, int32_t b,
                                             message_delta_sample_ring_buffer *ring)
{
    int32_t record[5];
    uint32_t half_span;

    half_span = ((uint32_t)c - (uint32_t)a) >> 1;
    record[0] = a;
    record[1] = b;
    record[2] = c;
    record[3] = (int32_t)half_span;
    record[4] = (int32_t)half_span - c + b;
    halo::networking::message_delta_sample_ring_buffer_append(ring, record);
}

void DeltaMetrics::sample_ring_buffer_append(message_delta_sample_ring_buffer *ring, const int32_t *entry)
{
    int32_t slot;
    int32_t count;
    int64_t sum;
    int32_t i;

    if (ring->count < 0x1e) {
        slot = ring->count;
        ring->entries[slot][0] = entry[0];
        ring->entries[slot][1] = entry[1];
        ring->entries[slot][2] = entry[2];
        ring->entries[slot][3] = entry[3];
        ring->entries[slot][4] = entry[4];
        ring->count = ring->count + 1;
        ring->write_cursor = 0;
    } else {
        slot = ring->write_cursor;
        ring->entries[slot][0] = entry[0];
        ring->entries[slot][1] = entry[1];
        ring->entries[slot][2] = entry[2];
        ring->entries[slot][3] = entry[3];
        ring->entries[slot][4] = entry[4];
        ring->write_cursor = ring->write_cursor + 1;
        if (ring->write_cursor == 0x1e) {
            ring->write_cursor = 0;
        }
    }

    count = ring->count;
    sum = 0;
    if (0 < count) {
        for (i = 0; i < count; i++) {
            sum = sum + (int64_t)ring->entries[i][4];
        }
    }
    if (count != 0) {
        ring->cached_average = (int32_t)(sum / (int64_t)count);
    } else {
        ring->cached_average = 0;
    }
}

int32_t DeltaMetrics::sample_ring_buffer_average(message_delta_sample_ring_buffer *ring)
{
    int32_t count;
    uint64_t sum;
    int32_t i;

    count = ring->count;
    sum = 0;
    if (0 < count) {
        for (i = 0; i < count; i++) {
            sum = sum + (uint32_t)ring->entries[i][3];
        }
    }
    if (count == 0) {
        return 0;
    }
    return (int32_t)((int64_t)sum / count);
}

}  // namespace halo::networking

namespace halo::networking {
void message_delta_metrics_dump(char *suffix)
{
    halo::networking::DeltaMetrics::metrics_dump(suffix);
}

void message_delta_sample_record_and_append(int32_t a, int32_t c, int32_t b,
                                             message_delta_sample_ring_buffer *ring)
{
    halo::networking::DeltaMetrics::sample_record_and_append(a, c, b, ring);
}

void message_delta_sample_ring_buffer_append(message_delta_sample_ring_buffer *ring, const int32_t *entry)
{
    halo::networking::DeltaMetrics::sample_ring_buffer_append(ring, entry);
}

int32_t message_delta_sample_ring_buffer_average(message_delta_sample_ring_buffer *ring)
{
    return halo::networking::DeltaMetrics::sample_ring_buffer_average(ring);
}

}
