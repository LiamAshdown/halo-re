// gt2GetRemotePort  (GameSpy SDK in halo.exe; no C existed)
// address 0x6147d0, size 9 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6147d0..0x6147d8: the connection's remote port (+0x04).
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

unsigned short gt2GetRemotePort(void *connection)
{
    return FIELD(connection, 0x04, unsigned short);
}
