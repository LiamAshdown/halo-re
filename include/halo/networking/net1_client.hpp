#pragma once

#include "halo/networking/net1_common.hpp"

namespace halo::networking {

/**
 * Behaviour group for the original `network_*` functions; every member is the original function body moved unchanged.
 */
class ClientView {
public:
    network_client_globals *self;

    explicit constexpr ClientView(network_client_globals *record) : self(record) {}

    static uint32_t begin_connect(wchar_t *player_name, s_network_address *target_address);
    static uint32_t check_connection_quality(uint32_t machine_index, uint8_t units);
    int16_t connect_progress_percent(int16_t *out_percent);
    static void connection_handshake_tick(int16_t state, network_server_globals *owner);
    static char drain_queued_updates(network_server_globals *server, network_machine *machine, bit_stream *stream);
    static int32_t globals_create();
    static void globals_dispose();
    static void handle_server_text_message(void *message);
    int32_t identity_tick();
    static void rejoin_check(int8_t machine_player_index);
    static void send_local_player_updates();
    int8_t state_dispatch();
    void timer_default_or_disconnect();
    void timer_schedule(int32_t delay_ms, int32_t context);
    static char update_dispatch();
    void disconnect_notify_dropped_machines();
    static uint32_t client_connect_to_address(wchar_t *player_name, char *address_string);
    int8_t client_update();
    int32_t record_message_send(const uint32_t *source);
    char join_finalize(network_player_entry *entry);
    static network_client_globals * create();
    void destroy();
    char info_packet_send(const uint32_t *source);
    void player_join_notify(const uint32_t *source);
    uint8_t player_table_index_apply(int32_t table_index, const uint8_t *candidate);
    int32_t staged_message_commit(uint16_t message_value);
};

/**
 * Behaviour group for the original `network_*` functions; every member is the original function body moved unchanged.
 */
class ConnectionView {
public:
    network_client_globals *self;

    explicit constexpr ConnectionView(network_client_globals *record) : self(record) {}

    int32_t endpoint_set(const uint32_t *source);
    static int32_t finalize_join(uint16_t *connection);
    int32_t initiate(const uint32_t *target, const uint32_t *session_info);
    void retransmit_if_overdue(const uint32_t *sender_address, uint32_t deadline_ms, int32_t remote_time);
    void send_keepalive();
    int32_t send_join_request_packet();
};

/**
 * Behaviour group for the original `network_host_*` functions; every member is the original function body moved unchanged.
 */
class HostClientView {
public:
    network_client_globals *self;

    explicit constexpr HostClientView(network_client_globals *record) : self(record) {}

    char channel_service_tick();
    char lobby_tick();
    void presence_broadcast_tick();
};

/**
 * Behaviour group for the original `network_join_*` functions; every member is the original function body moved unchanged.
 */
class JoinView {
public:
    network_client_globals *self;

    explicit constexpr JoinView(network_client_globals *record) : self(record) {}

    int32_t connect_retry_tick();
    uint32_t handshake_tick();
    static void hostname_resolved_callback(int32_t resolve_failed, uint32_t unused, uint8_t *hostent);
    static uint32_t request_resolve_host();
    void status_text_update(int32_t mode);
};

}
