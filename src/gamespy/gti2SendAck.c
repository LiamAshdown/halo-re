// gti2SendAck  (GameSpy SDK in halo.exe; no C existed)
// address 0x618a70, size 108 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x618a70..0x618adb: fe fe 64 and the expected serial (high byte first); the
//   pending ack is cleared once it went.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

int gti2SendAck(GTI2Connection *connection)
{
    unsigned char packet[5];

    packet[0] = 0xfe;
    packet[1] = 0xfe;
    packet[2] = GTI2MsgAck;
    packet[3] = (unsigned char)(connection->expectedSerialNumber >> 8);
    packet[4] = (unsigned char)connection->expectedSerialNumber;
    if (!gti2ConnectionSendData(connection, packet, 5, 0, 0)) {
        return 0;
    }
    connection->pendingAck = 0;
    return 1;
}
