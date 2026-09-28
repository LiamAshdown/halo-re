// SBServerGetPrivateAddress  (GameSpy SDK in halo.exe; no C existed)
// address 0x617640, size 14 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617640..0x61764d: inet_ntoa (WS2_32 #12) of the private ip (+0x08).
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

char *SBServerGetPrivateAddress(void *server)
{
    struct in_addr address;

    address.s_addr = FIELD(server, 0x08, unsigned int);
    return inet_ntoa(address);
}
