// SBServerHasBasicKeys  (GameSpy SDK in halo.exe; no C existed)
// address 0x6175c0, size 12 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6175c0..0x6175cb: state (+0x14) bit 1.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

int SBServerHasBasicKeys(void *server)
{
    return FIELD(server, 0x14, unsigned char) & 1;
}
