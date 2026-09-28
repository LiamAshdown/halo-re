// QueueNextQueries  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e910, size 67 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e910..0x61e952: EAX engine: while below the update limit and anything is
//   pending, the front pending server is queried.
// blam-cc: EAX -> engine

#include "gamespy.h"

#include "sb.h"

void QueueNextQueries(SBQueryEngine *engine)
{
    while (engine->querylist.count < engine->maxupdates && engine->pendinglist.count > 0) {
        SBServer *server = engine->pendinglist.first;

        if (server != 0) {
            engine->pendinglist.first = server->next;
            if (engine->pendinglist.first == 0) {
                engine->pendinglist.last = 0;
            }
            engine->pendinglist.count--;
        }
        QEStartQuery(engine, server);
    }
}
