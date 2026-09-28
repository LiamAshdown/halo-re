// SBServerListAppendServer  (GameSpy SDK in halo.exe; no C existed)
// address 0x61f000, size 45 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61f000..0x61f02c: appends the server and reports it to the list callback
//   (+0x480) as event 0 (server added) with the instance (+0x484).
// blam-cc: cdecl

#include "gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

void SBServerListAppendServer(void *slist, void *server)
{
    ArrayAppend(FIELD(slist, 0x04, DArray), &server);
    FIELD(slist, 0x480, SBListCallBackFn)(slist, 0, server, FIELD(slist, 0x484, void *));
}
