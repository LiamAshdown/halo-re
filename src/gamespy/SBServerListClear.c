// SBServerListClear  (GameSpy SDK in halo.exe; no C existed)
// address 0x61f180, size 153 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61f180..0x61f218: moves every server onto the dead list, clears the array, then
//   frees the dead list.
// blam-cc: cdecl

#include "gamespy.h"
#include "fn_gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))


void SBServerListClear(void *slist)
{
    int count = ArrayLength(FIELD(slist, 0x04, DArray));
    int i;
    void *server;

    for (i = 0; i < count; i++) {
        server = *(void **)ArrayNth(FIELD(slist, 0x04, DArray), i);
        SBServerSetNext(server, FIELD(slist, 0x5bc, void *));
        FIELD(slist, 0x5bc, void *) = server;
    }
    ArrayClear(FIELD(slist, 0x04, DArray));
    server = FIELD(slist, 0x5bc, void *);
    if (server != 0) {
        while (server != 0) {
            void *next = SBServerGetNext(server);

            SBServerFree(&server);
            server = next;
        }
        FIELD(slist, 0x5bc, void *) = server;
    }
}
