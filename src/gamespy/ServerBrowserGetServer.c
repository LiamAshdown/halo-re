// ServerBrowserGetServer  (GameSpy SDK in halo.exe; no C existed)
// address 0x617020, size 16 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617020..0x61702f: SBServerListNth of the server list (+0x48).
// blam-cc: cdecl

#include "gamespy.h"

extern void *SBServerListNth(void *slist, int i);

void *ServerBrowserGetServer(void *sb, int index)
{
    return SBServerListNth((char *)sb + 0x48, index);
}
