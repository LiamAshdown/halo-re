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
 * Appends one sized packet block to the channel's outgoing stream as an item with a clear item flag, flushing the
 * stream first when fewer bits are free than the item needs. Does nothing (and succeeds) while channel flag bit 0
 * is set; returns false only when the stream was full and the flush failed.
 */
inline bool channel_queue_packet(network_channel *channel, const uint16_t *packet)
{
    const int32_t bit_count = packet_block_bit_count(packet);
    const int32_t total_bits = bit_count + 1;
    network_channel_stream &out = channel->outgoing;

    if ((channel->flags & 1) != 0) {
        return true;
    }
    if (static_cast<int32_t>(out.stream.last_bit - out.stream.byte_cursor * 8 - out.stream.bit_cursor) + 1 < total_bits &&
        network_channel_stream_flush(&out, channel, 1) == 0) {
        return false;
    }
    channel->send_budget = channel->send_budget + total_bits;
    uint32_t item_flag = 0;
    memory::bit_stream_write_bits_chunked(&out.stream, &item_flag, 1);
    out.empty = 0;
    memory::bit_stream_write_bits_chunked(&out.stream, reinterpret_cast<const uint32_t *>(packet), bit_count);
    out.empty = 0;
    return true;
}

}
