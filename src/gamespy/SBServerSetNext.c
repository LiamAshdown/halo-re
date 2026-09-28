// SBServerSetNext  (GameSpy SDK in halo.exe; no C existed)
// address 0x617670, size 12 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617670..0x61767b: the list link (+0x20).
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void SBServerSetNext(void *server, void *next)
{
    FIELD(server, 0x20, void *) = next;
}
