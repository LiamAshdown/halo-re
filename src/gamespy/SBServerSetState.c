// SBServerSetState  (GameSpy SDK in halo.exe; no C existed)
// address 0x617b60, size 12 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617b60..0x617b6b: state byte (+0x14).
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void SBServerSetState(void *server, unsigned char state)
{
    FIELD(server, 0x14, unsigned char) = state;
}
