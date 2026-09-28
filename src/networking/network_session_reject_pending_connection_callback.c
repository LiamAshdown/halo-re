// network_session_reject_pending_connection_callback  (not a Ghidra function)
// address 0x4e1410, size 55 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 (retail-independence loop) from the disassembly 0x4e1410..0x4e1447. Reached only as an
// immediate: network_game_server_host_new (0x4dec40) stores it as the host session's message_callback, which the C
// did as the literal retail address 0x4e1410. The same work as network_listen_reject_pending_connection (0x442250)
// with the reject code as the second argument: while a connection request is pending (count 0x6f16d0), the newest
// one (network_pending_connections[count - 1], 0x87bc20) is rejected with the 4-byte code (gt2Reject through its
// thunk 0x614590) and the count drops by one. Returns the count as the original leaves it in EAX.
// blam-cc: cdecl (a callback: unused, reject code)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t network_pending_connection_count; // 0x006f16d0
extern network_pending_connection network_pending_connections[k_network_pending_connection_count]; // 0x0087bc20
extern void gt2Reject(void *connection, const unsigned char *message, int len); // 0x61cee0 (the original calls its thunk)

int32_t network_session_reject_pending_connection_callback(void *unused, int32_t reject_code)
{
    (void)unused;
    if (network_pending_connection_count > 0) {
        gt2Reject((void *)network_pending_connections[network_pending_connection_count - 1].reply_socket,
                  (const unsigned char *)&reject_code, 4);
        network_pending_connection_count = network_pending_connection_count - 1;
    }
    return network_pending_connection_count;
}
