// ProcessPushServer  (GameSpy SDK in halo.exe; no C existed)
// address 0x61fae0, size 197 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61fae0..0x61fba4: stack data, length; ESI list: a pushed server record (4 when
//   under 5 bytes or unparsable) updates the known server at that address or a new one (5 when it cannot be
//   allocated), appended when new; the callback hears updated (1); 0.
// blam-cc: stack -> data, len; ESI -> slist

#include "gamespy.h"

#include "sb.h"

int ProcessPushServer(SBServerList *slist, unsigned char *data, int len)
{
    unsigned char flags;
    unsigned int ip;
    unsigned short port;
    int index;
    SBServer *server;

    if (len < 5) {
        return 4;
    }
    flags = data[0];
    ip = *(unsigned int *)(data + 1);
    if (flags & 0x10) {
        if (len - 5 < 2) {
            port = (unsigned short)len;
        } else {
            port = *(unsigned short *)(data + 5);
        }
    } else {
        port = slist->defaultport;
    }
    index = SBServerListFindServer(slist, ip, port);
    if (index == -1) {
        server = SBAllocServer(slist, ip, port);
        if (SBIsNullServer(server)) {
            return 5;
        }
    } else {
        server = *(SBServer **)ArrayNth(slist->servers, index);
    }
    if (ParseServer(slist, server, data, len, 0) < 0) {
        return 4;
    }
    if (index == -1) {
        SBServerListAppendServer(slist, server);
    }
    slist->ListCallback(slist, 1, server, slist->instance);
    return 0;
}
