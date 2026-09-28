// gti2FreeClosedConnections  (GameSpy SDK in halo.exe; no C existed)
// address 0x61cb60, size 50 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61cb60..0x61cb91: gti2FreeClosedConnection on every closed connection, last
//   first.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

void gti2FreeClosedConnections(GTI2Socket *socket)
{
    int i;

    for (i = ArrayLength(socket->closedConnections) - 1; i >= 0; i--) {
        gti2FreeClosedConnection(*(GTI2Connection **)ArrayNth(socket->closedConnections, i));
    }
}
