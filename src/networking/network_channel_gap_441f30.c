// network_channel_gap_441f30  (not a Ghidra function; no C existed)
// address 0x441f30, size 35 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x441f30..0x441f52: the GT2 connection callback set by
//   network_listen_accept_pending_connection (config.error_callback): the connection's receive queue
//   (gt2GetConnectionData) gets byte +5 = 1, network_receive_queue_close_socket (ESI queue) and flag 0x40 in +0x0c.
//   (The name keeps the one its registrant uses.)
// blam-cc: cdecl (a GT2 connection callback)

#include "tags.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
typedef struct network_receive_queue network_receive_queue;
extern void *gt2GetConnectionData(void *connection); // 0x614840 gt2GetConnectionData
extern void network_receive_queue_close_socket(network_receive_queue *queue); // 0x442040, blam-cc: ESI queue

void network_channel_gap_441f30(void *connection)
{
    uint8_t *queue = (uint8_t *)gt2GetConnectionData(connection);

    if (queue != 0) {
        queue[0x05] = 1;
        network_receive_queue_close_socket((network_receive_queue *)queue);
        queue[0x0c] |= 0x40;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
