// gt2NetworkToHostShort  (GameSpy SDK in halo.exe; no C existed)
// address 0x6148a0, size 11 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6148a0..0x6148aa: ntohs (WSOCK32 #15 through its delay-import thunk).
// blam-cc: cdecl

#include "gamespy.h"

unsigned short gt2NetworkToHostShort(unsigned short value)
{
    return ntohs(value);
}
