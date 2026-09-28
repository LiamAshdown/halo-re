// ServerBrowserFree  (GameSpy SDK in halo.exe; no C existed)
// address 0x616f30, size 31 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x616f30..0x616f4e: list cleanup, engine cleanup, free.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

void ServerBrowserFree(ServerBrowser *sb)
{
    SBServerListCleanup(&sb->list);
    SBEngineCleanup(&sb->engine);
    free(sb);
}
