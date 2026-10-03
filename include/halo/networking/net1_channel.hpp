#pragma once

#include "halo/networking/net1_common.hpp"

namespace halo::networking {

/**
 * Creation and teardown of channels, receive queues, buffer pools, mutex slots and network threads.
 */
class ChannelFactory {
public:
    ChannelFactory() = delete;

    static void clear_buffer_pair_pool();
    static network_channel_list * create_list(int16_t requested_capacity);
    static network_channel * create_channel(uint32_t flags);
    static network_channel * create_child(network_receive_queue *endpoint);
    static void close_all();
    static void open_all();
    static void close_all_handles();
    static network_mutex_record * allocate_mutex_slot();
    static network_receive_queue * create_receive_queue();
    static int32_t create_thread(uint8_t flags, void *start_address, void *parameter, network_thread_record **out_handle);
};

/**
 * Non-owning view of a connection receive queue: remote address, connect attempt, socket close and release.
 */
class ReceiveQueueView {
public:
    network_receive_queue *self;

    explicit constexpr ReceiveQueueView(network_receive_queue *record) : self(record) {}

    int16_t attempt_connect(s_network_address *address, int32_t unused_param_1, uint8_t use_query_socket);
    int16_t get_remote_address(s_network_address *address);
    uint32_t start();
    void close_socket();
    void release();
};

/**
 * GameSpy transport (GT2) and server browser callbacks registered when the channels are opened.
 */
class ChannelCallbacks {
public:
    ChannelCallbacks() = delete;

    static void on_connected(void *connection, int32_t result, const uint8_t *message, int32_t length);
    static void on_receive_dump(void *socket, void *connection, uint32_t ip, uint16_t port, int32_t reset, const void *message, int32_t length, int32_t reliable, int32_t resend);
    static void on_send_dump(void *socket, void *connection, uint32_t ip, uint16_t port, int32_t reset, const void *message, int32_t length);
    static void on_socket_error(void *socket);
    static int32_t on_game_socket_unrecognized(void *socket, uint32_t ip, uint16_t port, const uint8_t *message, uint32_t length);
    static int32_t on_query_socket_unrecognized(void *socket, uint32_t ip, uint16_t port, const uint8_t *message, uint32_t length);
    static void on_connection_error(void *connection);
    static void on_server_browser_list(void *sb, uint32_t reason, void *server, void *instance);
    static void on_receive(void *handle, uint8_t *data, int32_t length);
};

/**
 * Non-owning view of a network channel: queueing, servicing, retransmission and transmit.
 */
class ChannelView {
public:
    network_channel *self;

    explicit constexpr ChannelView(network_channel *record) : self(record) {}

    void destroy();
    int32_t incoming_read_item(uint8_t *destination, int32_t *out_bit_offset, int32_t *out_remaining_bits, s_network_address *out_address, int32_t max_item_bits);
    char listen_service(network_channel **out_new_child);
    char queue_message(uint32_t header_value, uint32_t body_value, int32_t header_bit_count, char immediate, char flush_after, int32_t body_bit_count);
    void record_timestamp();
    int32_t reliable_pool_ensure_capacity(int32_t body_capacity_needed, int32_t header_capacity_needed);
    void reliable_pool_store(uint8_t *body_data, uint8_t *header_data, int32_t priority, uint32_t header_bits, uint32_t body_bits);
    void remote_address_or_default(network_resolved_address *out_address);
    int32_t remove_child(network_channel *child);
    void scan_retransmit_timeouts();
    char service(int32_t timeout_ms, network_channel **out_new_child);
    int32_t service_close_if_disconnected();
    char service_light(int32_t timeout_ms, network_channel **out_new_child);
    int32_t service_retransmit_only();
    static int32_t short_disconnect_timeout();
    char transmit();
};

/**
 * Per-player channel key handling (open, close, target resolution and state send).
 */
class ChannelKeys {
public:
    ChannelKeys() = delete;

    static int32_t close(network_player_entry *entry, datum_index requested_handle);
    static int32_t open(network_player_entry *entry);
    static uint8_t resolve_target(network_player_entry *entry);
    static int32_t send_state(network_client_globals *client, int32_t **entry);
};

/**
 * Non-owning view of a select-style channel list (readable set plus de-duplicated socket keys).
 */
class ChannelListView {
public:
    network_channel_list *self;

    explicit constexpr ChannelListView(network_channel_list *record) : self(record) {}

    int32_t add(network_receive_queue *entry);
    int32_t mark_readable();
    int32_t remove(network_receive_queue *entry);
};

/**
 * Non-owning view of a channel outgoing stream.
 */
class ChannelStreamView {
public:
    network_channel_stream *self;

    explicit constexpr ChannelStreamView(network_channel_stream *record) : self(record) {}

    char flush(network_channel *channel, char mode);
    void init();
};

/**
 * Listen socket handlers: accept, reject and connection-request callbacks.
 */
class ListenerCallbacks {
public:
    ListenerCallbacks() = delete;

    static network_receive_queue * accept_pending_connection();
    static void connection_request_handler(int32_t listen_handle, int32_t reply_socket, uint32_t remote_address, uint32_t remote_port_raw, int32_t transport_handle, uint32_t *payload, uint32_t payload_length);
    static uint32_t reject_pending_connection(int32_t reject_code);
    static int32_t reject_pending_connection_callback(void *unused, int32_t reject_code);
};

}
