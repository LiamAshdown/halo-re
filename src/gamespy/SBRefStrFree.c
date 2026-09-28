// SBRefStrFree  (GameSpy SDK in halo.exe; no C existed)
// address 0x617360, size 14 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617360..0x61736d: the ref-string table's element free: free(the string).
// blam-cc: cdecl

#include "gamespy.h"

void SBRefStrFree(void *elem)
{
    free((void *)((SBKeyValuePair *)elem)->key);
}
