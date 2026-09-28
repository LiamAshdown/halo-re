// ProcessAdHocData  (GameSpy SDK in halo.exe; no C existed)
// address 0x620060, size 297 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620060..0x620188: EAX list: whole messages from a connected master (length,
//   type, data; over 4 KB is an error 4): 1 a pushed key list, 2 a pushed server, 3 echoed straight back (3 when that
//   fails), 4 a server deleted by address (under 6 bytes: 4). Each is cut from the buffer; an error disconnects with
//   callback 4 (disconnected).
// blam-cc: EAX -> slist

#include "gamespy.h"

#include "sb.h"

extern SBServer *SBNullServer; // 0x006a27f8

int ProcessAdHocData(SBServerList *slist)
{
    int error = 0;

    if (slist->inbufferlen < 3) {
        return 0;
    }
    do {
        unsigned short msglen = ntohs(*(unsigned short *)slist->inbuffer);
        unsigned char *data = slist->inbuffer;

        if (msglen > 0x1000) {
            error = 4;
            break;
        }
        if (slist->inbufferlen < (int)msglen) {
            return 0;
        }
        switch ((signed char)data[2]) {
        case 1:
            error = ProcessPushKeyList(slist, data + 3, msglen - 3);
            break;
        case 2:
            error = ProcessPushServer(slist, data + 3, msglen - 3);
            break;
        case 3:
            if (send(slist->slsocket, (const char *)data, msglen, 0) <= 0) {
                return 3;
            }
            break;
        case 4:
            if (msglen - 3 < 6) {
                error = 4;
            } else {
                int index = SBServerListFindServer(slist, *(unsigned int *)(data + 3), *(unsigned short *)(data + 7));

                if (index != -1) {
                    SBServerListRemoveAt(slist, index);
                }
                error = 0;
            }
            break;
        }
        slist->inbufferlen -= msglen;
        if (slist->inbufferlen != 0 && slist->inbuffer != 0) {
            memmove(slist->inbuffer, slist->inbuffer + msglen, slist->inbufferlen);
        }
        if (error != 0) {
            break;
        }
    } while (slist->inbufferlen >= 3);
    if (error == 0) {
        return 0;
    }
    slist->ListCallback(slist, 4, SBNullServer, slist->instance);
    SBServerListDisconnect(slist);
    return error;
}
