// ghiFindFreeSlot  (GameSpy SDK in halo.exe; no C existed)
// address 0x6208a0, size 164 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6208a0..0x620943: the first unused connection, else the table grows by 4 fresh
//   0x11c-byte connections (the first new index); -1 when out of memory (new ones freed again).
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

extern GHIConnection **ghiConnections;   // 0x006a3284
extern int ghiConnectionsLen;             // 0x006a327c
extern int ghiNumConnections;             // 0x006a3280

int ghiFindFreeSlot(void)
{
    GHIConnection **grown;
    int oldLen = ghiConnectionsLen;
    int newLen;
    int i;

    for (i = 0; i < ghiConnectionsLen; i++) {
        if (ghiConnections[i]->inUse == 0) {
            return i;
        }
    }
    newLen = ghiConnectionsLen + 4;
    grown = (GHIConnection **)realloc(ghiConnections, newLen * 4);
    if (grown == 0) {
        return -1;
    }
    ghiConnections = grown;
    for (i = oldLen; i < newLen; i++) {
        ghiConnections[i] = (GHIConnection *)malloc(0x11c);
        if (ghiConnections[i] == 0) {
            for (i--; i >= oldLen; i--) {
                free(ghiConnections[i]);
            }
            return -1;
        }
        ghiConnections[i]->inUse = 0;
    }
    ghiConnectionsLen = newLen;
    return oldLen;
}
