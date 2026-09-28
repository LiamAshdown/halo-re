// SBServerSetPrivateAddr  (GameSpy SDK in halo.exe; no C existed)
// address 0x617b30, size 21 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617b30..0x617b44: the private ip (+0x08) and port (+0x0c).
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void SBServerSetPrivateAddr(void *server, unsigned int ip, unsigned short port)
{
    FIELD(server, 0x08, unsigned int) = ip;
    FIELD(server, 0x0c, unsigned short) = port;
}
