/**
 * @file include/halo/networking/net2_message_delta_metrics.hpp
 * Message-delta size sampling and metrics.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

namespace halo::networking {

/**
 * Message-delta size sampling and metrics.
 *
 * Static behaviour facade; the original C entry points forward to these members.
 */
class DeltaMetrics {
public:
    /**
     * Builds the output path for a network-message-metrics dump ("message metrics\<suffix> <name>") and logs a confirmation that it was written. No caller of this function exists anywhere in the 454-function networking module, so the actual dump/write step this path would feed is not present here.
     *
     * @address 0x4ec3d0
     */
    static void metrics_dump(char *suffix);

    /**
     * Builds a 5-field sample record out of three inputs and appends it to the ring buffer.
     *
     * @address 0x4ed310
     */
    static void sample_record_and_append(int32_t a, int32_t c, int32_t b,
                                             message_delta_sample_ring_buffer *ring);

    /**
     * Appends a new 20-byte sample entry: while the buffer has not yet reached 30 entries it grows in place, otherwise it overwrites the slot at the wrapping write_cursor. Either way, recomputes the buffer's cached running average (of entry field index 4) over every entry currently held.
     *
     * @address 0x4ed390
     */
    static void sample_ring_buffer_append(message_delta_sample_ring_buffer *ring, const int32_t *entry);

    /**
     * Averages entry field index 3 (offset 0xc) of the first `count` entries, using a 64-bit accumulator (summed as raw 32-bit patterns with carry, matching the original's unsigned add-with-carry then signed-divide) so up to 30 samples cannot lose precision. Returns 0 if the buffer is empty.
     *
     * @address 0x4ed350
     */
    static int32_t sample_ring_buffer_average(message_delta_sample_ring_buffer *ring);

};

}  // namespace halo::networking
