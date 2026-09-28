// SBServerListCleanup  (GameSpy SDK in halo.exe; no C existed)
// address 0x61f560, size 51 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61f560..0x61f592: disconnect, clear, drop the ref-string table, free the server
//   array.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

void SBServerListCleanup(SBServerList *slist)
{
    SBServerListDisconnect(slist);
    SBServerListClear(slist);
    SBRefStrHashCleanup();
    if (slist->servers != 0) {
        ArrayFree(slist->servers);
    }
    slist->servers = 0;
}
