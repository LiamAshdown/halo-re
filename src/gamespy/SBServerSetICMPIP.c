// SBServerSetICMPIP  (GameSpy SDK in halo.exe; no C existed)
// address 0x617b50, size 12 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617b50..0x617b5b: the icmp ip (+0x10).
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void SBServerSetICMPIP(void *server, unsigned int ip)
{
    FIELD(server, 0x10, unsigned int) = ip;
}
