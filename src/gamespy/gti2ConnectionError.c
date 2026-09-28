// gti2ConnectionError  (GameSpy SDK in halo.exe; no C existed)
// address 0x6183b0, size 101 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6183b0..0x618414: ESI connection, stack result and reason. Before connected: an
//   initiated connection closes and the connected callback hears the result (0 when that freed the socket); an
//   incoming one closes (marked freeAtAcceptReject when it was awaiting accept/reject). Connected or closing: closes
//   and the closed callback hears the reason. Already closed: nothing. Otherwise 1.
// blam-cc: ESI -> connection, stack -> result, reason

#include "gamespy.h"

#include "gt2.h"

int gti2ConnectionError(GTI2Connection *connection, int result, int reason)
{
    if (connection->state < GTI2Connected) {
        if (connection->initiated != 0) {
            gti2ConnectionClosed(connection);
            return gti2ConnectedCallback(connection, result, 0, 0) != 0;
        }
        if (connection->state == GTI2AwaitingAcceptReject) {
            connection->freeAtAcceptReject = 1;
        }
        gti2ConnectionClosed(connection);
        return 1;
    }
    if (connection->state == GTI2Closed) {
        return 1;
    }
    gti2ConnectionClosed(connection);
    return gti2ClosedCallback(connection, reason) != 0;
}
