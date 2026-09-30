// network_channel_gap_441040  (not a Ghidra function; no C existed; named as its registrant names it)
// address 0x441040, size 23 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x441040..0x441056: the GT2 send dump callback set by network_channels_open:
//   records the packet with is_sent, reliable and resend all 0.
// blam-cc: cdecl (a GT2 dump callback)

#include "tags.h"
#include "fn_networking.h"
#include <string.h>


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
