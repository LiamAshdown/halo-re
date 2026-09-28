// gti2SocketConnectionsThink  (GameSpy SDK in halo.exe; no C existed)
// address 0x61cb30, size 40 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61cb30..0x61cb57: TableMapSafe2 over the connections with the time now and the
//   static 0x61cae0 (a connection not closed thinks, stopping the map when that fails; one that is closed and not
//   held is freed); 1 when every connection went through.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

static int gti2ConnectionThinkMap(void *elem, void *client_data) // 0x61cae0
{
    GTI2Connection *connection = *(GTI2Connection **)elem;
    unsigned long now = *(unsigned long *)client_data;

    if (connection->state != GTI2Closed && !gti2ConnectionThink(connection, now)) {
        return 0;
    }
    if (connection->state == GTI2Closed && connection->freeAtAcceptReject == 0 && connection->callbackLevel == 0) {
        gti2FreeClosedConnection(connection);
    }
    return 1;
}

int gti2SocketConnectionsThink(GTI2Socket *socket)
{
    unsigned long now = current_time();

    return TableMapSafe2(socket->connections, gti2ConnectionThinkMap, &now) == 0;
}
