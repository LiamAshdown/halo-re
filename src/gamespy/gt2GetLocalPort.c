// gt2GetLocalPort  (GameSpy SDK in halo.exe; no C existed)
// address 0x6147f0, size 9 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6147f0..0x6147f8: the socket's local port (+0x08).
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

unsigned short gt2GetLocalPort(void *socket)
{
    return FIELD(socket, 0x08, unsigned short);
}
