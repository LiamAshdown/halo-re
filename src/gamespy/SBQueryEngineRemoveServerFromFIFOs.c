// SBQueryEngineRemoveServerFromFIFOs  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e9c0, size 39 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e9c0..0x61e9e6: out of the query list, else the pending one.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

int SBQueryEngineRemoveServerFromFIFOs(SBQueryEngine *engine, SBServer *server)
{
    if (FIFORemove(server, &engine->querylist)) {
        return 1;
    }
    return FIFORemove(server, &engine->pendinglist);
}
