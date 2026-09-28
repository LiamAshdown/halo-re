// ServerBrowserThink  (GameSpy SDK in halo.exe; no C existed)
// address 0x616f80, size 25 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x616f80..0x616f98: engine think, then list think (its result).
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

int ServerBrowserThink(ServerBrowser *sb)
{
    SBQueryEngineThink(&sb->engine);
    return SBListThink(&sb->list);
}
