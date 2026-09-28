// SBListThink  (GameSpy SDK in halo.exe; no C existed)
// address 0x620270, size 53 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620270..0x6202a4: frees the dead servers; LAN browsing reads LAN replies,
//   connected or main-list states read the master, others do nothing.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

int SBListThink(SBServerList *slist)
{
    SBFreeDeadList(slist);
    if (slist->state == 0) {
        return ProcessLanData(slist);
    }
    if (slist->state > 1 && slist->state <= 3) {
        return SBListThinkConnected(slist);
    }
    return 0;
}
