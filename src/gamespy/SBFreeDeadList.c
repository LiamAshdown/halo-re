// SBFreeDeadList  (GameSpy SDK in halo.exe; no C existed)
// address 0x61f140, size 60 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61f140..0x61f17b: frees every server on the dead list (+0x5bc) and empties it.
// blam-cc: cdecl

#include "gamespy.h"
#include "fn_gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))


void SBFreeDeadList(void *slist)
{
    void *server = FIELD(slist, 0x5bc, void *);

    if (server == 0) {
        return;
    }
    while (server != 0) {
        void *next = SBServerGetNext(server);

        SBServerFree(&server);
        server = next;
    }
    FIELD(slist, 0x5bc, void *) = server;
}
