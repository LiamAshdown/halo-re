#include "halo/networking/net1_channel.hpp"

extern "C" {
extern int32_t network_query_socket;
extern int32_t network_game_socket;
extern int16_t network_join_error_code;
extern uint8_t network_host_handoff_requested;
extern void network_channels_open(void);
extern void chat_close(void);
extern int gt2NetworkToHostInt(unsigned int value);
extern char *gt2AddressToString(unsigned int ip, unsigned short port, char *string);
extern int gt2Connect(void *socket, void **connection_out, const char *remote_address, const unsigned char *message, int len, unsigned long timeout, const void *callbacks, int blocking);
extern void gt2SetConnectionData(void *connection, void *data);
extern int32_t network_connect_timeout_ms;
extern void network_channel_connected_callback(void *connection, int32_t result, const uint8_t *message, int32_t length);
extern void network_channel_receive_callback(void *handle, uint8_t *data, int32_t length);
extern void network_channel_gap_441f30(void *connection);
extern void function_do_nothing(void);
extern int32_t network_pending_connection_count;
extern network_pending_connection network_pending_connections[k_network_pending_connection_count];
extern void gt2Reject(void *connection, const unsigned char *message, int len);
}

namespace halo::networking {

/**
 * FIXED 2026-09-28 (networking call audit, from the disassembly 0x441f60..0x442036): gt2Connect takes eight
 * arguments -- the socket, the queue (whose +0 receives the connection), the formatted remote address, a 4-byte
 * message that is this function's third argument (its address, length 4), the connect timeout (0x6894ac), a local
 * GT2ConnectionCallbacks {connected 0x441e00, received 0x441ed0, closed 0x441f30, ping 0x44ad80} and 0 (not
 * blocking); the previous C passed only the socket. On success the connection's data is the queue.
 * unused_param_1/use_query_socket are ordinary stack parameters
 * Picks the query or game socket (use_query_socket != 0 selects query), and if that socket
 * exists and reports a successful connect (FUN_006145a0 returns 0), clears the queue's
 * disconnect flag and error. Otherwise marks the queue disconnected, arms the host-handoff
 * retry state, closes chat, and reports error 0xfff0.
 *
 * @address 0x441f60
 */
int16_t ReceiveQueueView::attempt_connect(s_network_address *address, int32_t unused_param_1, uint8_t use_query_socket)
{
    network_receive_queue *queue = self;
    uint32_t formatted_address;
    int32_t socket;
    int32_t connect_result;
    char address_buf[24];
    void *callbacks[4];

    formatted_address = (uint32_t)gt2NetworkToHostInt(address->ipv4);
    gt2AddressToString(formatted_address, address->port, address_buf);
    callbacks[0] = (void *)network_channel_connected_callback;
    callbacks[1] = (void *)network_channel_receive_callback;
    callbacks[2] = (void *)network_channel_gap_441f30;
    callbacks[3] = (void *)function_do_nothing;
    network_channels_open();
    socket = network_query_socket;
    if (use_query_socket == 0) {
        socket = network_game_socket;
    }
    if (socket != 0) {
        connect_result = gt2Connect((void *)socket, (void **)&queue->socket, address_buf,
                                    (const unsigned char *)&unused_param_1, 4, (unsigned long)network_connect_timeout_ms,
                                    callbacks, 0);
        if (connect_result == 0) {
            queue->connection_failed = 0;
            gt2SetConnectionData((void *)queue->socket, queue);
            queue->last_error = 0;
            return 0;
        }
    }
    queue->connection_failed = 1;
    if (network_join_error_code == -1) {
        network_join_error_code = 7;
    }
    network_host_handoff_requested = 1;
    chat_close();
    queue->last_error = k_network_error_connect_failed;
    return k_network_error_connect_failed;
}

/**
 * WRITTEN 2026-09-28 (retail-independence loop) from the disassembly 0x4e1410..0x4e1447. Reached only as an
 * immediate: network_game_server_host_new (0x4dec40) stores it as the host session's message_callback, which the C
 * did as the literal retail address 0x4e1410. The same work as network_listen_reject_pending_connection (0x442250)
 * with the reject code as the second argument: while a connection request is pending (count 0x6f16d0), the newest
 * one (network_pending_connections[count - 1], 0x87bc20) is rejected with the 4-byte code (gt2Reject through its
 * thunk 0x614590) and the count drops by one. Returns the count as the original leaves it in EAX.
 *
 * @address 0x4e1410
 */
int32_t ListenerCallbacks::reject_pending_connection_callback(void *unused, int32_t reject_code)
{
    (void)unused;
    if (network_pending_connection_count > 0) {
        gt2Reject((void *)network_pending_connections[network_pending_connection_count - 1].reply_socket,
                  (const unsigned char *)&reject_code, 4);
        network_pending_connection_count = network_pending_connection_count - 1;
    }
    return network_pending_connection_count;
}

}
