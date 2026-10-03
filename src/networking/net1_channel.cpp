#include "halo/networking/net1_channel.hpp"
#include <string.h>
#include "halo/memory/api.hpp"

extern "C" {
extern int32_t network_buffer_pair_pool;
extern int32_t network_buffer_pair_pool_count;
extern network_buffer_pair *network_buffer_pair_pool_data;
extern network_channel_list *network_channel_list_new(int16_t requested_capacity);
extern int32_t network_channel_list_add(network_receive_queue *entry, network_channel_list *list);
extern network_receive_queue *network_receive_queue_new(void);
extern uint32_t network_listen_start(network_receive_queue *queue);
extern void network_channel_delete(network_channel *channel);
extern void network_channel_record_timestamp(network_channel *channel);
extern void network_channel_stream_init(network_channel_stream *stream);
extern int32_t network_game_socket;
extern int32_t network_query_socket;
extern void gt2CloseSocket(int32_t socket);
extern uint32_t network_local_address;
extern uint8_t network_channels_open_ok;
extern uint32_t network_game_socket_port;
extern uint32_t game_cport;
extern void gt2AddressToString(uint32_t address, uint16_t port, void *out_address);
extern int32_t gt2CreateSocket(int32_t *socket_out, uint8_t address_buffer[24], int32_t unused_a, int32_t unused_b, void *receive_callback);
extern void gt2SetSendDump(int32_t socket, void *callback);
extern void gt2SetReceiveDump(int32_t socket, void *callback);
extern void gt2SetUnrecognizedMessageCallback(int32_t socket, void *callback);
extern void network_channel_gap_441020(void);
extern void network_channel_gap_441040(void);
extern void network_channel_gap_441060(void);
extern void network_channel_gap_4410b0(void);
extern void network_channel_gap_441200(void);
extern network_handle_registry_slot network_handle_registry[64];
extern network_mutex_record network_mutex_table[k_network_mutex_table_count];
extern void network_channels_open(void);
extern void network_handle_registry_close_all(void);
extern network_thread_record network_thread_table[k_network_thread_table_count];
extern int32_t gt2GetConnectionState(int32_t socket);
extern uint32_t gamespy_array_length(int32_t object);
extern uint16_t gt2GetRemotePort(int32_t object);
extern uint32_t gt2GetLocalIP(int32_t socket);
extern uint16_t gt2GetLocalPort(int32_t socket);
extern void gt2SetSocketData(int32_t socket, void *data);
extern int32_t gt2Listen(int32_t socket, void *callback);
extern void network_listen_connection_request_handler(int32_t listen_handle, int32_t reply_socket, uint32_t remote_address, uint32_t remote_port_raw, int32_t transport_handle, uint32_t *payload, uint32_t payload_length);
extern void gt2CloseConnectionHard(int32_t socket);
extern void gt2SetConnectionData(int32_t socket, network_receive_queue *queue);
extern void network_connection_stats_end(int32_t connection_id, int16_t connection_key);
extern void network_receive_queue_close_socket(network_receive_queue *queue);
extern void *gt2GetConnectionData(void *connection);
extern int16_t network_channel_get_remote_address(s_network_address *address, network_receive_queue *queue);
extern char *network_address_to_string(s_network_address *addr);
extern void network_connection_stats_record_packet(void *gamespy_connection, int32_t payload_length, uint8_t is_sent, uint8_t is_reliable, uint8_t is_resend);
extern int16_t network_join_error_code;
extern uint8_t network_host_handoff_requested;
extern void chat_close(void);
extern void gt2CloseAllConnections(void *socket);
extern uint8_t network_game_receive_buffer[0x2000];
extern const uint8_t natneg_magic[6];
extern void *network_session_host_object;
extern void NNProcessData(char *data, int32_t len, void *fromaddr);
extern void qr2_parse_queryA(void *qrec, char *query, int32_t len, void *sender);
extern uint8_t network_query_receive_buffer[0x2000];
typedef struct network_receive_queue network_receive_queue;
typedef struct server_list_globals server_list_globals;
extern uint8_t server_browser_initialized;
extern uint8_t DAT_00719488;
extern int32_t server_browser_query_elapsed_ms;
extern int32_t server_browser_selected_index;
extern int32_t server_browser_last_click_ms;
extern int32_t SBServerHasBasicKeys(void *server);
extern int32_t SBServerHasFullKeys(void *server);
extern server_list_globals *server_list_mutex_try_lock(uint32_t timeout_ms);
extern int32_t dynamic_pointer_array_add_unique(void *value, server_list_globals *array);
extern void server_list_mutex_unlock(server_list_globals **list_slot);
extern int32_t dynamic_pointer_array_find_index(server_list_globals *array, void *value);
extern void dynamic_pointer_array_remove_at(int32_t index, server_list_globals *array);
extern void server_browser_ui_refresh(void);
extern void network_receive_queue_free(network_receive_queue *queue);
extern int32_t network_channel_list_remove(network_receive_queue *entry, network_channel_list *list);
extern int32_t network_bit_chunk_size;
extern int32_t network_pending_connection_count;
extern int32_t network_channel_list_mark_readable(network_channel_list *list);
extern network_receive_queue *network_listen_accept_pending_connection(void);
extern uint32_t network_listen_reject_pending_connection(int32_t reject_code);
extern network_channel *network_channel_new_child(network_receive_queue *endpoint);
extern char network_channel_transmit(network_channel *channel);
extern int32_t network_server_validate_join_request(network_receive_queue *listen_endpoint);
extern void network_channel_reliable_pool_store(network_channel *channel, uint8_t *body_data, uint8_t *header_data, int32_t priority, uint32_t header_bits, uint32_t body_bits);
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode);
extern int64_t performance_frequency;
extern int32_t network_channel_reliable_pool_ensure_capacity(network_channel *channel, int32_t body_capacity_needed, int32_t header_capacity_needed);
extern int32_t network_rate_override;
extern int32_t network_rate_table[];
extern uint8_t network_channel_service_backoff_bypass;
extern int32_t unknown_00697ed8;
extern game_time_globals *game_time;
extern int16_t network_game_mode;
extern void network_channel_scan_retransmit_timeouts(network_channel *channel);
extern char network_channel_listen_service(network_channel *channel, network_channel **out_new_child);
extern network_server_globals *network_server;
extern uint8_t network_disconnect_timeout_flag;
extern uint8_t network_channel_key_resolve_target(network_player_entry *entry);
extern datum_index player_new_local(datum_index requested_handle, uint32_t machine_index, int16_t local_player_index, uint16_t *identifier_record);
extern int32_t player_new_network(int32_t machine_index, int16_t machine_player_index);
extern void network_index_cache_find_or_allocate_slot(int32_t index);
extern network_client_globals *network_client;
extern char network_player_entry_validate(network_player_entry *entry);
extern void message_delta_decode_compound_field_staged(void *decode_context);
extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination);
extern char network_game_settings_packet_receive(void *scratch);
extern int32_t gt2Send(int32_t socket, uint8_t *buffer, int32_t byte_count, int32_t mode);
extern network_pending_connection network_pending_connections[k_network_pending_connection_count];
extern void network_channel_gap_441f30(void);
extern void function_do_nothing(void);
extern int32_t gt2Accept(int32_t reply_socket, network_listen_accept_config *config);
extern void network_channel_receive_callback(void *handle, uint8_t *data, int32_t length);
extern void gt2Reject(int32_t socket, void *buffer, int32_t length);
extern void gt2GetSocketData(int32_t listen_handle);
}

namespace halo::networking {

/**
 * Frees both allocations of every entry in the pool, then resets the pool to empty (count and
 * the unknown flag both -1, data pointer freed and cleared).
 *
 * @address 0x4e3ed0
 */
void ChannelFactory::clear_buffer_pair_pool()
{
    int32_t i;

    if (0 < network_buffer_pair_pool_count) {
        for (i = 0; i < network_buffer_pair_pool_count; i = i + 1) {
            GlobalFree(network_buffer_pair_pool_data[i].first);
            GlobalFree(network_buffer_pair_pool_data[i].second);
        }
    }
    network_buffer_pair_pool = -1;
    network_buffer_pair_pool_count = -1;
    if (network_buffer_pair_pool_data != 0) {
        GlobalFree(network_buffer_pair_pool_data);
        network_buffer_pair_pool_data = 0;
    }
}

/**
 * Allocates a network_channel_list and its parallel dedup array (GMEM_ZEROINIT, requested
 * capacity * 4 bytes); frees the outer allocation and returns NULL if the capacity is 0x41 or
 * higher or the inner allocation fails.
 *
 * @address 0x441960
 */
network_channel_list * ChannelFactory::create_list(int16_t requested_capacity)
{
    network_channel_list *list;
    void *entries;

    list = (network_channel_list *)GlobalAlloc(0, 0x114);
    if (list == 0) {
        return 0;
    }
    if (requested_capacity < 0x41) {
        list->fd_count = 0;
        entries = GlobalAlloc(0x40, requested_capacity * 4);
        list->entries = (network_receive_queue **)entries;
        if (entries != 0) {
            list->capacity = requested_capacity;
            list->last_index = -1;
            list->service_cursor = 0;
            return list;
        }
    }
    GlobalFree(list);
    return 0;
}

/**
 * Allocates the 0xae4-byte listening variant when flags has k_network_channel_listening set, or
 * the 0xa9c-byte plain variant when it has k_network_channel_client set; any other combination
 * returns NULL. Wires up the receive queue, incoming circular buffer, and (for a listening
 * channel) its own child-listen list and OS listen socket, then primes both stream directions.
 * Tears everything down and returns NULL if any of these steps fails.
 *
 * @address 0x4dc9b0
 */
network_channel * ChannelFactory::create_channel(uint32_t flags)
{
    network_channel *channel;
    int ok;

    ok = 1;
    if ((flags & k_network_channel_listening) == 0) {
        if ((flags & k_network_channel_client) == 0) {
            return 0;
        }
        channel = (network_channel *)GlobalAlloc(0x40, 0xa9c);
        if (channel == 0) {
            return 0;
        }
    } else {
        channel = (network_channel *)GlobalAlloc(0x40, 0xae4);
        if (channel == 0) {
            return 0;
        }
        channel->listening = 1;
        channel->child_busy = 0;
        channel->listen_list = network_channel_list_new(0);
        if (channel->listen_list == 0) {
            network_channel_delete(channel);
            return 0;
        }
    }
    network_channel_record_timestamp(channel);
    channel->flags = flags;
    channel->endpoint = network_receive_queue_new();
    if (channel->endpoint != 0 &&
        ((flags & k_network_channel_listening) == 0 ||
         (network_listen_start(channel->endpoint) == 0 &&
          network_channel_list_add(channel->endpoint, channel->listen_list) == 0))) {
        channel->incoming = halo::memory::circular_buffer_new((char *)"transport-incoming", 0);
        if (channel->incoming != 0) {
            goto primed;
        }
    }
    ok = 0;
primed:
    channel->reliable_count = 0;
    channel->reliable = 0;
    channel->send_budget = 0xe0;
    channel->rate_index = 0;
    channel->budget_base_tick = GetTickCount();
    network_channel_stream_init(&channel->outgoing);
    network_channel_stream_init(&channel->retransmit);
    if (ok) {
        return channel;
    }
    network_channel_delete(channel);
    return 0;
}

/**
 * out/phase4/networking_functions.md: "Allocates and initializes a new child network
 * channel object for a freshly-accepted incoming connection, mirroring the smaller-variant setup
 * done by the general channel constructor." Same field offsets as network_channel_new.c's plain
 * (0xa9c-byte) allocation, with flags hard-coded to k_network_channel_transmit_pending (4)
 * instead of being a parameter.
 *
 * @address 0x4dd430
 */
network_channel * ChannelFactory::create_child(network_receive_queue *endpoint)
{
    network_channel *channel;

    channel = (network_channel *)GlobalAlloc(0x40, 0xa9c);
    if (channel != 0) {
        channel->endpoint = endpoint;
        channel->flags = k_network_channel_transmit_pending;
        channel->incoming = halo::memory::circular_buffer_new((char *)"transport-incoming", 0);
        network_channel_record_timestamp(channel);
        channel->reliable_count = 0;
        channel->reliable = 0;
        channel->send_budget = 0xe0;
        channel->rate_index = 0;
        channel->budget_base_tick = GetTickCount();
        network_channel_stream_init(&channel->outgoing);
        network_channel_stream_init(&channel->retransmit);
        if (channel->incoming == 0) {
            network_channel_delete(channel);
            return 0;
        }
    }
    return channel;
}

/**
 * out/phase4/networking_types_notes.md names network_query_socket (0x006f14c8) and
 * network_game_socket (0x006f14c4) directly; this is the exact inverse of
 * network_channels_open.c, closing both with the same foreign GameSpy transport call.
 *
 * @address 0x441480
 */
void ChannelFactory::close_all()
{
    if (network_query_socket != 0) {
        gt2CloseSocket(network_query_socket);
        network_query_socket = 0;
    }
    if (network_game_socket != 0) {
        gt2CloseSocket(network_game_socket);
        network_game_socket = 0;
    }
}

/**
 * out/phase4/networking_types_notes.md "network_channel (0xae4 listening / 0xa9c
 * plain)" section names network_game_socket (0x006f14c4), network_query_socket (0x006f14c8),
 * network_local_address (0x006869b0) and network_channels_open_ok (0x006869be) directly.
 *
 * @address 0x441300
 */
void ChannelFactory::open_all()
{
    uint32_t swapped_address;
    uint8_t game_address_buf[24];
    uint8_t query_address_buf[24];
    int32_t result;

    swapped_address = (uint32_t)((network_local_address << 0x10 | network_local_address & 0xff00 |
                                   network_local_address >> 0x10 & 0xff) << 8) |
                      network_local_address >> 0x18;
    network_channels_open_ok = 1;

    gt2AddressToString(swapped_address, (uint16_t)network_game_socket_port, game_address_buf);
    gt2AddressToString(swapped_address, (uint16_t)game_cport, query_address_buf);

    if (network_game_socket == 0) {
        result = gt2CreateSocket(&network_game_socket, game_address_buf, 0, 0,
                                     network_channel_gap_441060);
        if (result == 0) {
            gt2SetSendDump(network_game_socket, network_channel_gap_441020);
            gt2SetReceiveDump(network_game_socket, network_channel_gap_441040);
            gt2SetUnrecognizedMessageCallback(network_game_socket, network_channel_gap_4410b0);
        } else {
            network_channels_open_ok = 0;
        }
    }

    if (network_query_socket == 0 && network_channels_open_ok == 1) {
        result = gt2CreateSocket(&network_query_socket, query_address_buf, 0, 0,
                                     network_channel_gap_441060);
        if (result != 0) {
            game_cport = 0;
            gt2AddressToString(swapped_address, 0, query_address_buf);
            result = gt2CreateSocket(&network_query_socket, query_address_buf, 0, 0,
                                         network_channel_gap_441060);
            if (result != 0) {
                network_channels_open_ok = 0;
                return;
            }
        }
        gt2SetSendDump(network_query_socket, network_channel_gap_441020);
        gt2SetReceiveDump(network_query_socket, network_channel_gap_441040);
        gt2SetUnrecognizedMessageCallback(network_query_socket, network_channel_gap_441200);
    }
}

/**
 * Sweeps the 64-slot handle registry; for every slot that is both registered and still points
 * at a live record, closes the record's Win32 handle and clears both the record and the slot.
 *
 * @address 0x441bb0
 */
void ChannelFactory::close_all_handles()
{
    int32_t i;
    network_thread_record *record;

    for (i = 0; i < 64; i++) {
        record = network_handle_registry[i].record;
        if (record != 0 && network_handle_registry[i].registered != 0) {
            CloseHandle(record->handle);
            record->handle = 0;
            record->in_use = 0;
            network_handle_registry[i].record = 0;
            network_handle_registry[i].registered = 0;
        }
    }
}

/**
 * Finds the first free slot in the static mutex-record table, marks it in use, clears its
 * handle and the first byte of its name, and returns a pointer to it; returns NULL if every
 * slot is already in use.
 *
 * @address 0x440420
 */
network_mutex_record * ChannelFactory::allocate_mutex_slot()
{
    uint32_t i;

    for (i = 0; i < k_network_mutex_table_count; i++) {
        if (network_mutex_table[i].in_use == 0) {
            network_mutex_table[i].name[0] = 0;
            network_mutex_table[i].handle = 0;
            network_mutex_table[i].in_use = 1;
            return &network_mutex_table[i];
        }
    }
    return 0;
}

/**
 * Ensures the main channels are open and any stale handles are swept, then allocates and
 * zero/default-initializes a network_receive_queue and its backing 0x10001-byte circular
 * buffer ("received_data_queue"). Returns NULL if either GlobalAlloc fails; if the outer
 * allocation succeeds but the inner one does not, `incoming` is left NULL.
 *
 * @address 0x441bf0
 */
network_receive_queue * ChannelFactory::create_receive_queue()
{
    network_receive_queue *queue;
    circular_buffer *buffer;

    network_channels_open();
    network_handle_registry_close_all();
    queue = (network_receive_queue *)GlobalAlloc(0, 0x1c);
    if (queue != 0) {
        queue->socket = 0;
        queue->data_ready = 0;
        queue->connection_failed = 0;
        queue->socket_key = 0xffffffff;
        queue->flags = 0;
        queue->unknown_0d = 0x14;
        queue->last_error = 0;
        buffer = (circular_buffer *)GlobalAlloc(0, 0x10019);
        if (buffer != 0) {
            buffer->name = 0;
            buffer->signature = 0;
            buffer->read_cursor = 0;
            buffer->write_cursor = 0;
            buffer->capacity = 0;
            buffer->data = 0;
            buffer->name = (char *)"received_data_queue";
            buffer->signature = 0x63697263;
            buffer->capacity = 0x10001;
            buffer->data = (uint8_t *)buffer + 0x18;
        }
        queue->incoming = buffer;
        queue->unknown_14 = 0xffffffff;
        queue->reject_reason = 0;
    }
    return queue;
}

/**
 * Finds a free slot in the static worker-thread table, creates a suspended thread with a
 * 0x4000-byte stack, applies a priority derived from the low bits of flags (bit1 set ->
 * below normal, else bit2 set -> above normal, else normal), resumes it, and reports the
 * handle through out_handle. Returns 1 on success, 0 if the table is full or any Win32 call
 * fails (the handle is closed and the table slot left marked in use on failure).
 *
 * @address 0x440460
 */
int32_t ChannelFactory::create_thread(uint8_t flags, void *start_address, void *parameter, network_thread_record **out_handle)
{
    uint32_t i;
    network_thread_record *slot;
    uint32_t thread_id;
    int32_t priority;

    for (i = 0; i < k_network_thread_table_count; i++) {
        if (network_thread_table[i].in_use == 0) {
            slot = &network_thread_table[i];
            slot->handle = 0;
            slot->in_use = 1;
            slot->handle = CreateThread(0, 0x4000, (LPTHREAD_START_ROUTINE)start_address, parameter, 4, (LPDWORD)&thread_id);
            *out_handle = slot;
            if (slot->handle != 0) {
                if ((flags & 2) == 0) {
                    priority = (flags & 4) != 0 ? 1 : 0;
                } else {
                    priority = -1;
                }
                if (SetThreadPriority(slot->handle, priority) != 0) {
                    if (ResumeThread(slot->handle) != 0xffffffff) {
                        return 1;
                    }
                }
                CloseHandle(slot->handle);
            }
            return 0;
        }
    }
    return 0;
}

/**
 * Fills `address` from `queue`'s own socket if it has one and is connected (state 1);
 * otherwise falls back to the shared network_game_socket if that is open; otherwise zeroes the
 * address and reports k_network_error_no_address through queue->last_error. Always sets
 * address->size to k_network_address_size_ipv4 and clears queue->last_error on success.
 *
 * @address 0x441ce0
 */
int16_t ReceiveQueueView::get_remote_address(s_network_address *address)
{
    network_receive_queue *queue = self;
    int32_t state;
    uint32_t byte0;
    uint32_t byte1;
    uint32_t byte2;
    uint32_t byte3;

    if (queue->socket != 0) {
        state = gt2GetConnectionState(queue->socket);
        if (state == 1) {
            byte0 = gamespy_array_length(queue->socket);
            byte1 = gamespy_array_length(queue->socket);
            byte2 = gamespy_array_length(queue->socket);
            byte3 = gamespy_array_length(queue->socket);
            address->ipv4 = ((byte0 & 0xff0000 | byte1 >> 0x10) >> 8) |
                            ((byte2 << 0x10 | byte3 & 0xff00) << 8);
            address->port = gt2GetRemotePort(queue->socket);
            address->size = k_network_address_size_ipv4;
            queue->last_error = 0;
            return 0;
        }
    }
    if (network_game_socket != 0) {
        byte0 = gt2GetLocalIP(network_game_socket);
        byte1 = gt2GetLocalIP(network_game_socket);
        byte2 = gt2GetLocalIP(network_game_socket);
        byte3 = gt2GetLocalIP(network_game_socket);
        address->ipv4 = ((byte0 & 0xff0000 | byte1 >> 0x10) >> 8) |
                        ((byte2 << 0x10 | byte3 & 0xff00) << 8);
        address->port = gt2GetLocalPort(network_game_socket);
        address->size = k_network_address_size_ipv4;
        queue->last_error = 0;
        return 0;
    }
    address->ipv4 = 0;
    address->port = 0;
    address->size = k_network_address_size_ipv4;
    queue->last_error = (int16_t)k_network_error_no_address;
    return (int16_t)k_network_error_no_address;
}

/**
 * out/phase4/networking_functions.md summary ("puts the shared network channel into
 * a listening state and installs the incoming-connection-request callback"); the fields
 * written (+0x04 data_ready, +0x0c flags, +0x0e last_error) match network_receive_queue; only
 * caller is network_channel_new (0x4dc9b0, out of this session's range, out/phase2/
 * networking/02.md) which calls this with no visible argument right after constructing the
 * listening channel's endpoint queue.
 *
 * @address 0x442170
 */
uint32_t ReceiveQueueView::start()
{
    network_receive_queue *queue = self;
    uint32_t result;

    queue->data_ready = 1;
    gt2SetSocketData(network_game_socket, queue);
    queue->flags = queue->flags | 2;
    result = gt2Listen(network_game_socket, (void *)network_listen_connection_request_handler);
    queue->last_error = 0;
    return result & 0xffff0000;
}

/**
 * If the queue was flagged data_ready and the main game socket is open, unregisters that
 * socket from it (callback NULL). If the queue itself has a live socket whose GameSpy state is
 * 0 or 1, closes it. Always clears the socket field and the "connection oriented" flag (bit0).
 *
 * @address 0x442040
 */
void ReceiveQueueView::close_socket()
{
    network_receive_queue *queue = self;
    int32_t state;

    if ((int8_t)queue->data_ready == 1 && network_game_socket != 0) {
        gt2Listen(network_game_socket, 0);
    }
    if (queue->socket != 0) {
        state = gt2GetConnectionState(queue->socket);
        if (state == 1 || state == 0) {
            gt2CloseConnectionHard(queue->socket);
        }
    }
    queue->socket = 0;
    queue->flags = queue->flags & 0xfe;
}

/**
 * out/phase4/networking_functions.md summary ("destroys a network receive-queue
 * object created by network_receive_queue_new: closes its channel, frees the inner buffer and the wrapper,
 * and cleans up handles"); called from network_channel_delete (0x4dcae0, out/phase2/
 * networking/02.md) as `if (channel->endpoint != 0) network_receive_queue_free();`, which is
 * how the EAX-as-queue-pointer convention is confirmed (channel->endpoint is loaded into the
 * register the comparison just used, and the call follows immediately).
 *
 * @address 0x441c80
 */
void ReceiveQueueView::release()
{
    network_receive_queue *queue = self;
    if (queue != 0 && queue->socket != 0) {
        int32_t connection_id = (int32_t)gamespy_array_length(queue->socket);
        int16_t connection_key = (int16_t)gt2GetRemotePort(queue->socket);
        network_connection_stats_end(connection_id, connection_key);
        gt2SetConnectionData(queue->socket, 0);
    }
    network_receive_queue_close_socket(queue);
    GlobalFree(queue->incoming);
    queue->incoming = 0;
    GlobalFree(queue);
    network_handle_registry_close_all();
}

/**
 * WRITTEN 2026-09-28 (retail-independence loop) from the disassembly 0x441e00..0x441ec9. It had no C because it is
 * reached only as an immediate: network_channel_attempt_connect (0x441f60) stores it as the `connected` member of
 * the GT2ConnectionCallbacks it hands gt2Connect (with network_channel_receive_callback 0x441ed0,
 * network_channel_gap_441f30 and function_do_nothing 0x44ad80). gt2 calls it with (connection, result, message,
 * length) once the connect attempt resolves: the connection's data is the channel's receive queue
 * (gt2GetConnectionData); the remote address is resolved and formatted (for the log); then
 * result 0 (connected): no error, flags |= 0x31 (connection oriented, readable, ...)
 *
 * @address 0x441e00
 */
void ChannelCallbacks::on_connected(void *connection, int32_t result, const uint8_t *message, int32_t length)
{
    network_receive_queue *queue = (network_receive_queue *)gt2GetConnectionData(connection);
    s_network_address address;

    if (queue == 0) {
        return;
    }
    network_channel_get_remote_address(&address, queue);
    network_address_to_string(&address);
    if (result == 0) {
        queue->last_error = 0;
        queue->flags |= 0x31;
        return;
    }
    if (result == 2) {
        int32_t reason = 1;

        if (message != 0 && length == 4) {
            int32_t code = *(const int32_t *)message;

            if (code > 0 && code < 9) {
                reason = code;
            }
        }
        queue->reject_reason = reason;
        queue->connection_failed = 1;
        queue->last_error = -0x18;
        network_receive_queue_close_socket(queue);
        queue->flags |= 0x40;
        return;
    }
    queue->last_error = -0x10;
    network_receive_queue_close_socket(queue);
    if (result == 6) {
        queue->flags |= 0x80;
    }
}

/**
 * the GT2 receive dump callback set by network_channels_open
 * (socket, connection, ip, port, reset, message, length, reliable, resend): records the packet with is_sent 1 and
 * the last two arguments as reliable / resend.
 *
 * @address 0x441020
 */
void ChannelCallbacks::on_receive_dump(void *socket, void *connection, uint32_t ip, uint16_t port, int32_t reset, const void *message, int32_t length, int32_t reliable, int32_t resend)
{
    (void)socket;
    (void)ip;
    (void)port;
    (void)reset;
    (void)message;
    network_connection_stats_record_packet(connection, length, 1, (uint8_t)reliable, (uint8_t)resend);
}

/**
 * the GT2 send dump callback set by network_channels_open:
 * records the packet with is_sent, reliable and resend all 0.
 *
 * @address 0x441040
 */
void ChannelCallbacks::on_send_dump(void *socket, void *connection, uint32_t ip, uint16_t port, int32_t reset, const void *message, int32_t length)
{
    (void)socket;
    (void)ip;
    (void)port;
    (void)reset;
    (void)message;
    network_connection_stats_record_packet(connection, length, 0, 0, 0);
}

/**
 * the GT2 socket-error callback network_channels_open gives
 * both sockets: an unset join error (-1) becomes 6, a host handoff is requested, chat closes, every connection on
 * the socket is closed (gt2CloseAllConnections 0x614740, soft) and the socket global that held it -- the game
 * socket when it is that one, otherwise the query socket -- is cleared (GT2 frees the socket after this returns).
 *
 * @address 0x441060
 */
void ChannelCallbacks::on_socket_error(void *socket)
{
    if (network_join_error_code == -1) {
        network_join_error_code = 6;
    }
    network_host_handoff_requested = 1;
    chat_close();
    gt2CloseAllConnections(socket);
    if ((int32_t)socket == network_game_socket) {
        network_game_socket = 0;
    } else {
        network_query_socket = 0;
    }
}

/**
 * the game socket's GT2 unrecognized-message callback (socket,
 * ip, port, message, length): like 0x441200 (copy of at most 0x1fff bytes to 0x006a4140; natneg packets to
 * NNProcessData and handled) but a query ("\\" or ";" first, or 0xfe 0xfd) also goes to qr2_parse_queryA for the
 * host record when there is one; queries count as handled, anything else 0.
 *
 * @address 0x4410b0
 */
int32_t ChannelCallbacks::on_game_socket_unrecognized(void *socket, uint32_t ip, uint16_t port, const uint8_t *message, uint32_t length)
{
    uint8_t is_natneg = 0;
    uint8_t is_query;
    uint8_t address[16];

    (void)socket;
    if (length >= 0x1fff) {
        length = 0x1fff;
    }
    memcpy(network_game_receive_buffer, message, length);
    network_game_receive_buffer[length] = 0;
    if ((int32_t)length >= 6 && memcmp(network_game_receive_buffer, natneg_magic, 6) == 0) {
        is_natneg = 1;
    }
    is_query = ((int32_t)length >= 1 && network_game_receive_buffer[0] == 0x5c) || network_game_receive_buffer[0] == 0x3b ||
               ((int32_t)length >= 2 && network_game_receive_buffer[0] == 0xfe && network_game_receive_buffer[1] == 0xfd);
    memset(address, 0, sizeof(address));
    *(uint16_t *)(address + 0) = 2;
    *(uint16_t *)(address + 2) = (uint16_t)((port >> 8) | (port << 8));
    *(uint32_t *)(address + 4) = ip;
    if (is_natneg) {
        NNProcessData((char *)network_game_receive_buffer, (int32_t)length, address);
        return 1;
    }
    if (!is_query) {
        return 0;
    }
    if (network_session_host_object != 0) {
        qr2_parse_queryA(network_session_host_object, (char *)network_game_receive_buffer, (int32_t)length, address);
    }
    return 1;
}

/**
 * the query socket's GT2 unrecognized-message callback (socket,
 * ip, port, message, length): the message (at most 0x1fff bytes) is copied to 0x006a6148 and terminated; a natneg
 * packet (the 6 byte magic at 0x00657208) goes to NNProcessData with the sender as a sockaddr_in and is handled
 * (1); a query ("\\" or ";" first, or 0xfe 0xfd) counts as handled (1); anything else 0.
 *
 * @address 0x441200
 */
int32_t ChannelCallbacks::on_query_socket_unrecognized(void *socket, uint32_t ip, uint16_t port, const uint8_t *message, uint32_t length)
{
    uint8_t is_natneg = 0;
    uint8_t is_query;

    (void)socket;
    if (length >= 0x1fff) {
        length = 0x1fff;
    }
    memcpy(network_query_receive_buffer, message, length);
    network_query_receive_buffer[length] = 0;
    if ((int32_t)length >= 6 && memcmp(network_query_receive_buffer, natneg_magic, 6) == 0) {
        is_natneg = 1;
    }
    is_query = ((int32_t)length >= 1 && network_query_receive_buffer[0] == 0x5c) || network_query_receive_buffer[0] == 0x3b ||
               ((int32_t)length >= 2 && network_query_receive_buffer[0] == 0xfe && network_query_receive_buffer[1] == 0xfd);
    if (is_natneg) {
        uint8_t address[16];

        memset(address, 0, sizeof(address));
        *(uint16_t *)(address + 0) = 2;
        *(uint16_t *)(address + 2) = (uint16_t)((port >> 8) | (port << 8));
        *(uint32_t *)(address + 4) = ip;
        NNProcessData((char *)network_query_receive_buffer, (int32_t)length, address);
        return 1;
    }
    return is_query ? 1 : 0;
}

/**
 * the GT2 connection callback set by
 * network_listen_accept_pending_connection (config.error_callback): the connection's receive queue
 * (gt2GetConnectionData) gets byte +5 = 1, network_receive_queue_close_socket (ESI queue) and flag 0x40 in +0x0c.
 * (The name keeps the one its registrant uses.)
 *
 * @address 0x441f30
 */
void ChannelCallbacks::on_connection_error(void *connection)
{
    uint8_t *queue = (uint8_t *)gt2GetConnectionData(connection);

    if (queue != 0) {
        queue[0x05] = 1;
        network_receive_queue_close_socket((network_receive_queue *)queue);
        queue[0x0c] |= 0x40;
    }
}

/**
 * the ServerBrowser list callback (sb, reason, server,
 * instance) registered by server_browser_open, active once the browser is initialized: server added (0) with basic
 * or full keys, and server updated (1), add the server to the locked list (0x4ba760 / 0x4ba8a0); deleted (2, 3)
 * removes it (0x4ba870 / 0x4ba940), dropping the selection and refreshing the UI when it was selected; query
 * complete (4) resets the elapsed time to 9999 when a query was pending (0x00719488). (Named as its registrant
 * names it.)
 *
 * @address 0x4ba660
 */
void ChannelCallbacks::on_server_browser_list(void *sb, uint32_t reason, void *server, void *instance)
{
    server_list_globals *list;

    (void)sb;
    (void)instance;
    if (server_browser_initialized == 0 || reason > 4) {
        return;
    }
    switch (reason) {
    case 0:
        if (SBServerHasBasicKeys(server) == 0 && SBServerHasFullKeys(server) == 0) {
            return;
        }

    case 1:
        list = server_list_mutex_try_lock(100);
        if (list != 0) {
            dynamic_pointer_array_add_unique(server, list);
            server_list_mutex_unlock(&list);
        }
        return;
    case 2:
    case 3:
        list = server_list_mutex_try_lock(100);
        if (list != 0) {
            int32_t index = dynamic_pointer_array_find_index(list, server);

            if (index != -1) {
                dynamic_pointer_array_remove_at(index, list);
                if (index == server_browser_selected_index) {
                    server_browser_selected_index = -1;
                    server_browser_last_click_ms = 0;
                    server_browser_ui_refresh();
                }
            }
            server_list_mutex_unlock(&list);
        }
        return;
    default:
        if (DAT_00719488 != 0) {
            server_browser_query_elapsed_ms = 9999;
        }
        DAT_00719488 = 0;
        return;
    }
}

/**
 * out/phase4/networking_functions.md: "Tears down and frees a network channel object,
 * recursively closing/freeing any child channels, its buffers, and associated OS handles before
 * freeing the object itself." Every dword-indexed offset (channel[3]=0x00c, channel+0x2a3(byte)=
 * 0xa8c, channel+0x2a8=0xaa0, channel[0x2a7]=0xa9c, channel+0xb(byte)=0x02c, channel[4..10]=
 * 0x010..0x028, channel+0x158(byte)=0x560, channel[0x151..0x157]=0x544..0x55c,
 * channel[0x29e]/[0x29f]/[0x2a0]=0xa78/0xa7c/0xa80) matches types/networking.h's network_channel
 * field offsets exactly (incoming, flags, children, listen_list, in.empty, in.stream.*,
 *
 * @address 0x4dcae0
 */
void ChannelView::destroy()
{
    network_channel *channel = self;
    int i;
    network_channel_reliable_slot *slot;

    if (channel == 0) {
        return;
    }
    if (channel->endpoint != 0) {
        network_receive_queue_free(channel->endpoint);
    }
    if (channel->incoming != 0) {
        GlobalFree(channel->incoming);
    }
    if (channel->flags & k_network_channel_listening) {
        for (i = 0; i < 16; i++) {
            if (channel->children[i] != 0) {
                if (channel->listen_list != 0) {
                    network_channel_list_remove(channel->children[i]->endpoint, channel->listen_list);
                }
                network_channel_delete(channel->children[i]);
            }
        }
        if (channel->listen_list != 0) {
            GlobalFree(channel->listen_list->entries);
            GlobalFree(channel->listen_list);
        }
    }
    channel->outgoing.stream.unknown_00 = (uint32_t)-1;
    channel->outgoing.stream.data = 0;
    channel->outgoing.stream.first_bit = 0;
    channel->outgoing.stream.byte_cursor = 0;
    channel->outgoing.stream.bit_cursor = 0;
    channel->outgoing.stream.last_bit = 0;
    channel->outgoing.capacity_bits = 0;
    channel->outgoing.empty = 1;
    channel->retransmit.stream.unknown_00 = (uint32_t)-1;
    channel->retransmit.stream.data = 0;
    channel->retransmit.stream.first_bit = 0;
    channel->retransmit.stream.byte_cursor = 0;
    channel->retransmit.stream.bit_cursor = 0;
    channel->retransmit.stream.last_bit = 0;
    channel->retransmit.capacity_bits = 0;
    channel->retransmit.empty = 1;
    if (channel->reliable_count > 0) {
        slot = channel->reliable;
        for (i = 0; i < channel->reliable_count; i++) {
            GlobalFree(slot[i].header);
            GlobalFree(slot[i].body);
        }
        GlobalFree(channel->reliable);
        channel->reliable = 0;
        channel->reliable_count = 0;
    }
    channel->send_budget = 0;
    GlobalFree(channel);
}

/**
 * out/phase4/networking_functions.md: "Reads one length-prefixed item out of the
 * channel's incoming ring buffer, used by the incoming-message processing loop before each item
 * is dispatched." Peeks 2 bytes from channel->incoming, decodes a bit-chunked length prefix from
 * them, validates the length against both a hidden byte-count bound and the buffer's actual
 * available bytes, then (on success) consumes the item into `destination` and reports the
 * decoded length via out_bit_offset/out_remaining_bits, resetting the buffer's cursors to 0 on
 * any failure path.
 *
 * @address 0x4dcf10
 */
int32_t ChannelView::incoming_read_item(uint8_t *destination, int32_t *out_bit_offset, int32_t *out_remaining_bits, s_network_address *out_address, int32_t max_item_bits)
{
    network_channel *channel = self;
    circular_buffer *incoming;
    int32_t available;
    uint8_t peeked[4];
    int32_t item_length;
    bit_stream length_stream;
    int32_t chunk_size;
    int32_t consumed_bits;
    int32_t max_item_bytes;

    incoming = channel->incoming;
    if (incoming == 0) {
        return 0;
    }
    available = incoming->write_cursor - incoming->read_cursor;
    if (available < 0) {
        available = available + incoming->capacity;
    }
    if (available <= 0) {
        return 0;
    }
    item_length = 0;
    if (halo::memory::circular_buffer_read(peeked, 2, 0, incoming) == 0) {
        return 0;
    }
    chunk_size = network_bit_chunk_size;
    length_stream.unknown_00 = 1;
    length_stream.data = peeked;
    length_stream.first_bit = 0;
    length_stream.byte_cursor = 0;
    length_stream.bit_cursor = 0;
    length_stream.last_bit = 0xf;
    consumed_bits = halo::memory::bit_stream_read_bits_chunked(chunk_size, (uint32_t *)&item_length, &length_stream);
    if (consumed_bits != chunk_size) {
        return 0;
    }
    if (item_length > 1) {

        max_item_bytes = max_item_bits / 8 + ((max_item_bits % 8) != 0);
        if (item_length <= max_item_bytes) {
            int32_t available2 = incoming->write_cursor - incoming->read_cursor;
            if (available2 < 0) {
                available2 = available2 + incoming->capacity;
            }
            if (item_length <= available2) {
                halo::memory::circular_buffer_read(destination, item_length, 1, incoming);
                if (out_address != 0) {
                    if (network_channel_get_remote_address(out_address, channel->endpoint) != 0) {
                        out_address->ipv4 = 0;
                        out_address->ipv6_1 = 0;
                        out_address->ipv6_2 = 0;
                        out_address->ipv6_3 = 0;
                        out_address->size = k_network_address_size_ipv4;
                        out_address->port = 0;

                        *(uint32_t *)((uint8_t *)out_address + 0x14) = 0;
                    }
                }
                *out_bit_offset = chunk_size;
                *out_remaining_bits = item_length * 8 - chunk_size;
                return 1;
            }
        }
    }
    incoming->write_cursor = 0;
    incoming->read_cursor = 0;
    return 0;
}

/**
 * Scans the listening channel's readable-socket list. Data on the listen socket itself triggers
 * an accept attempt (subject to the 16-child table and the pending-connection queue); data on an
 * already-accepted child's endpoint is forwarded via network_channel_transmit, with a failed
 * transmit marking that child dead (k_network_channel_dead) instead of aborting the whole scan.
 *
 * @address 0x4dd4e0
 */
char ChannelView::listen_service(network_channel **out_new_child)
{
    network_channel *channel = self;
    int32_t mark_result;
    int32_t idx;
    network_receive_queue *entry;
    char result;
    char found_one;
    circular_buffer *incoming;
    int32_t available;
    int32_t i;
    int32_t reject_code;
    network_channel *new_child;
    s_network_address remote_address;

    *out_new_child = 0;
    result = 1;
    mark_result = network_channel_list_mark_readable(channel->listen_list);
    if (mark_result != 0) {
        if (mark_result == -0xd) {
            return 1;
        }
        return 0;
    }
    channel->listen_list->service_cursor = 0;
    found_one = 0;
    for (;;) {
        idx = channel->listen_list->service_cursor;
        if (channel->listen_list->last_index < idx) {
            return result;
        }
        entry = channel->listen_list->entries[idx];
        channel->listen_list->service_cursor = idx + 1;
        if (entry == 0) {
            return result;
        }
        if (found_one != 0) {
            return result;
        }

        if (entry->data_ready == 1 && network_pending_connection_count > 0) {
            goto handle_readable;
        }
        incoming = entry->incoming;
        if (incoming != 0) {
            available = incoming->write_cursor - incoming->read_cursor;
            if (available < 0) {
                available = available + incoming->capacity;
            }
            if (available > 0) {
                goto handle_readable;
            }
        }
        continue;

    handle_readable:
        if (entry == channel->endpoint) {
            if (channel->listen_list->last_index + 1 < 0x11) {
                if (channel->listening == 0) {
                    reject_code = 2;
                } else {
                    reject_code = network_server_validate_join_request(channel->endpoint);
                    if (reject_code == 0) {
                        entry = network_listen_accept_pending_connection();
                        if (entry != 0) {
                            new_child = network_channel_new_child(entry);
                            if (new_child != 0) {
                                for (i = 0; i < 0x10; i++) {
                                    if (channel->children[i] == 0) {
                                        *out_new_child = new_child;
                                        channel->children[i] = new_child;
                                        new_child->parent = channel;
                                        new_child->connected = 0;
                                        if (channel->child_busy == 0) {
                                            network_channel_get_remote_address(&remote_address, new_child->endpoint);
                                            if (new_child->endpoint->last_error == 0 &&
                                                (remote_address.ipv4 == 0x7f000001 ||
                                                 remote_address.ipv4 == network_local_address)) {
                                                new_child->connected = 1;
                                                channel->child_busy = 1;
                                            }
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                        continue;
                    }

                }
            } else {
                reject_code = 6;
            }
            if (channel->accept_callback == 0) {
                network_listen_reject_pending_connection(reject_code);
            } else {
                ((network_channel_accept_callback)(void *)(uint32_t)channel->accept_callback)(entry);
            }
        } else {
            for (i = 0; i < 0x10; i++) {
                if (channel->children[i] != 0 && channel->children[i]->endpoint == entry) {
                    result = network_channel_transmit(channel->children[i]);
                    if (result == 0) {
                        network_channel_list_remove(channel->children[i]->endpoint, channel->listen_list);
                        channel->children[i]->flags = channel->children[i]->flags | k_network_channel_dead;
                        result = 1;
                    }
                    break;
                }
            }
        }
        if (result == 0) {
            return 0;
        }
    }
}

/**
 * out/phase4/networking_functions.md: "Queues an outgoing message either for
 * immediate transmission or, when not marked immediate, into the reliable retransmission buffer
 * pool, depending on a caller-supplied mode flag." Matches: when immediate (param_4 == 1) and
 * there is room, writes header then body bits directly into a channel bit_stream via
 * bit_stream_write_bits_chunked and marks it non-empty; otherwise (or if there is no room even
 * after one flush attempt via network_channel_stream_flush) falls back to
 * network_channel_reliable_pool_store (network_channel_reliable_pool_store).
 *
 * @address 0x4dce40
 */
char ChannelView::queue_message(uint32_t header_value, uint32_t body_value, int32_t header_bit_count, char immediate, char flush_after, int32_t body_bit_count)
{
    network_channel *channel = self;
    char result;
    int32_t free_bits;

    if (channel->flags & k_network_channel_listening) {
        return 1;
    }
    result = 1;
    if (immediate == 1) {
        free_bits = (channel->outgoing.stream.last_bit - channel->outgoing.stream.byte_cursor * 8) -
                    channel->outgoing.stream.bit_cursor + 1;
        if (free_bits < body_bit_count + header_bit_count) {
            result = network_channel_stream_flush(&channel->outgoing, channel, 1);
            if (result == 0) {
                return 0;
            }
        }
        channel->send_budget = channel->send_budget + body_bit_count + header_bit_count;
        halo::memory::bit_stream_write_bits_chunked(&channel->outgoing.stream, (const uint32_t *)header_value, header_bit_count);
        channel->outgoing.empty = 0;
        halo::memory::bit_stream_write_bits_chunked(&channel->outgoing.stream, (const uint32_t *)body_value, body_bit_count);
        channel->outgoing.empty = 0;
        if (flush_after != 1) {
            return result;
        }
        result = network_channel_stream_flush(&channel->outgoing, channel, 1);
        return result;
    }
    network_channel_reliable_pool_store(channel, (uint8_t *)&body_value, (uint8_t *)&header_value,
        0, header_bit_count, body_bit_count);
    return 1;
}

/**
 * out/phase4/networking_functions.md: "Records the current time (in milliseconds)
 * into a channel's timestamp field, used elsewhere for timeout comparisons." channel+4 matches
 * types/networking.h's network_channel.last_activity_ms exactly.
 *
 * @address 0x4dd930
 */
void ChannelView::record_timestamp()
{
    network_channel *channel = self;
    large_integer counter;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    channel->last_activity_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
}

/**
 * Scans the pool for a free (pending == 0) slot already large enough for both requested
 * capacities; if one exists, returns its index without doing anything else. Otherwise grows the
 * pool by 10 slots (reallocating and copying the existing array), initializes each new slot with
 * max(requested, 100)-byte header/body allocations, and returns the index of the first new slot.
 *
 * @address 0x4dcc30
 */
int32_t ChannelView::reliable_pool_ensure_capacity(int32_t body_capacity_needed, int32_t header_capacity_needed)
{
    network_channel *channel = self;
    int32_t i;
    network_channel_reliable_slot *slot;
    network_channel_reliable_slot *new_pool;
    int32_t old_count;
    int32_t header_cap;
    int32_t body_cap;

    i = 0;
    if (channel->reliable_count > 0) {
        slot = channel->reliable;
        do {
            if (slot->pending == 0 && body_capacity_needed <= slot->body_capacity &&
                header_capacity_needed <= slot->header_capacity) {
                if (i != -1) {
                    return i;
                }
                break;
            }
            i = i + 1;
            slot = slot + 1;
        } while (i < channel->reliable_count);
    }

    new_pool = (network_channel_reliable_slot *)GlobalAlloc(0, (channel->reliable_count + 10) * 0x20);
    if (channel->reliable_count > 0) {
        memcpy(new_pool, channel->reliable, (uint32_t)channel->reliable_count * 0x20);
        GlobalFree(channel->reliable);
    }
    old_count = channel->reliable_count;
    channel->reliable = new_pool;
    for (i = old_count; i < channel->reliable_count + 10; i++) {
        slot = &channel->reliable[i];
        slot->pending = 0;
        slot->body_bits = 0;
        slot->header_bits = 0;
        slot->priority = -1;
        header_cap = header_capacity_needed;
        if (header_capacity_needed < 100) {
            header_cap = 100;
        }
        slot->header_capacity = header_cap;
        body_cap = body_capacity_needed;
        if (body_capacity_needed < 100) {
            body_cap = 100;
        }
        slot->body_capacity = body_cap;
        slot->body = (uint8_t *)GlobalAlloc(0, slot->body_capacity);
        slot->header = (uint8_t *)GlobalAlloc(0, slot->header_capacity);
    }
    channel->reliable_count = channel->reliable_count + 10;
    return old_count;
}

/**
 * out/phase4/networking_functions.md: "Stores a header/body message pair into a
 * tagged slot of the channel's reliable-message buffer pool for later (re)transmission
 * tracking." Calls network_channel_reliable_pool_ensure_capacity(channel, body_bytes,
 * header_bytes) with the byte counts rounded up from bit counts (ceil(bits/8)), matching
 * types/networking.h's note that this function's argument order is the reverse of that one's.
 * register/parameter convention: EAX -> header_bits, ECX -> body_bits (both elided from
 * Ghidra's own signature); stack -> channel, body_data, header_data, priority.
 *
 * @address 0x4dcdb0
 */
void ChannelView::reliable_pool_store(uint8_t *body_data, uint8_t *header_data, int32_t priority, uint32_t header_bits, uint32_t body_bits)
{
    network_channel *channel = self;
    uint32_t header_bytes;
    uint32_t body_bytes;
    int32_t index;
    network_channel_reliable_slot *slot;
    uint32_t i;

    header_bytes = (header_bits >> 3) + ((header_bits & 7) != 0);
    body_bytes = (body_bits >> 3) + ((body_bits & 7) != 0);
    index = network_channel_reliable_pool_ensure_capacity(channel, body_bytes, header_bytes);
    slot = &channel->reliable[index];
    slot->priority = priority;
    slot->header_bits = header_bits;
    slot->body_bits = body_bits;
    slot->pending = 1;
    for (i = 0; i < header_bytes; i++) {
        slot->header[i] = header_data[i];
    }
    for (i = 0; i < body_bytes; i++) {
        slot->body[i] = body_data[i];
    }
}

/**
 * from the rewriters' network_message_decode_guard)
 * address 0x4dd390, size 84 bytes
 * name confidence: 0.6   rewrite confidence: 0.75
 * the disassembly settles what the decompilation could not. 0x4dd390 is
 * push esi / mov esi,ecx / test esi,esi / je ret        -> ECX is the out record, may be NULL
 * mov edi,[eax] / test edi,edi / je default             -> EAX is a network_channel *, and
 * [eax] is channel->endpoint
 *
 * @address 0x4dd390
 */
void ChannelView::remote_address_or_default(network_resolved_address *out_address)
{
    network_channel *channel = self;
    network_receive_queue *queue;
    int use_default;

    if (out_address == 0) {
        return;
    }
    use_default = 0;
    queue = channel->endpoint;
    if (queue == 0) {
        use_default = 1;
    } else if (network_channel_get_remote_address(&out_address->address, queue) != 0) {
        use_default = 1;
    }
    if (use_default) {
        out_address->address.ipv4 = 0;
        out_address->address.ipv6_1 = 0;
        out_address->address.ipv6_2 = 0;
        out_address->address.ipv6_3 = 0;
        *(uint32_t *)&out_address->address.size = 0;
        out_address->unknown_14 = 0;
        out_address->address.size = k_network_address_size_ipv4;
    }
}

/**
 * out/phase4/networking_functions.md: "Finds a specific child channel in the parent's
 * child table, closes its socket, deletes the child object, and clears the table entry."
 * children[] (0xaa0, 16 entries) and connected (0xa98) match types/networking.h's
 * network_channel exactly.
 *
 * @address 0x4dd090
 */
int32_t ChannelView::remove_child(network_channel *child)
{
    network_channel *parent = self;
    int32_t i;

    i = 0;
    while (parent->children[i] == 0 || parent->children[i] != child) {
        i = i + 1;
        if (i > 0x10) {
            return 0;
        }
    }
    if (child->endpoint != 0) {
        network_channel_list_remove(parent->children[i]->endpoint, parent->listen_list);
    }
    if (parent->children[i]->connected == 1) {
        parent->child_busy = 0;
    }
    network_channel_delete(parent->children[i]);
    parent->children[i] = 0;
    return 1;
}

/**
 * already named)
 * address 0x4dd9d0, size 368 bytes
 * name confidence: 0.5   rewrite confidence: 0.3
 * out/phase4/networking_functions.md: "Scans the channel's reliable-message buffer
 * pool for entries that are overdue based on an estimated delivery rate and retransmits them,
 * then clears the pool's active flags." The free-space formula
 * (out.stream.last_bit - out.stream.byte_cursor*8 - out.stream.bit_cursor + 1) matches
 *
 * @address 0x4dd9d0
 */
void ChannelView::scan_retransmit_timeouts()
{
    network_channel *channel = self;
    uint32_t now;
    int32_t rate;
    int32_t priority;
    int32_t i;
    network_channel_reliable_slot *slot;
    int32_t message_bits;
    int32_t budget_bits;
    int32_t free_bits;

    now = GetTickCount();
    rate = network_rate_override;
    if (network_rate_override == 0) {
        rate = network_rate_table[channel->rate_index];
    }
    for (priority = 0; priority < 10; priority++) {
        if (channel->reliable_count > 0) {
            slot = channel->reliable;
            for (i = 0; i < channel->reliable_count; i++) {
                if (slot->pending == 1 && slot->priority == priority) {
                    message_bits = slot->header_bits + slot->body_bits;
                    budget_bits = (rate / 1000) * (int32_t)(now - channel->budget_base_tick) -
                                  channel->send_budget;
                    if (budget_bits != message_bits && budget_bits - message_bits > -1) {
                        free_bits = (channel->retransmit.stream.last_bit -
                                     channel->retransmit.stream.byte_cursor * 8) -
                                    channel->retransmit.stream.bit_cursor + 1;
                        if (message_bits <= free_bits ||
                            network_channel_stream_flush(&channel->retransmit, channel, 0) != 0) {
                            halo::memory::bit_stream_write_bits_chunked(&channel->retransmit.stream, (const uint32_t *)slot->header, slot->header_bits);
                            channel->retransmit.empty = 0;
                            halo::memory::bit_stream_write_bits_chunked(&channel->retransmit.stream, (const uint32_t *)slot->body, slot->body_bits);
                            channel->retransmit.empty = 0;
                        }
                        channel->send_budget = channel->send_budget + message_bits;
                    }
                }
                slot = slot + 1;
            }
        }
    }
    if (channel->reliable_count > 0) {
        slot = channel->reliable;
        for (i = 0; i < channel->reliable_count; i++) {
            slot->pending = 0;
            slot = slot + 1;
        }
    }
}

/**
 * out/phase4/networking_functions.md: "Per-tick service routine for a network
 * channel: updates timing/status flags, scans for and retransmits timed-out reliable messages,
 * and dispatches to the receive or transmit path as needed." channel[1]=+0x004
 * (last_activity_ms), channel[0x2a3]=+0xa8c (flags), channel[0xb]=+0x02c (in.empty),
 * channel[0x158]=+0x560 (out.empty), channel[0x2a0]=+0xa80 (send_budget) all match
 * types/networking.h's network_channel exactly.
 * `timeout_ms` (EAX, kept in ESI): idle timeout added to last_activity_ms; 0 skips the timing block.
 *
 * @address 0x4dd110
 */
char ChannelView::service(int32_t timeout_ms, network_channel **out_new_child)
{
    network_channel *channel = self;
    int32_t now_ms;
    uint32_t flags;

    {

        large_integer counter;
        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    }

    flags = channel->flags;
    channel->flags = flags & 0xffffffdf;
    if (timeout_ms != 0) {
        if ((uint32_t)(channel->last_activity_ms + 5000) < (uint32_t)now_ms) {
            channel->flags = (flags & 0xffffffdf) | k_network_channel_timed_out;
        }
        if ((uint32_t)now_ms <= (uint32_t)(channel->last_activity_ms + timeout_ms)) {
            goto after_timestamp;
        }
        if (network_channel_service_backoff_bypass == 0 &&
            (unknown_00697ed8 * 0x1e < game_time->game_time || network_game_mode == 1)) {
            return 0;
        }
    }
    channel->last_activity_ms = now_ms;
after_timestamp:
    network_channel_scan_retransmit_timeouts(channel);

    if (channel->outgoing.empty == 0) {
        network_channel_stream_flush(&channel->outgoing, channel, 1);
    }
    if (channel->retransmit.empty == 0) {
        network_channel_stream_flush(&channel->retransmit, channel, 0);
    }
    channel->send_budget = 0xe0;
    if (channel->flags & k_network_channel_listening) {
        return network_channel_listen_service(channel, out_new_child);
    }
    if (channel->flags & (k_network_channel_client | k_network_channel_transmit_pending)) {
        return network_channel_transmit(channel);
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md: "If the channel is in an active
 * connected/connecting state, flushes pending output and then performs a follow-up
 * teardown/reconnect step." channel->flags (k_network_channel_client /
 * k_network_channel_transmit_pending), channel->endpoint and network_receive_queue.flags bit0
 * (k... connection-oriented) all match types/networking.h.
 *
 * @address 0x4dd3f0
 */
int32_t ChannelView::service_close_if_disconnected()
{
    network_channel *channel = self;
    uint32_t flags;

    flags = channel->flags;
    if ((flags & (k_network_channel_client | k_network_channel_transmit_pending)) != 0 &&
        channel->endpoint != 0 && (channel->endpoint->flags & 1) != 0) {
        if (flags & (k_network_channel_client | k_network_channel_transmit_pending)) {
            network_channel_transmit(channel);
        }
        network_receive_queue_close_socket(channel->endpoint);
    }
    return 1;
}

/**
 * FIXED in the review pass: like 0x4dd110, this function has a third, stack-passed
 * argument Ghidra dropped -- 0x4dd2ef is `mov edx,[esp+0x18]`, forwarded to
 * network_channel_listen_service.
 *
 * @address 0x4dd240
 */
char ChannelView::service_light(int32_t timeout_ms, network_channel **out_new_child)
{
    network_channel *channel = self;
    large_integer counter;
    int32_t now_ms;
    uint32_t flags;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);

    flags = channel->flags;
    channel->flags = flags & 0xffffffdf;
    if (timeout_ms != 0) {
        if ((uint32_t)(channel->last_activity_ms + 5000) < (uint32_t)now_ms) {
            channel->flags = (flags & 0xffffffdf) | k_network_channel_timed_out;
        }
        if ((uint32_t)now_ms <= (uint32_t)(channel->last_activity_ms + timeout_ms)) {
            goto after_timestamp;
        }
        if (network_channel_service_backoff_bypass == 0 &&
            (unknown_00697ed8 * 0x1e < game_time->game_time || network_game_mode == 1)) {
            return 0;
        }
    }
    channel->last_activity_ms = now_ms;
after_timestamp:
    if (channel->flags & k_network_channel_listening) {
        return network_channel_listen_service(channel, out_new_child);
    }
    if (channel->flags & (k_network_channel_client | k_network_channel_transmit_pending)) {
        return network_channel_transmit(channel);
    }
    return 1;
}

/**
 * out/phase4/networking_functions.md: "Runs the retransmit-timeout scan and
 * pending-flush bookkeeping for a channel without performing an actual receive or transmit
 * pass." Same in.empty/out.empty/send_budget fields as network_channel_service.c. The Ghidra
 * decompile's CONCAT31(uVar1,1) return packs garbage high bytes from extraout_EAX/_var with a
 * fixed low byte of 1; this rewrite just returns 1.
 *
 * @address 0x4dd330
 */
int32_t ChannelView::service_retransmit_only()
{
    network_channel *channel = self;
    large_integer counter;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    network_channel_scan_retransmit_timeouts(channel);
    if (channel->outgoing.empty == 0) {
        network_channel_stream_flush(&channel->outgoing, channel, 1);
    }
    if (channel->retransmit.empty == 0) {
        network_channel_stream_flush(&channel->retransmit, channel, 0);
    }
    channel->send_budget = 0xe0;
    return 1;
}

/**
 * describes this address as "shortens the disconnect timeout when clear"
 *
 * @address 0x4ddd20
 */
int32_t ChannelView::short_disconnect_timeout()
{
    if (network_server != 0 && network_disconnect_timeout_flag == 0) {
        return 1;
    }
    return 0;
}

namespace {

static int32_t transmit_circular_buffer_used(circular_buffer *buffer)
{
    int32_t used = buffer->write_cursor - buffer->read_cursor;
    if (used < 0) {
        used = used + buffer->capacity;
    }
    return used;
}

}

/**
 * VERIFIED against disassembly 0x4dd730..0x4dd927 (2026-09-30)
 *
 * @address 0x4dd730
 */
char ChannelView::transmit()
{
    network_channel *channel = self;
    large_integer counter;
    s_network_address remote_address;
    circular_buffer *destination;
    circular_buffer *source;
    network_receive_queue *queue;
    int32_t free_space;
    int32_t chunk;
    int32_t source_available;
    int32_t count;
    int32_t read_cursor;
    int32_t remaining;
    uint8_t *scratch_cursor;
    char done;
    uint8_t scratch[0x5000];

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    destination = channel->incoming;
    done = 1;
    free_space = destination->capacity - transmit_circular_buffer_used(destination) - 1;

    network_channel_get_remote_address(&remote_address, channel->endpoint);

    for (;;) {
        queue = channel->endpoint;
        if (queue->data_ready != 1 || network_pending_connection_count < 1) {
            source = queue->incoming;
            if (source == 0) {
                break;
            }
            if (transmit_circular_buffer_used(source) < 1) {
                break;
            }
        }
        if (free_space < 1) {
            break;
        }
        chunk = (free_space < 0x5000) ? free_space : 0x5000;

        source = queue->incoming;
        source_available = transmit_circular_buffer_used(source);
        if (queue->connection_failed == 1) {
            if (source_available == 0) {
                channel->flags = channel->flags | k_network_channel_dead;
                done = 0;
                goto refresh;
            }
        } else if (source_available == 0) {
            break;
        }

        count = (source_available > chunk) ? chunk : source_available;

        read_cursor = source->read_cursor;
        scratch_cursor = scratch;
        remaining = count;
        if (count <= transmit_circular_buffer_used(source)) {
            int32_t tail_room = source->capacity - read_cursor;
            if (count >= tail_room) {
                memcpy(scratch, source->data + read_cursor, (uint32_t)tail_room);
                read_cursor = 0;
                scratch_cursor = scratch + tail_room;
                remaining = count - tail_room;
            }
            if (remaining > 0) {
                memcpy(scratch_cursor, source->data + read_cursor, (uint32_t)remaining);
                read_cursor = read_cursor + remaining;
            }
            source->read_cursor = read_cursor;
        }

        if (count > 0) {
            QueryPerformanceCounter((LARGE_INTEGER *)&counter);
            channel->last_activity_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
            halo::memory::circular_buffer_write((uint32_t)count, channel->incoming, scratch);
        } else {
            if (count == -4) {
                break;
            }
            if (count == -3) {
                channel->flags = channel->flags | k_network_channel_dead;
            }
            done = 0;
        }
    refresh:
        destination = channel->incoming;
        free_space = destination->capacity - transmit_circular_buffer_used(destination) - 1;
        if (done == 0) {
            break;
        }
    }
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    return done;
}

/**
 * out/phase4/networking_functions.md: "Releases the channel previously obtained for
 * the given (player,machine) key via player_new_local, recording the returned index at in_EAX+0x1f
 * if valid." Mirrors network_channel_key_open.c.
 *
 * @address 0x4de8c0
 */
int32_t ChannelKeys::close(network_player_entry *entry, datum_index requested_handle)
{
    int16_t key;

    if (network_channel_key_resolve_target(entry) == 0) {
        key = -1;
    } else {
        key = entry->machine_player_index;
    }
    player_new_local(requested_handle, entry->machine_index, key, (uint16_t *)entry);
    if (requested_handle != (datum_index)-1) {
        entry->slot_index = (int8_t)requested_handle;
        return 1;
    }
    return 0;
}

/**
 * out/phase4/networking_functions.md: "Opens/obtains a network channel for the given
 * (player,machine) key via player_new_network, records the resulting index, and notifies network_index_cache_find_or_allocate_slot
 * of the new channel." entry->machine_index/machine_player_index/slot_index match
 * types/networking.h's network_player_entry.
 *
 * @address 0x4de870
 */
int32_t ChannelKeys::open(network_player_entry *entry)
{
    int16_t key;
    int32_t index;

    if (network_channel_key_resolve_target(entry) == 0) {
        key = -1;
    } else {
        key = entry->machine_player_index;
    }
    index = player_new_network(entry->machine_index, key);
    if (index != -1) {
        entry->slot_index = (int8_t)index;
        network_index_cache_find_or_allocate_slot(index);
        return 1;
    }
    return 0;
}

/**
 * FIXED (objdump): every ret sets only AL; the upper bits of EAX are left as they were
 *
 * @address 0x4ddcc0
 */
uint8_t ChannelKeys::resolve_target(network_player_entry *entry)
{
    if (entry == 0 || network_player_entry_validate(entry) == 0) {
        if (network_game_mode != 3) {
            return 1;
        }
        return entry->machine_index == 0;
    }
    if ((network_server == 0 || ((network_server->flags >> 2) & 1) == 0) &&
        (*(int32_t *)network_client != -1 &&
         (int32_t)*(int32_t *)network_client == (int32_t)entry->machine_index)) {
        return 1;
    }
    return 0;
}

/**
 * out/phase4/networking_functions.md: "Builds and sends a large state packet (via
 * FUN_004ec590/network_game_settings_packet_receive) when the connection is mid-game as host or client, otherwise
 * delegates to FUN_004ec670." client->state (state 2 or 3) matches this cluster's established
 * field.
 *
 * @address 0x4de950
 */
int32_t ChannelKeys::send_state(network_client_globals *client, int32_t **entry)
{
    uint8_t scratch[0x3ba];

    if (network_game_mode == 2 || (client->state != 2 && client->state != 3)) {
        message_delta_decode_compound_field_staged(entry);
        return 0;
    }
    if (**entry == 0) {
        memset(scratch, 0, sizeof(scratch));
        if (message_delta_decode_compound_field(entry, scratch) != 0) {
            if (network_game_settings_packet_receive(scratch) != 0) {
                return 1;
            }
            return 0;
        }
    }
    return 0;
}

/**
 * Appends `entry` to the write-cursor slot of `list->entries`, then adds its socket_key to the
 * 64-entry dedup array if not already present and there is room. Always advances last_index
 * and marks the entry as "in a list" (flags bit3). Returns 0 on success, -20 (0xffffffec) if
 * the list's write cursor has reached capacity or would overflow.
 *
 * @address 0x441a40
 */
int32_t ChannelListView::add(network_receive_queue *entry)
{
    network_channel_list *list = self;
    int32_t new_index;
    uint32_t fd_count_snapshot;
    uint32_t dedup_index;
    uint32_t i;

    if (list->capacity - 1 < list->last_index || (new_index = list->last_index + 1, new_index < 0)) {
        return 0xffffffec;
    }
    list->entries[new_index] = entry;
    fd_count_snapshot = list->fd_count;
    if ((entry->flags & 2) == 0) {
        dedup_index = 0;
        if (fd_count_snapshot != 0) {
            for (i = 0; i < list->fd_count; i++) {
                if (list->fd_array[i] == (uint32_t)list->entries[new_index]->socket_key) {
                    break;
                }
                dedup_index = dedup_index + 1;
            }
        }
    } else {
        dedup_index = 0;
        if (fd_count_snapshot != 0) {
            for (i = 0; i < list->fd_count; i++) {
                if (list->fd_array[i] == (uint32_t)list->entries[new_index]->socket_key) {
                    break;
                }
                dedup_index = dedup_index + 1;
            }
        }
    }
    if (dedup_index == fd_count_snapshot && fd_count_snapshot < 0x40) {
        list->fd_array[dedup_index] = (uint32_t)list->entries[new_index]->socket_key;
        list->fd_count = list->fd_count + 1;
    }
    list->last_index = list->last_index + 1;
    entry->flags = entry->flags | 8;
    return 0;
}

/**
 * Marks every entry in `list` whose queue either already reports data_ready (while a
 * connection request is pending) or whose incoming circular buffer has bytes available, by
 * setting bit2 of the entry's flags byte. Returns 0 if at least one entry was marked, else
 * -13 (0xfffffff3).
 *
 * @address 0x4419d0
 */
int32_t ChannelListView::mark_readable()
{
    network_channel_list *list = self;
    int32_t i;
    network_receive_queue *entry;
    circular_buffer *incoming;
    int32_t available;
    int32_t result;

    result = 0xfffffff3;
    if (-1 < list->last_index) {
        for (i = 0; i <= list->last_index; i++) {
            entry = list->entries[i];
            if (entry->data_ready == 1 && 0 < network_pending_connection_count) {
                entry->flags |= 4;
                result = 0;
            } else {
                incoming = entry->incoming;
                if (incoming != 0) {
                    available = incoming->write_cursor - incoming->read_cursor;
                    if (available < 0) {
                        available = available + incoming->capacity;
                    }
                    if (0 < available) {
                        entry->flags |= 4;
                        result = 0;
                    }
                }
            }
        }
    }
    return result;
}

/**
 * Returns 0 on success, -19 (0xffffffed) if the list is empty (last_index < 0) or `entry` is
 * not present in list->entries.
 * FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
 *
 * @address 0x441b00
 */
int32_t ChannelListView::remove(network_receive_queue *entry)
{
    network_channel_list *list = self;
    int32_t found_index;
    uint32_t dedup_index;
    uint32_t i;

    found_index = 0;
    if (list->last_index < 0) {
        return 0xffffffed;
    }
    while (list->entries[found_index] != entry) {
        found_index = found_index + 1;
        if (list->last_index < found_index) {
            return 0xffffffed;
        }
    }
    if (list->fd_count != 0) {
        dedup_index = 0;
        while (list->fd_array[dedup_index] != (uint32_t)entry->socket_key) {
            dedup_index = dedup_index + 1;
            if (list->fd_count <= dedup_index) {
                goto done_dedup;
            }
        }
        if (dedup_index < list->fd_count - 1) {
            for (i = dedup_index; i < list->fd_count - 1; i++) {
                list->fd_array[i] = list->fd_array[i + 1];
            }
        }
        list->fd_count = list->fd_count - 1;
    }
done_dedup:
    entry->flags = entry->flags & 0xf7;
    list->entries[found_index] = list->entries[list->last_index];
    list->entries[list->last_index] = 0;
    list->last_index = list->last_index - 1;
    return 0;
}

/**
 * out/phase4/networking_functions.md: "Register-based (ESI=connection object) helper
 * that computes an unsent byte length from a bit-position ring buffer, flushes it through
 * gt2Send (a socket/channel send), then advances the connection." The `stream+8/+0xc/+0x10/
 * +0x14` fields match bit_stream's first_bit/byte_cursor/bit_cursor/last_bit, and `stream+0x1c`/
 * `stream+0x1d` match network_channel_stream's empty flag and inline data buffer exactly, so ESI
 * is a network_channel_stream* -- one of channel->in or channel->out depending on the caller
 * (unresolvable from this function's own body; see each caller for which one it intends).
 *
 * @address 0x4ddb60
 */
char ChannelStreamView::flush(network_channel *channel, char mode)
{
    network_channel_stream *stream = self;
    uint32_t first_bit;
    uint32_t used_bits;
    uint32_t rem;
    int32_t byte_count;
    int32_t send_result;
    int32_t success;
    int32_t send_mode;

    success = 0;
    first_bit = stream->stream.first_bit;

    used_bits = (stream->stream.bit_cursor + stream->stream.byte_cursor * 8) - first_bit;
    rem = used_bits & 0x80000007;
    if ((int32_t)rem < 0) {
        rem = (rem - 1 | 0xfffffff8) + 1;
    }
    byte_count = (rem != 0) + ((int32_t)(used_bits + ((int32_t)used_bits >> 31 & 7)) >> 3);
    if (first_bit <= stream->stream.last_bit || first_bit == stream->stream.last_bit + 1) {
        stream->stream.byte_cursor = first_bit >> 3;
        stream->stream.bit_cursor = first_bit & 7;

        send_result = halo::memory::bit_stream_write_bits_chunked(&stream->stream, (const uint32_t *)&byte_count, network_bit_chunk_size);
        if (send_result == network_bit_chunk_size) {
            do {
                if (channel->endpoint->connection_failed == 1) {
                    break;
                }
                send_mode = mode != 0;
                byte_count = gt2Send(channel->endpoint->socket, (uint8_t *)stream + 0x1d,
                    byte_count, send_mode);
                if (byte_count > 0) {
                    success = 1;
                    break;
                }
            } while (byte_count == -4);
        }
    }
    first_bit = stream->stream.first_bit;
    stream->empty = 1;
    if (first_bit <= stream->stream.last_bit || first_bit == stream->stream.last_bit + 1) {
        stream->stream.bit_cursor = first_bit & 7;
        stream->stream.byte_cursor = first_bit >> 3;
    }
    {
        uint32_t zero = 0;
        halo::memory::bit_stream_write_bits_chunked(&stream->stream, &zero, network_bit_chunk_size);
    }
    channel->send_budget = channel->send_budget + 0xe0;
    channel->budget_base_tick = GetTickCount();
    return success > 0;
}

/**
 * out/phase4/networking_functions.md: "Initializes a small per-direction bookkeeping
 * record (window sizes, flags) for a newly-created channel and primes the shared output queue
 * with a default byte count." Every field written (data = this+0x1d, last_bit = 0x287f,
 * capacity_bits = 0x2880, empty = 1) matches types/networking.h's network_channel_stream and
 * its k_network_channel_stream_bits constant exactly; network_bit_chunk_size defaults to 11 (0xb)
 * here, matching the header's own note.
 *
 * @address 0x4dd980
 */
void ChannelStreamView::init()
{
    network_channel_stream *stream = self;
    if (network_bit_chunk_size == 0) {
        network_bit_chunk_size = 0xb;
    }
    stream->stream.unknown_00 = 0;
    stream->stream.data = (uint8_t *)stream + 0x1d;
    stream->stream.first_bit = 0;
    stream->stream.byte_cursor = 0;
    stream->stream.bit_cursor = 0;
    stream->stream.last_bit = 0x287f;
    stream->capacity_bits = 0x2880;
    stream->empty = 1;
    {
        uint32_t zero = 0;
        halo::memory::bit_stream_write_bits_chunked(&stream->stream, &zero, network_bit_chunk_size);
    }
}

/**
 * Pops the most recently queued pending connection (if any) and, if the transport accepts it,
 * builds a fresh receive queue for it, flags it connection-oriented, and formats its remote
 * address. Always decrements the pending count when there was an entry. Returns the new queue,
 * or NULL if there was nothing pending, the transport rejected it, or the queue allocation
 * failed.
 *
 * @address 0x4421b0
 */
network_receive_queue * ListenerCallbacks::accept_pending_connection()
{
    network_receive_queue *queue;
    network_pending_connection *entry;
    network_listen_accept_config config;
    int32_t accepted;
    s_network_address remote_address;

    queue = 0;
    if (0 < network_pending_connection_count) {
        entry = &network_pending_connections[network_pending_connection_count - 1];
        config.result = 0;
        config.receive_callback = (void *)network_channel_receive_callback;
        config.error_callback = (void *)network_channel_gap_441f30;
        config.connect_callback = (void *)function_do_nothing;
        accepted = gt2Accept(entry->reply_socket, &config);
        if (accepted == 1) {
            queue = network_receive_queue_new();
            if (queue != 0) {
                queue->socket = entry->reply_socket;
                gt2SetConnectionData(entry->reply_socket, queue);
                queue->flags = queue->flags | 1;
                network_channel_get_remote_address(&remote_address, queue);
                network_address_to_string(&remote_address);
            }
        }
        network_pending_connection_count = network_pending_connection_count - 1;
    }
    return queue;
}

/**
 * 0x16-byte address record (stride confirmed by the imul esi,esi,0x16 at 0x6148c8) // foreign
 * Refuses a new request once 30 are already queued, or if the payload is missing/too short
 * (fewer than 4 bytes). Otherwise records the request in the next pending-connection slot.
 *
 * @address 0x442090
 */
void ListenerCallbacks::connection_request_handler(int32_t listen_handle, int32_t reply_socket, uint32_t remote_address, uint32_t remote_port_raw, int32_t transport_handle, uint32_t *payload, uint32_t payload_length)
{
    int32_t reply_code;
    uint32_t first_payload_word;
    int32_t index;
    uint8_t address_buf[24];

    reply_code = 0;
    if (k_network_pending_connection_count < network_pending_connection_count) {
        reply_code = k_network_listen_error_queue_full;
        gt2Reject(reply_socket, &reply_code, 4);
        return;
    }
    if (payload != 0 && 3 < payload_length) {
        first_payload_word = *payload;
        index = network_pending_connection_count;
        network_pending_connections[index].reply_socket = reply_socket;
        network_pending_connections[index].transport_handle = transport_handle;
        network_pending_connections[index].remote_address = remote_address;
        network_pending_connections[index].remote_port = (uint16_t)remote_port_raw;
        network_pending_connections[index].first_payload_word = first_payload_word;
        network_pending_connection_count = network_pending_connection_count + 1;
        gt2GetSocketData(listen_handle);
        gt2AddressToString(remote_address, (uint16_t)remote_port_raw, address_buf);
        if (reply_code != 0) {
            gt2Reject(reply_socket, &reply_code, 4);
        }
        return;
    }
    reply_code = k_network_listen_error_bad_payload;
    gt2Reject(reply_socket, &reply_code, 4);
}

/**
 * out/phase4/networking_functions.md summary ("cancels (rejects) the most recently
 * queued pending incoming connection request without accepting it"); mirrors
 * network_listen_accept_pending_connection.c's `network_pending_connections[count - 1]`
 * derivation from the same `(&DAT_0087bc0c)[count*5]` dword-array indexing.
 *
 * @address 0x442250
 */
uint32_t ListenerCallbacks::reject_pending_connection(int32_t reject_code)
{
    if (0 < network_pending_connection_count) {
        gt2Reject(network_pending_connections[network_pending_connection_count - 1].reply_socket,
                            &reject_code, 4);
        network_pending_connection_count = network_pending_connection_count - 1;
    }
    return (uint32_t)network_pending_connection_count & 0xffff0000;
}

}
