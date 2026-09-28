// SBServerGetState  (GameSpy SDK in halo.exe; no C existed)
// address 0x617b70, size 8 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617b70..0x617b77: state byte (+0x14).
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

unsigned char SBServerGetState(void *server)
{
    return FIELD(server, 0x14, unsigned char);
}
