#pragma once

#include "halo/memory/api.hpp"
#include "halo/networking/api.hpp"

namespace halo::networking {

/** The bit length of a sized packet block: the 12-bit size field in its first word counts bytes in its upper bits. */
inline int32_t packet_block_bit_count(const uint16_t *packet) noexcept
{
    return static_cast<int32_t>(*packet >> 4) * 8;
}

/**
 * Appends bit_count bits to the channel's outgoing stream as one item preceded by item_flag (0 for a packet block,
 * 1 for a delta-coded message), counting them against the send budget. Flushes the stream first when fewer bits
 * are free than the item needs; returns false only when that flush fails and nothing was staged.
 */
inline bool channel_queue_bits(network_channel *channel, const uint32_t *bits, int32_t bit_count, uint32_t item_flag)
{
    const int32_t total_bits = bit_count + 1;
    network_channel_stream &out = channel->outgoing;

    if (static_cast<int32_t>(out.stream.last_bit - out.stream.byte_cursor * 8 - out.stream.bit_cursor) + 1 < total_bits &&
        network_channel_stream_flush(&out, channel, 1) == 0) {
        return false;
    }
    channel->send_budget = channel->send_budget + total_bits;
    memory::bit_stream_write_bits_chunked(&out.stream, &item_flag, 1);
    out.empty = 0;
    memory::bit_stream_write_bits_chunked(&out.stream, bits, bit_count);
    out.empty = 0;
    return true;
}

/**
 * Appends one sized packet block to the channel's outgoing stream as an item with a clear item flag. Does nothing
 * (and succeeds) while channel flag bit 0 is set; returns false only when the stream was full and could not be
 * flushed.
 */
inline bool channel_queue_packet(network_channel *channel, const uint16_t *packet)
{
    if ((channel->flags & 1) != 0) {
        return true;
    }
    return channel_queue_bits(channel, reinterpret_cast<const uint32_t *>(packet), packet_block_bit_count(packet), 0);
}

}
