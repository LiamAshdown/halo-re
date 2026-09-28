// gt2NetworkToHostInt  (GameSpy SDK in halo.exe; no C existed)
// address 0x614890, size 11 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x614890..0x61489a: ntohl (WSOCK32 #14 through its delay-import thunk).
// blam-cc: cdecl

#include "gamespy.h"

unsigned int gt2NetworkToHostInt(unsigned int value)
{
    return ntohl(value);
}
