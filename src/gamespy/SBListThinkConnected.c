// SBListThinkConnected  (GameSpy SDK in halo.exe; no C existed)
// address 0x6201a0, size 196 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6201a0..0x620263: EAX list: with data waiting, recv into the rest of the 4 KB
//   buffer (closed or failed: callback 4 and disconnect, 3); once past the crypt header (connected, or a main-list
//   state past 0) the new bytes are decrypted; the main list is processed (its error returned), a connected list with
//   data handles ad-hoc messages.
// blam-cc: EAX -> slist

#include "gamespy.h"

#include "sb.h"

extern SBServer *SBNullServer; // 0x006a27f8

int SBListThinkConnected(SBServerList *slist)
{
    int oldlen;
    int received;

    if (!CanReceiveOnSocket(slist->slsocket)) {
        return 0;
    }
    oldlen = slist->inbufferlen;
    received = recv(slist->slsocket, (char *)slist->inbuffer + oldlen, 0x1000 - oldlen, 0);
    if (received == SOCKET_ERROR || received == 0) {
        slist->ListCallback(slist, 4, SBNullServer, slist->instance);
        SBServerListDisconnect(slist);
        return 3;
    }
    slist->inbufferlen += received;
    if (slist->state == 2 || slist->pstate > 0) {
        GOADecrypt(&slist->cryptkey, slist->inbuffer + oldlen, slist->inbufferlen - oldlen);
    }
    if (slist->state == 3) {
        int error = ProcessMainListData(slist);

        if (error != 0) {
            return error;
        }
    }
    if (slist->state == 2 && slist->inbufferlen > 0) {
        return ProcessAdHocData(slist);
    }
    return 0;
}
