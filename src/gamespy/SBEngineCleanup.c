// SBEngineCleanup  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e580, size 43 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e580..0x61e5aa: closes the query socket (-1) and empties both lists.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

void SBEngineCleanup(SBQueryEngine *engine)
{
    closesocket(engine->querysock);
    engine->querysock = INVALID_SOCKET;
    engine->pendinglist.last = 0;
    engine->pendinglist.first = 0;
    engine->pendinglist.count = 0;
    engine->querylist.last = 0;
    engine->querylist.first = 0;
    engine->querylist.count = 0;
}
