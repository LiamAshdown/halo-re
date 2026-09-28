// ProcessLanData  (GameSpy SDK in halo.exe; no C existed)
// address 0x61fbb0, size 330 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61fbb0..0x61fcf9: every LAN reply (up to 0x1f3 bytes) from an unknown address
//   adds a server (flags 0x11) and the callback hears added (0); 5 when one cannot be allocated. After 2 s the LAN
//   socket closes and the list is disconnected (1).
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

int ProcessLanData(SBServerList *slist)
{
    char buffer[0x1f8];
    struct sockaddr_in from;
    int fromlen = 0x10;

    while (CanReceiveOnSocket(slist->slsocket)) {
        if (recvfrom(slist->slsocket, buffer, 0x1f3, 0, (struct sockaddr *)&from, &fromlen) != SOCKET_ERROR &&
            SBServerListFindServer(slist, from.sin_addr.s_addr, from.sin_port) == -1) {
            SBServer *server = SBAllocServer(slist, from.sin_addr.s_addr, from.sin_port);

            if (SBIsNullServer(server)) {
                return 5;
            }
            SBServerSetFlags(server, 0x11);
            ArrayAppend(slist->servers, &server);
            slist->ListCallback(slist, 0, server, slist->instance);
        }
    }
    if (current_time() - slist->lanstarttime > 2000) {
        closesocket(slist->slsocket);
        slist->slsocket = INVALID_SOCKET;
        slist->state = 1;
    }
    return 0;
}
