// gt2CloseSocket  (GameSpy SDK in halo.exe; no C existed)
// address 0x614860, size 36 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x614860..0x614883: for a socket: hard-closes every connection (TableMapSafe with
//   0x614760), then gti2FreeSocket.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

extern void gti2CloseConnectionHardMap(void *elem, void *client_data);

void gt2CloseSocket(GTI2Socket *socket)
{
    if (socket == 0) {
        return;
    }
    TableMapSafe(socket->connections, gti2CloseConnectionHardMap, 0);
    gti2FreeSocket(socket);
}
