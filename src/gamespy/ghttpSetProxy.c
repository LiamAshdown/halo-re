// ghttpSetProxy  (GameSpy SDK in halo.exe; no C existed)
// address 0x622050, size 149 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622050..0x6220e4: drops the old proxy (ghiProxyAddress 0x007231e0, port
//   0x007231dc); a non-empty "host[:port]" is copied (goastrdup) with the port split off (atoi; a zero port rejects
//   the proxy) or port 80 by default. Returns 0 when the copy fails or the port is 0.
// blam-cc: cdecl

#include "gamespy.h"

extern char *ghiProxyAddress;          // 0x007231e0
extern unsigned short ghiProxyPort;    // 0x007231dc

int ghttpSetProxy(const char *server)
{
    char *colon;

    if (ghiProxyAddress != 0) {
        free(ghiProxyAddress);
        ghiProxyAddress = 0;
    }
    ghiProxyPort = 0;
    if (server == 0 || server[0] == 0) {
        return 1;
    }
    ghiProxyAddress = goastrdup(server);
    if (ghiProxyAddress == 0) {
        return 0;
    }
    colon = strchr(ghiProxyAddress, ':');
    if (colon == 0) {
        ghiProxyPort = 80;
        return 1;
    }
    *colon = 0;
    ghiProxyPort = (unsigned short)atoi(colon + 1);
    if (ghiProxyPort != 0) {
        return 1;
    }
    free(ghiProxyAddress);
    ghiProxyAddress = 0;
    return 0;
}
