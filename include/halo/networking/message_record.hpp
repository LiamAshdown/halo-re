#pragma once

#include <cstddef>
#include <cstdint>

/**
 * A received network-game message record: a 16-bit header word (low four bits flag the bit-stream kind, the rest
 * is the length in bits) followed by the encoded message body that data_packet_group_decode_packet consumes.
 */
struct network_message_record {
    uint16_t header;
    uint8_t body[2];
};

static_assert(offsetof(network_message_record, body) == 2);
