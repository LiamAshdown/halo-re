// ServerListConnect  (GameSpy SDK in halo.exe; no C existed)
// address 0x61ea20, size 298 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61ea20..0x61eb49: ESI list: the master is SBOverrideMasterServer when set, else
//   "s1.ms01.hosthpc.com" (used as the sprintf format), port 28910; an unresolvable name gives 2. A TCP socket is
//   made when there is none (1 on failure); a failed connect closes it again (3); else 0.
// blam-cc: ESI -> slist

#include "gamespy.h"

#include "sb.h"

extern char *SBOverrideMasterServer; // 0x006a3278

int ServerListConnect(SBServerList *slist)
{
    char hostname[0x80];
    struct sockaddr_in address;

    if (SBOverrideMasterServer != 0) {
        strcpy(hostname, SBOverrideMasterServer);
    } else {
        sprintf(hostname, "s1.ms01.hosthpc.com");
    }
    address.sin_family = AF_INET;
    address.sin_port = htons(28910);
    address.sin_addr.s_addr = inet_addr(hostname);
    if (address.sin_addr.s_addr == INADDR_NONE) {
        struct hostent *host = gethostbyname(hostname);

        if (host == 0) {
            return 2;
        }
        address.sin_addr.s_addr = *(unsigned int *)host->h_addr_list[0];
    }
    if (slist->slsocket == INVALID_SOCKET) {
        slist->slsocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (slist->slsocket == INVALID_SOCKET) {
            return 1;
        }
    }
    if (connect(slist->slsocket, (const struct sockaddr *)&address, 0x10) != 0) {
        closesocket(slist->slsocket);
        slist->slsocket = INVALID_SOCKET;
        return 3;
    }
    return 0;
}
