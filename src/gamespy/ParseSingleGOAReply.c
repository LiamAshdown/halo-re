// ParseSingleGOAReply  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e750, size 101 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e750..0x61e7b4: EBX data, EDI server, stack engine: the key/value pairs are
//   parsed; once the reply carries "\\final\\" the query is complete (basic, or full), the ping taken, it leaves the
//   query list and the callback hears success.
// blam-cc: EBX -> data, EDI -> server, stack -> engine

#include "gamespy.h"

#include "sb.h"

void ParseSingleGOAReply(SBQueryEngine *engine, SBServer *server, char *data)
{
    int isfinal = strstr(data, "\\final\\") != 0;

    SBServerParseKeyVals(server, data);
    if (!isfinal) {
        return;
    }
    if (server->state & 4) {
        server->state |= 1;
    } else {
        server->state |= 2;
    }
    server->state &= 0xf3;
    server->updatetime = current_time() - server->updatetime;
    FIFORemove(server, &engine->querylist);
    engine->ListCallback(engine, 0, server, engine->instance);
}
