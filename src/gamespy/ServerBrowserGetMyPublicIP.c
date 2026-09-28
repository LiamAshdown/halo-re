// ServerBrowserGetMyPublicIP  (GameSpy SDK in halo.exe; no C existed)
// address 0x617040, size 17 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617040..0x617050: inet_ntoa of the public ip the master reported (+0x4d8).
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

char *ServerBrowserGetMyPublicIP(void *sb)
{
    struct in_addr address;

    address.s_addr = FIELD(sb, 0x4d8, unsigned int);
    return inet_ntoa(address);
}
