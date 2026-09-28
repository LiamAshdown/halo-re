// SBServerGetPublicQueryPort  (GameSpy SDK in halo.exe; no C existed)
// address 0x617600, size 17 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617600..0x617610: ntohs of the public port (+0x04).
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

unsigned short SBServerGetPublicQueryPort(void *server)
{
    return ntohs(FIELD(server, 0x04, unsigned short));
}
