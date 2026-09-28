// SBServerListGetLANList  (GameSpy SDK in halo.exe; no C existed)
// address 0x61ff20, size 310 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61ff20..0x620055: (list, start port, end port, query version): disconnects a
//   connected list, opens a broadcast UDP socket (1 on failure) and to every port from start to end (at most 500
//   more) broadcasts fe fd 02 00 00 00 00 00 (version 1) or "\\echo\\test"; the list is LAN browsing (0) from now.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

int SBServerListGetLANList(SBServerList *slist, unsigned short startSearchPort, unsigned short endSearchPort,
    int queryversion)
{
    unsigned char query[8];
    BOOL broadcast = 1;
    struct sockaddr_in address;
    unsigned short port;

    query[0] = 0xfe;
    query[1] = 0xfd;
    query[2] = 2;
    query[3] = 0;
    query[4] = 0;
    query[5] = 0;
    query[6] = 0;
    query[7] = 0;
    if (slist->state != 1) {
        SBServerListDisconnect(slist);
    }
    slist->slsocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (slist->slsocket == INVALID_SOCKET ||
        setsockopt(slist->slsocket, SOL_SOCKET, SO_BROADCAST, (const char *)&broadcast, 4) != 0) {
        return 1;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = 0xffffffff;
    if ((int)endSearchPort - (int)startSearchPort > 500) {
        endSearchPort = (unsigned short)(startSearchPort + 500);
    }
    if (startSearchPort <= endSearchPort) {
        port = startSearchPort;
        do {
            address.sin_port = htons(port);
            if (queryversion == 1) {
                sendto(slist->slsocket, (const char *)query, 8, 0, (const struct sockaddr *)&address, 0x10);
            } else {
                sendto(slist->slsocket, "\\echo\\test", 10, 0, (const struct sockaddr *)&address, 0x10);
            }
            port++;
        } while (port <= endSearchPort);
    }
    slist->state = 0;
    slist->lanstarttime = current_time();
    return 0;
}
