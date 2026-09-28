// ServerBrowserClear  (GameSpy SDK in halo.exe; no C existed)
// address 0x616fc0, size 33 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x616fc0..0x616fe0: disconnects, halts and clears the server list.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

void ServerBrowserClear(ServerBrowser *sb)
{
    SBServerListDisconnect(&sb->list);
    SBEngineHaltUpdates(&sb->engine);
    SBServerListClear(&sb->list);
}
