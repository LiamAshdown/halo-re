// SBServerGetPrivateQueryPort  (GameSpy SDK in halo.exe; no C existed)
// address 0x617650, size 17 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617650..0x617660: ntohs of the private port (+0x0c).
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

unsigned short SBServerGetPrivateQueryPort(void *server)
{
    return ntohs(FIELD(server, 0x0c, unsigned short));
}
