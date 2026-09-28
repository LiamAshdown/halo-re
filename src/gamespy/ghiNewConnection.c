// ghiNewConnection  (GameSpy SDK in halo.exe; no C existed)
// address 0x620c10, size 307 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620c10..0x620d42: under the lock: a free slot zeroed and in use, the next
//   unique id, no socket, total size -1, a 2 KB send buffer (grow 4 KB) and a 2 KB receive buffer; NULL (freed again)
//   when anything fails.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

extern GHIConnection **ghiConnections;   // 0x006a3284
extern int ghiConnectionsLen;             // 0x006a327c
extern int ghiNumConnections;             // 0x006a3280

extern int ghiNextUniqueID;               // 0x006a3288

GHIConnection *ghiNewConnection(void)
{
    GHIConnection *connection;
    int index;

    ghiLock();
    index = ghiFindFreeSlot();
    if (index == -1) {
        ghiUnlock();
        return 0;
    }
    connection = ghiConnections[index];
    memset(connection, 0, 0x11c);
    connection->uniqueID = ghiNextUniqueID++;
    connection->inUse = 1;
    connection->request = index;
    connection->socket = INVALID_SOCKET;
    connection->totalSize = -1;
    if (ghiInitBuffer(connection, &connection->sendBuffer, 0x800, 0x1000) &&
        ghiInitBuffer(connection, &connection->recvBuffer, 0x800, 0x800)) {
        ghiNumConnections++;
        ghiUnlock();
        return connection;
    }
    ghiFreeConnection(connection);
    ghiUnlock();
    return 0;
}
