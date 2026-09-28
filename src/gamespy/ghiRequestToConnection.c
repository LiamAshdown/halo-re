// ghiRequestToConnection  (GameSpy SDK in halo.exe; no C existed)
// address 0x620a60, size 55 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620a60..0x620a96: the live connection with that index (under the lock), else
//   NULL.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

extern GHIConnection **ghiConnections;   // 0x006a3284
extern int ghiConnectionsLen;             // 0x006a327c
extern int ghiNumConnections;             // 0x006a3280

GHIConnection *ghiRequestToConnection(int request)
{
    GHIConnection *connection;

    ghiLock();
    if (request < 0 || request >= ghiConnectionsLen) {
        ghiUnlock();
        return 0;
    }
    connection = ghiConnections[request];
    if (connection->inUse == 0) {
        connection = 0;
    }
    ghiUnlock();
    return connection;
}
