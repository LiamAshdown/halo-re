// SBServerGetPing  (GameSpy SDK in halo.exe; no C existed)
// address 0x617aa0, size 8 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617aa0..0x617aa7: the ping (+0x1c).
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

int SBServerGetPing(void *server)
{
    return FIELD(server, 0x1c, int);
}
