// ServerBrowserCount  (GameSpy SDK in halo.exe; no C existed)
// address 0x617030, size 16 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617030..0x61703f: SBServerListCount of the server list (+0x48).
// blam-cc: cdecl

#include "gamespy.h"
#include "fn_gamespy.h"


int ServerBrowserCount(void *sb)
{
    return SBServerListCount((char *)sb + 0x48);
}
