// network_channel_gap_441040  (not a Ghidra function; no C existed; named as its registrant names it)
// address 0x441040, size 23 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x441040..0x441056: the GT2 send dump callback set by network_channels_open:
//   records the packet with is_sent, reliable and resend all 0.
// blam-cc: cdecl (a GT2 dump callback)

#include "tags.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void network_connection_stats_record_packet(void *gamespy_connection, int32_t payload_length, uint8_t is_sent,
    uint8_t is_reliable, uint8_t is_resend); // 0x440b20, blam-cc: EAX connection, ECX length

void network_channel_gap_441040(void *socket, void *connection, uint32_t ip, uint16_t port, int32_t reset, const void *message,
    int32_t length)
{
    (void)socket;
    (void)ip;
    (void)port;
    (void)reset;
    (void)message;
    network_connection_stats_record_packet(connection, length, 0, 0, 0);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
