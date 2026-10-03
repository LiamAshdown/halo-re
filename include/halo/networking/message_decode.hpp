#pragma once

#include "halo/core/link.hpp"
#include "halo/memory/api.hpp"
#include "halo/networking/vars.hpp"

namespace halo::networking {

/** A game message on the wire: a 2-byte header followed by a group-encoded body. */
struct network_message_header {
    uint8_t bytes[2];
};
static_assert(sizeof(network_message_header) == 2, "game message header");

/**
 * Decodes the group-encoded body of a game message of the given class into decoded_body, discarding the type and
 * version out-values. message_length counts the header; returns nonzero when the body decoded.
 */
inline int32_t decode_message_body(const uint8_t *body, int32_t message_length, void *decoded_body, int16_t expected_class)
{
    int16_t body_length = static_cast<int16_t>(message_length - static_cast<int32_t>(sizeof(network_message_header)));
    int16_t out_type;
    uint16_t out_version;

    return memory::data_packet_group_decode_packet(&body_length, &link::ref<data_packet_group>(vars().network_game_messages_group),
                                                   decoded_body, const_cast<uint8_t *>(body), &out_type, &out_version, expected_class);
}

}
