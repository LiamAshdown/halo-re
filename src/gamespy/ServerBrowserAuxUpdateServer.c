// ServerBrowserAuxUpdateServer  (GameSpy SDK in halo.exe; no C existed)
// address 0x617290, size 162 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617290..0x617331: (browser, server, async, full update): updates held
//   meanwhile. A LAN server (flag 1) is queried directly (out of the queues, to the front, full when asked); others
//   are requested through the master. Synchronous and without error: waits for that server as the trigger. The
//   result.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

int ServerBrowserAuxUpdateServer(ServerBrowser *sb, SBServer *server, int async, int fullUpdate)
{
    int error = 0;
    int viaMaster;

    sb->dontUpdate = 1;
    if (server->flags & 1) {
        SBQueryEngineRemoveServerFromFIFOs(&sb->engine, server);
        SBQueryEngineUpdateServer(&sb->engine, server, 1, fullUpdate != 0);
        viaMaster = 0;
    } else {
        error = SBRequestServerUpdate(&sb->list, server->publicip, server->publicport);
        viaMaster = 1;
    }
    if (async == 0 && error == 0) {
        sb->triggerIP = server->publicip;
        sb->triggerPort = server->publicport;
        error = WaitForTriggerUpdate(sb, viaMaster);
    }
    sb->dontUpdate = 0;
    return error;
}
