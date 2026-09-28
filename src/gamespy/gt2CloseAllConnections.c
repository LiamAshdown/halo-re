// gt2CloseAllConnections  (GameSpy SDK in halo.exe; no C existed)
// address 0x614740, size 24 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x614740..0x614757: TableMapSafe over the connections with 0x614720 (a static
//   here: gti2CloseConnection(*elem, 0), the soft close).
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

static void gti2CloseConnectionSoftMap(void *elem, void *client_data) // 0x614720
{
    (void)client_data;
    gti2CloseConnection(*(GTI2Connection **)elem, 0);
}

void gt2CloseAllConnections(GTI2Socket *socket)
{
    TableMapSafe(socket->connections, gti2CloseConnectionSoftMap, 0);
}
