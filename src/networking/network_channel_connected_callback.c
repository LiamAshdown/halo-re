// network_channel_connected_callback  (no prior name; the GT2 "connected" callback)
// address 0x441e00, size 201 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 (retail-independence loop) from the disassembly 0x441e00..0x441ec9. It had no C because it is
// reached only as an immediate: network_channel_attempt_connect (0x441f60) stores it as the `connected` member of
// the GT2ConnectionCallbacks it hands gt2Connect (with network_channel_receive_callback 0x441ed0,
// network_channel_gap_441f30 and function_do_nothing 0x44ad80). gt2 calls it with (connection, result, message,
// length) once the connect attempt resolves: the connection's data is the channel's receive queue
// (gt2GetConnectionData); the remote address is resolved and formatted (for the log); then
//   result 0 (connected): no error, flags |= 0x31 (connection oriented, readable, ...)
//   result 2 (rejected): +0x18 = the reject reason from a 4-byte message when it is 1..8 (else 1), +0x05 = 1, error
//                        -0x18, socket closed, flags |= 0x40
//   result 6:            error -0x10, socket closed, flags |= 0x80
//   anything else:       error -0x10, socket closed.
// blam-cc: cdecl (a GT2 callback)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *gt2GetConnectionData(void *connection); // 0x614840, cdecl
extern int16_t network_channel_get_remote_address(s_network_address *address, network_receive_queue *queue); // 0x441ce0, ESI, EDI
extern char *network_address_to_string(s_network_address *addr); // 0x440570, EAX
extern void network_receive_queue_close_socket(network_receive_queue *queue); // 0x442040, ESI

void network_channel_connected_callback(void *connection, int32_t result, const uint8_t *message, int32_t length)
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
