// gti2CheckTimeout  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d0d0, size 82 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d0d0..0x61d121: EAX now, ESI connection: a connection still being set up
//   (state below 5) times out -- an initiated one after its timeout (never with 0), an incoming one still before
//   accept/reject after 60 s: the peer is told it is closed, it is closed and the connected callback hears 6 (timed
//   out); 0 when that freed the socket.
// blam-cc: EAX -> now, ESI -> connection

#include "gamespy.h"

#include "gt2.h"

int gti2CheckTimeout(GTI2Connection *connection, unsigned long now)
{
    if (connection->state >= GTI2Connected) {
        return 1;
    }
    if (connection->initiated != 0) {
        if (connection->timeout == 0 || now - connection->startTime <= connection->timeout) {
            return 1;
        }
    } else {
        if (connection->state >= GTI2AwaitingAcceptReject || now - connection->startTime <= 60000) {
            return 1;
        }
    }
    gti2ConnectionSendClosed(connection);
    gti2ConnectionClosed(connection);
    return gti2ConnectedCallback(connection, 6, 0, 0) != 0;
}
