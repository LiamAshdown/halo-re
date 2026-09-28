// gt2SetReceiveDump  (GameSpy SDK in halo.exe; no C existed)
// address 0x614850, size 12 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x614850..0x61485b: stores the receive dump callback at socket +0x28
// (GTI2Socket.receiveDumpCallback). RENAMED 2026-09-28: this file was named gt2SetSendDump, swapped with 0x61e550.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void gt2SetReceiveDump(void *socket, void *callback)
{
    FIELD(socket, 0x28, void *) = callback;
}
