// SBRefStrHashCleanup  (GameSpy SDK in halo.exe; no C existed)
// address 0x617370, size 66 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617370..0x6173b1: frees this thread's ref-string table once it is empty.
// blam-cc: cdecl

#include "gamespy.h"

extern __declspec(thread) HashTable g_SBRefStrList;

void SBRefStrHashCleanup(void)
{
    if (g_SBRefStrList != 0 && TableCount(g_SBRefStrList) == 0) {
        TableFree(g_SBRefStrList);
        g_SBRefStrList = 0;
    }
}
