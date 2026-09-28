// SBServerGetPublicAddress  (GameSpy SDK in halo.exe; no C existed)
// address 0x6175e0, size 13 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6175e0..0x6175ec: inet_ntoa (WS2_32 #12) of the public ip (+0x00).
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

char *SBServerGetPublicAddress(void *server)
{
    struct in_addr address;

    address.s_addr = FIELD(server, 0x00, unsigned int);
    return inet_ntoa(address);
}
