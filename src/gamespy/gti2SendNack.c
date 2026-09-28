// gti2SendNack  (GameSpy SDK in halo.exe; no C existed)
// address 0x618ae0, size 103 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618ae0..0x618b46: fe fe 65, the first missing serial and, when the range is
//   longer than one, the last (high bytes first).
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2SendNack(GTI2Connection *connection, unsigned short from, unsigned short to)
{
    unsigned char packet[7];
    int len = 5;

    packet[0] = 0xfe;
    packet[1] = 0xfe;
    packet[2] = GTI2MsgNack;
    packet[3] = (unsigned char)(from >> 8);
    packet[4] = (unsigned char)from;
    if (from != to) {
        packet[5] = (unsigned char)(to >> 8);
        packet[6] = (unsigned char)to;
        len = 7;
    }
    return gti2ConnectionSendData(connection, packet, len, 0, 0) != 0;
}
