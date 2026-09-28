// SBQueryEngineUpdateServer  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e5b0, size 128 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e5b0..0x61e62f: clears the pending bits (keeping 0x1c clear) and marks a
//   basic (type 0: 4) or full (1: 8) query; queried at once while below the update limit, else queued at the front or
//   back.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

void SBQueryEngineUpdateServer(SBQueryEngine *engine, SBServer *server, int addfront, int querytype)
{
    server->state &= 0xe3;
    if (querytype == 0) {
        server->state |= 4;
    } else if (querytype == 1) {
        server->state |= 8;
    }
    if (engine->querylist.count < engine->maxupdates) {
        QEStartQuery(engine, server);
        return;
    }
    if (addfront != 0) {
        server->next = engine->pendinglist.first;
        engine->pendinglist.first = server;
        if (engine->pendinglist.last == 0) {
            engine->pendinglist.last = server;
        }
    } else {
        if (engine->pendinglist.last != 0) {
            engine->pendinglist.last->next = server;
        }
        engine->pendinglist.last = server;
        server->next = 0;
        if (engine->pendinglist.first == 0) {
            engine->pendinglist.first = server;
        }
    }
    engine->pendinglist.count++;
}
