// network_channel_gap_441020  (not a Ghidra function; no C existed; named as its registrant names it)
// address 0x441020, size 29 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x441020..0x44103c: the GT2 receive dump callback set by network_channels_open
//   (socket, connection, ip, port, reset, message, length, reliable, resend): records the packet with is_sent 1 and
//   the last two arguments as reliable / resend.
// blam-cc: cdecl (a GT2 dump callback)

#include "tags.h"
#include <string.h>

extern void network_connection_stats_record_packet(void *gamespy_connection, int32_t payload_length, uint8_t is_sent,
    uint8_t is_reliable, uint8_t is_resend); // 0x440b20, blam-cc: EAX connection, ECX length

void network_channel_gap_441020(void *socket, void *connection, uint32_t ip, uint16_t port, int32_t reset, const void *message,
    int32_t length, int32_t reliable, int32_t resend)
{
    (void)socket;
    (void)ip;
    (void)port;
    (void)reset;
    (void)message;
    network_connection_stats_record_packet(connection, length, 1, (uint8_t)reliable, (uint8_t)resend);
}
