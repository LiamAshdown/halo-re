// ghiEnumConnections  (GameSpy SDK in halo.exe; no C existed)
// address 0x620aa0, size 69 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620aa0..0x620ae4: when any are live: the callback on every live connection,
//   under the lock.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

extern GHIConnection **ghiConnections;   // 0x006a3284
extern int ghiConnectionsLen;             // 0x006a327c
extern int ghiNumConnections;             // 0x006a3280

void ghiEnumConnections(int (*callback)(GHIConnection *connection))
{
    int i;

    if (ghiNumConnections <= 0) {
        return;
    }
    ghiLock();
    for (i = 0; i < ghiConnectionsLen; i++) {
        if (ghiConnections[i]->inUse != 0) {
            callback(ghiConnections[i]);
        }
    }
    ghiUnlock();
}
