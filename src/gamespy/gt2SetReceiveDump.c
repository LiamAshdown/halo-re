// gt2SetReceiveDump  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e550, size 12 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e550..0x61e55b: stores the receive dump callback at socket +0x24.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void gt2SetReceiveDump(void *socket, void *callback)
{
    FIELD(socket, 0x24, void *) = callback;
}
