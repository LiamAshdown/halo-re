// SBServerDirectConnect  (GameSpy SDK in halo.exe; no C existed)
// address 0x617630, size 12 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617630..0x61763b: flags (+0x15) bit 1.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

int SBServerDirectConnect(void *server)
{
    return FIELD(server, 0x15, unsigned char) & 1;
}
