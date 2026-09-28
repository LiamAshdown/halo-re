// SBEngineHaltUpdates  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e560, size 25 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e560..0x61e578: empties both lists.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

void SBEngineHaltUpdates(SBQueryEngine *engine)
{
    engine->pendinglist.last = 0;
    engine->pendinglist.first = 0;
    engine->pendinglist.count = 0;
    engine->querylist.last = 0;
    engine->querylist.first = 0;
    engine->querylist.count = 0;
}
