// ServerBrowserUpdate  (GameSpy SDK in halo.exe; no C existed)
// address 0x6171a0, size 43 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6171a0..0x6171ca: ServerBrowserBeginUpdate2 with no options and no server
//   limit.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

extern int ServerBrowserBeginUpdate2(ServerBrowser *sb, int async, int disconnectOnComplete,
    const unsigned char *basicFields, int numBasicFields, const char *serverFilter, int updateOptions, int maxServers);

int ServerBrowserUpdate(ServerBrowser *sb, int async, int disconnectOnComplete, const unsigned char *basicFields,
    int numBasicFields, const char *serverFilter)
{
    return ServerBrowserBeginUpdate2(sb, async, disconnectOnComplete, basicFields, numBasicFields, serverFilter, 0, 0);
}
