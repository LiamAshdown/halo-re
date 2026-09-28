// SBQueryEngineInit  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e4f0, size 81 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e4f0..0x61e540: SocketStartUp; version, max updates, no keys, callback and
//   instance, no public ip, a UDP query socket, empty lists.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

void SBQueryEngineInit(SBQueryEngine *engine, int maxupdates, int queryversion, SBEngineCallbackFn callback,
    void *instance)
{
    SocketStartUp();
    engine->queryversion = queryversion;
    engine->maxupdates = maxupdates;
    engine->numserverkeys = 0;
    engine->ListCallback = callback;
    engine->instance = instance;
    engine->mypublicip = 0;
    engine->querysock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    engine->pendinglist.last = 0;
    engine->pendinglist.first = 0;
    engine->pendinglist.count = 0;
    engine->querylist.last = 0;
    engine->querylist.first = 0;
    engine->querylist.count = 0;
}
