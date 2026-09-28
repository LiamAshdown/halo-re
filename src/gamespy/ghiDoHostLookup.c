// ghiDoHostLookup  (GameSpy SDK in halo.exe; no C existed)
// address 0x620f10, size 143 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620f10..0x620f9e: progress, SocketStartUp, parse the URL (parse failed 3),
//   resolve the proxy or the server (lookup failed 4); then connecting (1).
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

extern char *ghiProxyAddress;             // 0x007231e0
extern unsigned short ghiProxyPort;       // 0x007231dc

void ghiDoHostLookup(GHIConnection *connection)
{
    const char *host;

    ghiCallProgressCallback(connection, 0, 0);
    SocketStartUp();
    if (!ghiParseURL(connection)) {
    connection->completed = 1;
    connection->result = 3;
        return;
    }
    host = ghiProxyAddress != 0 ? ghiProxyAddress : connection->serverAddress;
    connection->serverIP = inet_addr(host);
    if (connection->serverIP == INADDR_NONE) {
        struct hostent *entry = gethostbyname(host);

        if (entry == 0) {
    connection->completed = 1;
    connection->result = 4;
            return;
        }
        connection->serverIP = *(unsigned int *)entry->h_addr_list[0];
    }
    connection->state = 1;
    ghiCallProgressCallback(connection, 0, 0);
}
