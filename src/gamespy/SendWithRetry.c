// SendWithRetry  (GameSpy SDK in halo.exe; no C existed)
// address 0x6202b0, size 205 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6202b0..0x62037c: EAX list, stack data, length: sends to the master; after a
//   failed first try the list is dropped (as SBServerListDisconnect) and reconnected with option 2 (no servers) -- a
//   failed reconnect reports disconnected (4), disconnects and returns its error -- and sent once more (3 if that
//   fails too).
// blam-cc: EAX -> slist, stack -> data, len

#include "gamespy.h"

#include "sb.h"

extern SBServer *SBNullServer; // 0x006a27f8

int SendWithRetry(SBServerList *slist, const char *data, int len)
{
    int retry = 1;

    for (;;) {
        int error;

        retry--;
        if (send(slist->slsocket, data, len, 0) > 0) {
            return 0;
        }
        if (retry < 0) {
            return 3;
        }
        if (slist->inbuffer != 0) {
            free(slist->inbuffer);
        }
        slist->inbuffer = 0;
        slist->inbufferlen = 0;
        if (slist->slsocket != INVALID_SOCKET) {
            closesocket(slist->slsocket);
        }
        slist->slsocket = INVALID_SOCKET;
        slist->state = 1;
        FreeKeyList(slist);
        slist->expectedelements = -1;
        FreePopularValues(slist);
        error = SBServerListConnectAndQuery(slist, 0, 0, 2, 0);
        if (error != 0) {
            slist->ListCallback(slist, 4, SBNullServer, slist->instance);
            SBServerListDisconnect(slist);
            return error;
        }
    }
}
