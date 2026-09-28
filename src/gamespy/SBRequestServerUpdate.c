// SBRequestServerUpdate  (GameSpy SDK in halo.exe; no C existed)
// address 0x620380, size 131 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620380..0x620402: connects (option 2) when disconnected (3 when that fails),
//   then asks the master to have the server at (ip, port) report: length 9, type 1, ip, port.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

int SBRequestServerUpdate(SBServerList *slist, unsigned int ip, unsigned short port)
{
    unsigned char message[9];

    if (slist->state == 1) {
        SBServerListConnectAndQuery(slist, 0, 0, 2, 0);
        if (slist->state == 1) {
            return 3;
        }
    }
    *(unsigned short *)message = htons(9);
    message[2] = 1;
    *(unsigned int *)(message + 3) = ip;
    *(unsigned short *)(message + 7) = port;
    return SendWithRetry(slist, (const char *)message, 9);
}
