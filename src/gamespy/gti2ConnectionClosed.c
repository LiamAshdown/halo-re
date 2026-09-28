// gti2ConnectionClosed  (GameSpy SDK in halo.exe; no C existed)
// address 0x61cff0, size 65 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61cff0..0x61d030: once: state 7, out of the connection table and onto the
//   socket  closed list.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

void gti2ConnectionClosed(GTI2Connection *connection)
{
    if (connection->state == GTI2Closed) {
        return;
    }
    connection->state = GTI2Closed;
    TableRemove(connection->socket->connections, &connection);
    ArrayAppend(connection->socket->closedConnections, &connection);
}
