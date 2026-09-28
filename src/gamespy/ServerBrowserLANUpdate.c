// ServerBrowserLANUpdate  (GameSpy SDK in halo.exe; no C existed)
// address 0x6171d0, size 102 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6171d0..0x617235: (browser, async, start port, end port): halts everything and
//   broadcasts for LAN servers with the engine  query version; synchronous: thinks every 10 ms while LAN browsing or
//   queries are out and nothing failed.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

int ServerBrowserLANUpdate(ServerBrowser *sb, int async, unsigned short startSearchPort, unsigned short endSearchPort)
{
    int error = 0;

    SBServerListDisconnect(&sb->list);
    SBEngineHaltUpdates(&sb->engine);
    SBServerListGetLANList(&sb->list, startSearchPort, endSearchPort, sb->engine.queryversion);
    if (async == 0) {
        while (sb->list.state == 0 || (sb->engine.querylist.count > 0 && error == 0)) {
            msleep(10);
            SBQueryEngineThink(&sb->engine);
            error = SBListThink(&sb->list);
        }
    }
    return error;
}
