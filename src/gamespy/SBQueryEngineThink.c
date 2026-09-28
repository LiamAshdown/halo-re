// SBQueryEngineThink  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e960, size 60 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e960..0x61e99b: with queries out: replies, timeouts, then pending queries;
//   when none are left the callback hears idle (2).
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

void SBQueryEngineThink(SBQueryEngine *engine)
{
    if (engine->querylist.count == 0) {
        return;
    }
    ProcessIncomingReplies(engine);
    TimeoutOldQueries(engine);
    if (engine->pendinglist.count > 0) {
        QueueNextQueries(engine);
    }
    if (engine->querylist.count == 0) {
        engine->ListCallback(engine, 2, 0, engine->instance);
    }
}
