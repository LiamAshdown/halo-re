#include "halo/networking/net1_dispatch.hpp"
#include "halo/networking/net1_client.hpp"
#include "halo/networking/net1_decode.hpp"
#include "halo/networking/net1_server.hpp"
#include "halo/networking/net1_session.hpp"

extern "C" {
extern data_packet_group network_game_messages_group;
extern int32_t data_packet_group_decode_packet(int16_t *remaining_length, data_packet_group *group, void *decoded_body, const uint8_t *buffer, int16_t *out_type, uint16_t *out_version_used, int16_t expected_class);
extern uint8_t network_disconnect_timeout_flag;
}

namespace halo::networking {

namespace {

using ClientDecodeFn = int32_t (*)(ClientMessageDecoder decoder, uint16_t *record, int32_t record_length, const uint32_t *sender);

class ClientFunctionHandler final : public ClientMessageHandler {
public:
    ClientDecodeFn decode;

    explicit constexpr ClientFunctionHandler(ClientDecodeFn fn) : decode(fn) {}

    int32_t handle(network_client_globals *client, uint16_t *record, int32_t record_length, const uint32_t *sender) const override
    {
        return decode(ClientMessageDecoder(client), record, record_length, sender);
    }
};

int32_t decode_beacon_reply(ClientMessageDecoder d, uint16_t *r, int32_t n, const uint32_t *)
{
    return d.beacon_reply((const uint8_t *)r, n);
}

int32_t decode_pong_reply(ClientMessageDecoder d, uint16_t *r, int32_t n, const uint32_t *s)
{
    return d.pong_reply((const uint8_t *)r, n, s);
}

int32_t decode_settings_request(ClientMessageDecoder d, uint16_t *r, int32_t n, const uint32_t *s)
{
    return d.decode_settings_request((const uint8_t *)r, n, (const int32_t *)s);
}

int32_t decode_join_accepted(ClientMessageDecoder d, uint16_t *r, int32_t n, const uint32_t *s)
{
    return d.join_accepted((const uint8_t *)r, n, s);
}

int32_t decode_connect_rejected(ClientMessageDecoder d, uint16_t *r, int32_t n, const uint32_t *s)
{
    return d.connect_rejected((const uint8_t *)r, n, s);
}

int32_t decode_join_complete(ClientMessageDecoder d, uint16_t *r, int32_t n, const uint32_t *s)
{
    return d.join_complete((const uint8_t *)r, n, s);
}

int32_t decode_settings_or_ack(ClientMessageDecoder d, uint16_t *r, int32_t n, const uint32_t *s)
{
    return d.settings_or_ack((const uint8_t *)r, n, (const int32_t *)s);
}

int32_t decode_player_config_value(ClientMessageDecoder d, uint16_t *r, int32_t n, const uint32_t *s)
{
    return d.player_config_value((const uint8_t *)r, n, s);
}

int32_t decode_join_finalize_message(ClientMessageDecoder d, uint16_t *r, int32_t n, const uint32_t *s)
{
    return d.join_finalize_message((const uint8_t *)r, n, s);
}

int32_t decode_join_finalize_ack(ClientMessageDecoder d, uint16_t *r, int32_t n, const uint32_t *s)
{
    return d.join_finalize_ack((const uint8_t *)r, n, s);
}

int32_t decode_and_discard_join_message(ClientMessageDecoder d, uint16_t *r, int32_t n, const uint32_t *s)
{
    return d.and_discard_join_message((const uint8_t *)r, n, s);
}

int32_t decode_and_discard_ingame_message(ClientMessageDecoder d, uint16_t *r, int32_t n, const uint32_t *s)
{
    return d.and_discard_ingame_message((const uint8_t *)r, n, s);
}

int32_t decode_state_update_chunk(ClientMessageDecoder d, uint16_t *r, int32_t n, const uint32_t *s)
{
    return d.state_update_chunk((uint8_t *)r, n, (int32_t *)s);
}

int32_t decode_player_join_chunk(ClientMessageDecoder d, uint16_t *r, int32_t n, const uint32_t *s)
{
    return d.player_join_chunk((uint8_t *)r, n, (int32_t *)s);
}

int32_t decode_player_slot_chunk(ClientMessageDecoder d, uint16_t *r, int32_t n, const uint32_t *s)
{
    return d.player_slot_chunk((uint8_t *)r, n, (int32_t *)s);
}

int32_t decode_sync_complete(ClientMessageDecoder d, uint16_t *r, int32_t n, const uint32_t *s)
{
    return d.sync_complete((const uint8_t *)r, n, s);
}

int32_t decode_replicated_command(ClientMessageDecoder d, uint16_t *r, int32_t n, const uint32_t *s)
{
    return d.replicated_command((uint8_t *)r, n, (int32_t *)s);
}

int32_t decode_ingame_notification(ClientMessageDecoder d, uint16_t *r, int32_t n, const uint32_t *s)
{
    return d.ingame_notification((const uint8_t *)r, n, s);
}

constexpr ClientFunctionHandler k_beacon_reply(decode_beacon_reply);
constexpr ClientFunctionHandler k_pong_reply(decode_pong_reply);
constexpr ClientFunctionHandler k_settings_request(decode_settings_request);
constexpr ClientFunctionHandler k_join_accepted(decode_join_accepted);
constexpr ClientFunctionHandler k_connect_rejected(decode_connect_rejected);
constexpr ClientFunctionHandler k_join_complete(decode_join_complete);
constexpr ClientFunctionHandler k_settings_or_ack(decode_settings_or_ack);
constexpr ClientFunctionHandler k_player_config_value(decode_player_config_value);
constexpr ClientFunctionHandler k_join_finalize_message(decode_join_finalize_message);
constexpr ClientFunctionHandler k_join_finalize_ack(decode_join_finalize_ack);
constexpr ClientFunctionHandler k_discard_join_message(decode_and_discard_join_message);
constexpr ClientFunctionHandler k_discard_ingame_message(decode_and_discard_ingame_message);
constexpr ClientFunctionHandler k_state_update_chunk(decode_state_update_chunk);
constexpr ClientFunctionHandler k_player_join_chunk(decode_player_join_chunk);
constexpr ClientFunctionHandler k_player_slot_chunk(decode_player_slot_chunk);
constexpr ClientFunctionHandler k_sync_complete(decode_sync_complete);
constexpr ClientFunctionHandler k_replicated_command(decode_replicated_command);
constexpr ClientFunctionHandler k_ingame_notification(decode_ingame_notification);

struct ClientEntry {
    uint8_t message_type;
    const ClientMessageHandler *handler;
};

constexpr ClientEntry k_client_entries[] = {
    {0x02, &k_beacon_reply},
    {0x03, &k_pong_reply},
    {0x04, &k_settings_request},
    {0x05, &k_join_accepted},
    {0x06, &k_connect_rejected},
    {0x07, &k_join_complete},
    {0x08, &k_settings_or_ack},
    {0x09, &k_player_config_value},
    {0x0a, &k_join_finalize_message},
    {0x0b, &k_join_finalize_ack},
    {0x0c, &k_discard_join_message},
    {0x0d, &k_discard_ingame_message},
    {0x16, &k_state_update_chunk},
    {0x17, &k_player_join_chunk},
    {0x18, &k_player_slot_chunk},
    {0x19, &k_sync_complete},
    {0x21, &k_replicated_command},
    {0x22, &k_ingame_notification},
};

class JoinHandshakeState final : public ClientStateHandler {
public:
    int8_t tick(network_client_globals *client) const override
    {
        return (int8_t)JoinView(client).handshake_tick();
    }
};

class JoinRetryState final : public ClientStateHandler {
public:
    int8_t tick(network_client_globals *client) const override
    {
        return (int8_t)JoinView(client).connect_retry_tick();
    }
};

class LobbyState final : public ClientStateHandler {
public:
    int8_t tick(network_client_globals *client) const override
    {
        return (int8_t)HostClientView(client).lobby_tick();
    }
};

class InGameState final : public ClientStateHandler {
public:
    int8_t tick(network_client_globals *client) const override
    {
        return ClientView(client).client_update();
    }
};

class ChannelServiceState final : public ClientStateHandler {
public:
    int8_t tick(network_client_globals *client) const override
    {
        return (int8_t)HostClientView(client).channel_service_tick();
    }
};

constexpr JoinHandshakeState k_join_handshake_state;
constexpr JoinRetryState k_join_retry_state;
constexpr LobbyState k_lobby_state;
constexpr InGameState k_in_game_state;
constexpr ChannelServiceState k_channel_service_state;

constexpr const ClientStateHandler *k_client_states[] = {
    &k_join_handshake_state, &k_join_retry_state, &k_lobby_state, &k_in_game_state, &k_channel_service_state,
};

using ServerHandleFn = uint32_t (*)(network_server_globals *server, network_machine *machine, uint8_t *bytes, int32_t length);

class ServerFunctionHandler final : public ServerMessageHandler {
public:
    ServerHandleFn handle_fn;

    explicit constexpr ServerFunctionHandler(ServerHandleFn fn) : handle_fn(fn) {}

    uint32_t handle(network_server_globals *server, network_machine *machine, uint8_t *bytes, int32_t length) const override
    {
        return handle_fn(server, machine, bytes, length);
    }
};

class KeepaliveHandler final : public ServerMessageHandler {
public:
    uint32_t handle(network_server_globals *server, network_machine *machine, uint8_t *bytes, int32_t length) const override
    {
        (void)server;
        if (network_disconnect_timeout_flag != 0) {
            int32_t body[1];
            int16_t out_type;
            uint16_t version_used;

            if (data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body,
                                                bytes + 2, &out_type, &version_used, 0) != 0) {
                ServerMessageHandlers::keepalive((network_channel **)machine, body);
            }
        }
        return 1;
    }
};

class PositionUpdateHandler final : public ServerMessageHandler {
public:
    uint32_t handle(network_server_globals *server, network_machine *machine, uint8_t *bytes, int32_t length) const override
    {
        if (*(int16_t *)((uint8_t *)server + 4) == 1) {
            uint32_t body[8];
            int16_t out_type;
            uint16_t version_used;

            if (data_packet_group_decode_packet((length -= 2, (int16_t *)&length), &network_game_messages_group, body,
                                                bytes + 2, &out_type, &version_used, 5) != 0) {
                GameRuntime::client_apply_position_update((uint8_t *)machine, body, (void *)-1, 0);
            }
        }
        return 1;
    }
};

uint32_t serve_join_password(network_server_globals *s, network_machine *m, uint8_t *b, int32_t n)
{
    return (uint8_t)ServerView(s).handle_join_password(m, b, n);
}

uint32_t serve_join_confirm(network_server_globals *s, network_machine *m, uint8_t *b, int32_t n)
{
    return (uint8_t)ServerView(s).handle_join_confirm(m, b, n);
}

uint32_t serve_settings_relay(network_server_globals *s, network_machine *, uint8_t *b, int32_t n)
{
    return (uint8_t)ServerMessageHandlers(s).settings_relay(b, n);
}

uint32_t serve_player_count_broadcast(network_server_globals *s, network_machine *, uint8_t *b, int32_t n)
{
    return (uint8_t)ServerMessageHandlers(s).player_count_broadcast(b, n);
}

uint32_t serve_player_entry_update(network_server_globals *s, network_machine *, uint8_t *b, int32_t n)
{
    return (uint8_t)ServerMessageHandlers(s).player_entry_update(b, n);
}

uint32_t serve_handshake_forward(network_server_globals *s, network_machine *, uint8_t *b, int32_t n)
{
    return (uint8_t)ServerMessageHandlers(s).handshake_forward(b, n);
}

uint32_t serve_retry_schedule(network_server_globals *s, network_machine *m, uint8_t *b, int32_t n)
{
    return (uint8_t)ServerMessageHandlers(s).retry_schedule(m, b, n);
}

uint32_t serve_build_version(network_server_globals *s, network_machine *m, uint8_t *b, int32_t n)
{
    return (uint8_t)ServerMessageHandlers(s).build_version(m, b, n);
}

uint32_t serve_info_request(network_server_globals *s, network_machine *m, uint8_t *b, int32_t n)
{
    return (uint8_t)ServerView(s).handle_info_request(m, b, n);
}

uint32_t serve_map_data(network_server_globals *s, network_machine *, uint8_t *b, int32_t n)
{
    return (uint8_t)ServerMessageHandlers(s).client_map_data(b, n);
}

uint32_t serve_client_settings_relay(network_server_globals *s, network_machine *, uint8_t *b, int32_t n)
{
    return (uint8_t)ServerMessageHandlers(s).client_settings_relay(b, n);
}

uint32_t serve_client_retry_schedule(network_server_globals *s, network_machine *m, uint8_t *b, int32_t n)
{
    return (uint8_t)ServerMessageHandlers(s).client_retry_schedule(m, b, n);
}

uint32_t serve_settings_relay_role2(network_server_globals *s, network_machine *, uint8_t *b, int32_t n)
{
    return (uint8_t)ServerMessageHandlers(s).settings_relay_role2(b, n);
}

uint32_t serve_join_finalize_ack_role2(network_server_globals *s, network_machine *m, uint8_t *b, int32_t n)
{
    return (uint8_t)ServerMessageHandlers(s).join_finalize_ack_role2(m, b, n);
}

constexpr KeepaliveHandler k_keepalive;
constexpr PositionUpdateHandler k_position_update;
constexpr ServerFunctionHandler k_join_password(serve_join_password);
constexpr ServerFunctionHandler k_join_confirm(serve_join_confirm);
constexpr ServerFunctionHandler k_settings_relay(serve_settings_relay);
constexpr ServerFunctionHandler k_player_count_broadcast(serve_player_count_broadcast);
constexpr ServerFunctionHandler k_player_entry_update(serve_player_entry_update);
constexpr ServerFunctionHandler k_handshake_forward(serve_handshake_forward);
constexpr ServerFunctionHandler k_retry_schedule(serve_retry_schedule);
constexpr ServerFunctionHandler k_build_version(serve_build_version);
constexpr ServerFunctionHandler k_info_request(serve_info_request);
constexpr ServerFunctionHandler k_map_data(serve_map_data);
constexpr ServerFunctionHandler k_client_settings_relay(serve_client_settings_relay);
constexpr ServerFunctionHandler k_client_retry_schedule(serve_client_retry_schedule);
constexpr ServerFunctionHandler k_settings_relay_role2(serve_settings_relay_role2);
constexpr ServerFunctionHandler k_join_finalize_ack_role2(serve_join_finalize_ack_role2);

struct ServerEntry {
    uint8_t message_type;
    const ServerMessageHandler *handler;
};

constexpr ServerEntry k_server_entries[] = {
    {0x01, &k_keepalive},
    {0x0e, &k_join_password},
    {0x0f, &k_join_confirm},
    {0x10, &k_settings_relay},
    {0x11, &k_player_count_broadcast},
    {0x12, &k_player_entry_update},
    {0x13, &k_handshake_forward},
    {0x14, &k_retry_schedule},
    {0x25, &k_retry_schedule},
    {0x15, &k_build_version},
    {0x1a, &k_info_request},
    {0x1b, &k_position_update},
    {0x1c, &k_map_data},
    {0x1d, &k_client_settings_relay},
    {0x1e, &k_client_retry_schedule},
    {0x23, &k_settings_relay_role2},
    {0x24, &k_join_finalize_ack_role2},
};

}

const ClientMessageHandler *ClientMessageRegistry::find(uint8_t message_type)
{
    for (const ClientEntry &entry : k_client_entries) {
        if (entry.message_type == message_type) {
            return entry.handler;
        }
    }
    return nullptr;
}

const ClientStateHandler *ClientStateMachine::handler_for(uint16_t state)
{
    if (state >= sizeof(k_client_states) / sizeof(k_client_states[0])) {
        return nullptr;
    }
    return k_client_states[state];
}

const ServerMessageHandler *ServerMessageRegistry::find(uint8_t message_type)
{
    for (const ServerEntry &entry : k_server_entries) {
        if (entry.message_type == message_type) {
            return entry.handler;
        }
    }
    return nullptr;
}

}
