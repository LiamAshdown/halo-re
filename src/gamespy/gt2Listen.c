// gt2Listen  (GameSpy SDK in halo.exe; no C existed)
// address 0x61c660, size 12 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61c660..0x61c66b: stores the connect-attempt callback at socket +0x1c.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void gt2Listen(void *socket, void *callback)
{
    FIELD(socket, 0x1c, void *) = callback;
}
