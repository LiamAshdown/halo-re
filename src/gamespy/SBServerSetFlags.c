// SBServerSetFlags  (GameSpy SDK in halo.exe; no C existed)
// address 0x617b20, size 12 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617b20..0x617b2b: flags byte (+0x15).
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void SBServerSetFlags(void *server, unsigned char flags)
{
    FIELD(server, 0x15, unsigned char) = flags;
}
