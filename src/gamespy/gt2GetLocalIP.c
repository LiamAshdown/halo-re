// gt2GetLocalIP  (GameSpy SDK in halo.exe; no C existed)
// address 0x6147e0, size 8 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6147e0..0x6147e7: the socket's local ip (+0x04).
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

unsigned int gt2GetLocalIP(void *socket)
{
    return FIELD(socket, 0x04, unsigned int);
}
