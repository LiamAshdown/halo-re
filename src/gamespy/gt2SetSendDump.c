// gt2SetSendDump  (GameSpy SDK in halo.exe; no C existed)
// address 0x614850, size 12 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x614850..0x61485b: stores the dump callback at socket +0x28.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void gt2SetSendDump(void *socket, void *callback)
{
    FIELD(socket, 0x28, void *) = callback;
}
