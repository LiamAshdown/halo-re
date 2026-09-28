// ServerBrowserGetMyPublicIPAddr  (GameSpy SDK in halo.exe; no C existed)
// address 0x617060, size 11 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617060..0x61706a: the public ip the master reported (+0x4d8).
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

unsigned int ServerBrowserGetMyPublicIPAddr(void *sb)
{
    return FIELD(sb, 0x4d8, unsigned int);
}
