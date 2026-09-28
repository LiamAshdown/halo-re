// gt2GetSocketData  (GameSpy SDK in halo.exe; no C existed)
// address 0x614820, size 8 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x614820..0x614827: the user data at socket +0x30.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void *gt2GetSocketData(void *socket)
{
    return FIELD(socket, 0x30, void *);
}
