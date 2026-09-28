// SBServerListDisconnect  (GameSpy SDK in halo.exe; no C existed)
// address 0x61f500, size 90 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61f500..0x61f559: frees the input buffer, closes the master socket, back to
//   disconnected (1), key list and popular values released.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

void SBServerListDisconnect(SBServerList *slist)
{
    if (slist->inbuffer != 0) {
        free(slist->inbuffer);
    }
    slist->inbuffer = 0;
    slist->inbufferlen = 0;
    if (slist->slsocket != INVALID_SOCKET) {
        closesocket(slist->slsocket);
    }
    slist->slsocket = INVALID_SOCKET;
    slist->state = 1;
    FreeKeyList(slist);
    slist->expectedelements = -1;
    FreePopularValues(slist);
}
