// SBServerListNth  (GameSpy SDK in halo.exe; no C existed)
// address 0x61f120, size 24 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61f120..0x61f137: the i-th server.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void *SBServerListNth(void *slist, int i)
{
    return *(void **)ArrayNth(FIELD(slist, 0x04, DArray), i);
}
