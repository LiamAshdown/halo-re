// SBServerHasPrivateAddress  (GameSpy SDK in halo.exe; no C existed)
// address 0x617620, size 12 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617620..0x61762b: flags (+0x15) bit 2.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

int SBServerHasPrivateAddress(void *server)
{
    return FIELD(server, 0x15, unsigned char) & 2;
}
