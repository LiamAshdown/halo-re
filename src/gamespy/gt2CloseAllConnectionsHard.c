// gt2CloseAllConnectionsHard  (GameSpy SDK in halo.exe; no C existed)
// address 0x614780, size 24 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x614780..0x614797: TableMapSafe over the connections with
//   gti2CloseConnectionHardMap 0x614760.
// blam-cc: cdecl

#include "gamespy.h"

#include "gt2.h"

extern void gti2CloseConnectionHardMap(void *elem, void *client_data);

void gt2CloseAllConnectionsHard(GTI2Socket *socket)
{
    TableMapSafe(socket->connections, gti2CloseConnectionHardMap, 0);
}
