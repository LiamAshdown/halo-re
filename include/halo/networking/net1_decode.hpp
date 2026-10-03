#pragma once

#include "halo/networking/net1_common.hpp"

namespace halo::networking {

/**
 * Behaviour group for the original `network_*` functions; every member is the original function body moved unchanged.
 */
class GameClientView {
public:
    network_client_globals *self;

    explicit constexpr GameClientView(network_client_globals *record) : self(record) {}

    void action_apply(void **context);
    char action_queue_drain(bit_stream *stream, const uint32_t *sender);
    int32_t process_incoming_messages();
    int32_t settings_packet_receive(const uint32_t *request);
    void settings_packet_send(const uint8_t *request);
    int32_t state_update_receive(uint8_t *record);
    char incoming_item_dispatch(uint32_t item_flag, bit_stream *stream, const uint32_t *sender);
};

/**
 * Behaviour group for the original `network_game_*` functions; every member is the original function body moved unchanged.
 */
class ClientMessageDecoder {
public:
    network_client_globals *self;

    explicit constexpr ClientMessageDecoder(network_client_globals *record) : self(record) {}

    int32_t and_discard_ingame_message(const uint8_t *buffer, int32_t length, const uint32_t *sender_address);
    int32_t and_discard_join_message(const uint8_t *buffer, int32_t length, const uint32_t *sender_address);
    int32_t beacon_reply(const uint8_t *buffer, int32_t length);
    int32_t connect_rejected(const uint8_t *buffer, int32_t length, const uint32_t *expected_sequence);
    int32_t join_accepted(const uint8_t *buffer, int32_t length, const uint32_t *sender_address);
    int32_t join_complete(const uint8_t *buffer, int32_t length, const uint32_t *sender_address);
    int32_t join_finalize_ack(const uint8_t *buffer, int32_t length, const uint32_t *sender_address);
    int32_t join_finalize_message(const uint8_t *buffer, int32_t length, const uint32_t *sender_address);
    int32_t player_config_value(const uint8_t *buffer, int32_t length, const uint32_t *sender_address);
    char player_join_chunk(uint8_t *param_1, int32_t param_2, int32_t *param_3);
    char player_slot_chunk(uint8_t *param_1, int32_t param_2, int32_t *param_3);
    int32_t pong_reply(const uint8_t *buffer, int32_t length, const uint32_t *sender_address);
    int32_t settings_or_ack(const uint8_t *buffer, int32_t length, const int32_t *expected_sequence);
    char state_update_chunk(uint8_t *param_1, int32_t param_2, int32_t *param_3);
    int32_t sync_complete(const uint8_t *buffer, int32_t length, const uint32_t *sender_address);
    int32_t decode_settings_request(const uint8_t *buffer, int32_t length, const int32_t *expected_sequence);
    char dispatch(uint16_t *record, int32_t record_length, const uint32_t *sender);
    int32_t ingame_notification(const uint8_t *buffer, int32_t length, const uint32_t *sender_address);
    int32_t replicated_command(uint8_t *param_1, int32_t param_2, int32_t *param_3);
};

}
