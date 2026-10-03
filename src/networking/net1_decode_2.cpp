#include "halo/networking/net1_decode.hpp"
#include "halo/memory/api.hpp"

extern "C" {
extern void network_channel_remote_address_or_default(network_channel *channel, network_resolved_address *out_address);
extern data_packet_group network_game_messages_group;
extern void network_session_disconnect_with_error(int16_t error_code);
extern int16_t network_game_mode;
extern char network_game_settings_ack_send(uint8_t *client, int16_t template_row);
extern int32_t network_game_settings_packet_receive(network_client_globals *client, const uint32_t *request);
extern uint32_t message_delta_vector3d_mode;
extern void network_game_settings_packet_send(network_client_globals *client, const uint8_t *request);
}

namespace halo::networking {

/**
 * out/phase4/networking_functions.md summary ("Decodes a connect-rejected / error
 * notification from the server and triggers the client disconnect path with the given error
 * code"). Same guard/decode shape as the rest of this handler cluster.
 *
 * @address 0x4dbd40
 */
int32_t ClientMessageDecoder::connect_rejected(const uint8_t *buffer, int32_t length, const uint32_t *expected_sequence)
{
    network_client_globals *client = self;
    network_resolved_address sender;
    uint32_t decoded_body[2];
    int16_t out_a;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *expected_sequence && client->state != 0 && client->state != 4) {
        uint16_t version_used;
        if (halo::memory::data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group,
                                             decoded_body, (uint8_t *)buffer + 2, &out_a, &version_used, 2) != 0) {
            network_session_disconnect_with_error((int16_t)decoded_body[0]);
        }
    }
    return 0;
}

/**
 * out/phase4/networking_functions.md summary ("Decodes an incoming game-settings/
 * map-info message during the join handshake and applies it, sending a one-time acknowledgement
 * when acting purely as a client"). Forwards to network_game_settings_packet_receive.c
 * (0x4d9800) and network_game_settings_ack_send.c (0x4d9f50), both this task's batch.
 *
 * @address 0x4dbe50
 */
int32_t ClientMessageDecoder::settings_or_ack(const uint8_t *buffer, int32_t length, const int32_t *expected_sequence)
{
    network_client_globals *client = self;
    network_resolved_address sender;
    uint8_t decoded_body[944];
    int16_t out_a, out_b;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *expected_sequence) {
        if (network_game_mode == 2) {
            if (client->state == 2 && *(uint8_t *)&client->pad_ee2 == 0) {
                network_game_settings_ack_send((uint8_t *)client, 0);
                *(uint8_t *)&client->pad_ee2 = 1;
            }
        } else if (client->state == 2 || client->state == 3) {
            if (halo::memory::data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group,
                                                 decoded_body, (uint8_t *)buffer + 2, &out_a, (uint16_t *)&out_b, 2) != 0) {
                return network_game_settings_packet_receive(client, (const uint32_t *)decoded_body);
            }
            return 0;
        }
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md summary ("Decodes an early-handshake server/map
 * identification message and compares it against locally cached data, advancing the loading UI
 * state on a mismatch"); forwards the decoded record straight into
 * network_game_settings_packet_send.c (0x4d94c0, same task batch), which builds and sends the
 * full settings/map-data reply -- consistent with this being the *host's* handler for a
 * newly-joined client's settings request. Follows the same guard/decode shape as
 * network_game_client_decode_state_update_chunk.c (0x4dc190, same address family).
 *
 * @address 0x4dbc00
 */
int32_t ClientMessageDecoder::decode_settings_request(const uint8_t *buffer, int32_t length, const int32_t *expected_sequence)
{
    network_client_globals *client = self;
    network_resolved_address sender;
    uint8_t decoded_body[0x94];
    int16_t out_a, out_b;

    network_channel_remote_address_or_default(client->channel, &sender);
    if (sender.address.ipv4 == *expected_sequence && client->state == 1) {
        if (halo::memory::data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group,
                                             decoded_body, (uint8_t *)buffer + 2, &out_a, (uint16_t *)&out_b, 2) != 0) {
            message_delta_vector3d_mode = (decoded_body[8] == 1);
            network_game_settings_packet_send(client, decoded_body);
            return 1;
        }
    }
    return 0;
}

}
