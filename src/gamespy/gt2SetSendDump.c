// gt2SetSendDump  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e550, size 12 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e550..0x61e55b: stores the send dump callback at socket +0x24
// (GTI2Socket.sendDumpCallback). The linker folded the identical SBQueryEngineSetPublicIP (engine +0x24) into it,
// so ServerBrowserNew calls this too. RENAMED 2026-09-28: this file was named gt2SetReceiveDump, swapped with
// 0x614850.
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void gt2SetSendDump(void *socket, void *callback)
{
    FIELD(socket, 0x24, void *) = callback;
}
