// SBServerListRemoveAt  (GameSpy SDK in halo.exe; no C existed)
// address 0x61f0a0, size 103 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61f0a0..0x61f106: reports the server to the list callback as event 2 (server
//   deleted), deletes it from the array and pushes it onto the dead list (+0x5bc).
// blam-cc: cdecl

#include "gamespy.h"
#include "fn_gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))


void SBServerListRemoveAt(void *slist, int index)
{
    void *server = *(void **)ArrayNth(FIELD(slist, 0x04, DArray), index);

    FIELD(slist, 0x480, SBListCallBackFn)(slist, 2, server, FIELD(slist, 0x484, void *));
    ArrayDeleteAt(FIELD(slist, 0x04, DArray), index);
    SBServerSetNext(server, FIELD(slist, 0x5bc, void *));
    FIELD(slist, 0x5bc, void *) = server;
}
