// gti2CloseConnection  (GameSpy SDK in halo.exe; no C existed)
// address 0x61d1b0, size 71 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61d1b0..0x61d1f6: hard: an open connection is closed at once, the peer told,
//   the closed callback hears 0 (local close) and it is freed when not held. Soft: it goes to closing (6) and a close
//   message is sent.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

void gti2CloseConnection(GTI2Connection *connection, int hard)
{
    if (hard != 0) {
        if (connection->state < GTI2Closed) {
            gti2ConnectionClosed(connection);
            gti2ConnectionSendClosed(connection);
            gti2ClosedCallback(connection, 0);
            gti2FreeClosedConnection(connection);
        }
        return;
    }
    connection->state = GTI2Closing;
    gti2SendClose(connection);
}
