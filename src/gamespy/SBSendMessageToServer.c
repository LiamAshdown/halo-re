// SBSendMessageToServer  (GameSpy SDK in halo.exe; no C existed)
// address 0x620410, size 175 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620410..0x6204be: connects (option 2) when disconnected (3 when that fails),
//   then forwards data to the server through the master: length 9 + len, type 2, ip, port, then the data (3 when that
//   send fails).
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

int SBSendMessageToServer(SBServerList *slist, unsigned int ip, unsigned short port, const char *data, int len)
{
    unsigned char header[9];
    int error;

    if (slist->state == 1) {
        SBServerListConnectAndQuery(slist, 0, 0, 2, 0);
        if (slist->state == 1) {
            return 3;
        }
    }
    *(unsigned short *)header = htons((unsigned short)(len + 9));
    header[2] = 2;
    *(unsigned int *)(header + 3) = ip;
    *(unsigned short *)(header + 7) = port;
    error = SendWithRetry(slist, (const char *)header, 9);
    if (error != 0) {
        return error;
    }
    return send(slist->slsocket, data, len, 0) >= 0 ? 0 : 3;
}
