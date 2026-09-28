// gti2HandleReject  (GameSpy SDK in halo.exe; no C existed)
// address 0x619040, size 92 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x619040..0x61909b: EAX connection, stack message, len: only while awaiting
//   acceptance (else a negotiation error); closes, tells the peer it is closed and the connected callback hears
//   rejected (2) with the message.
// blam-cc: EAX -> connection, stack -> message, len

#include "gamespy.h"

#include "gt2.h"

int gti2HandleReject(GTI2Connection *connection, const unsigned char *message, int len)
{
    if (connection->state != GTI2AwaitingAcceptance) {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    gti2ConnectionClosed(connection);
    if (!gti2SendClosed(connection->socket, connection->ip, connection->port)) {
        return 0;
    }
    return gti2ConnectedCallback(connection, 2, message, len) != 0;
}
