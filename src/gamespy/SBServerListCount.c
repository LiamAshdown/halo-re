// SBServerListCount  (GameSpy SDK in halo.exe; no C existed)
// address 0x61f110, size 16 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61f110..0x61f11f: the number of servers.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

int SBServerListCount(void *slist)
{
    return ArrayLength(FIELD(slist, 0x04, DArray));
}
