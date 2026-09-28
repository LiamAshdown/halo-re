// gti2FreeClosedConnection  (GameSpy SDK in halo.exe; no C existed)
// address 0x61ca60, size 127 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61ca60..0x61cade: unless held (freeAtAcceptReject or a callback level): a
//   closed connection is found in the socket  closed list and deleted there (its free function frees it); any other
//   is removed from the connection table.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

void gti2FreeClosedConnection(GTI2Connection *connection)
{
    GTI2Socket *socket;
    int count;
    int i;

    if (connection->freeAtAcceptReject != 0 || connection->callbackLevel != 0) {
        return;
    }
    socket = connection->socket;
    if (connection->state != GTI2Closed) {
        TableRemove(socket->connections, &connection);
        return;
    }
    count = ArrayLength(socket->closedConnections);
    for (i = 0; i < count; i++) {
        if (connection == *(GTI2Connection **)ArrayNth(connection->socket->closedConnections, i)) {
            ArrayDeleteAt(connection->socket->closedConnections, i);
            return;
        }
    }
}
