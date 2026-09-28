// ServerBrowserBeginUpdate2  (GameSpy SDK in halo.exe; no C existed)
// address 0x617070, size 302 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x617070..0x61719d: (browser, async, disconnect on complete, basic fields, count,
//   filter, options, max servers): remembers the disconnect flag, rebuilds the engine key list and a "\\name" field
//   string (up to 255 chars) from the qr2 key names, and asks the master. Synchronous: thinks every 10 ms while the
//   main list comes in or queries are out and nothing failed. The last result.
// blam-cc: cdecl

#include "gamespy.h"

#include "sb.h"

extern char *qr2_registered_key_list[0x100]; // 0x00683990

int ServerBrowserBeginUpdate2(ServerBrowser *sb, int async, int disconnectOnComplete, const unsigned char *basicFields,
    int numBasicFields, const char *serverFilter, int updateOptions, int maxServers)
{
    char keylist[0x100];
    int len = 0;
    int i;
    int error;

    memset(keylist, 0, sizeof(keylist));
    sb->disconnectFlag = disconnectOnComplete;
    sb->engine.numserverkeys = 0;
    for (i = 0; i < numBasicFields; i++) {
        const char *name = qr2_registered_key_list[basicFields[i]];

        if ((int)strlen(name) + len + 1 >= 0x100) {
            break;
        }
        len += sprintf(keylist + len, "\\%s", name);
        SBQueryEngineAddQueryKey(&sb->engine, basicFields[i]);
    }
    error = SBServerListConnectAndQuery(&sb->list, keylist, serverFilter, updateOptions, maxServers);
    if (error == 0 && async == 0) {
        while (sb->list.state == 3 || (sb->engine.querylist.count > 0 && error == 0)) {
            msleep(10);
            SBQueryEngineThink(&sb->engine);
            error = SBListThink(&sb->list);
        }
    }
    return error;
}
