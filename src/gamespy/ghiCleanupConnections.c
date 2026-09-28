// ghiCleanupConnections  (GameSpy SDK in halo.exe; no C existed)
// address 0x620ba0, size 108 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620ba0..0x620c0b: frees every live connection, then every slot and the table.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

extern GHIConnection **ghiConnections;   // 0x006a3284
extern int ghiConnectionsLen;             // 0x006a327c
extern int ghiNumConnections;             // 0x006a3280

static int ghiFreeConnectionEnum(GHIConnection *connection)
{
    return ghiFreeConnection(connection);
}

void ghiCleanupConnections(void)
{
    int i;

    if (ghiConnections == 0) {
        return;
    }
    ghiEnumConnections(ghiFreeConnectionEnum);
    for (i = 0; i < ghiConnectionsLen; i++) {
        free(ghiConnections[i]);
    }
    free(ghiConnections);
    ghiConnections = 0;
    ghiConnectionsLen = 0;
    ghiNumConnections = 0;
}
