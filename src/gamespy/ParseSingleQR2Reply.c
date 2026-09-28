// ParseSingleQR2Reply  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e630, size 273 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e630..0x61e740: EAX data, EBX server, stack engine, len: only a reply of type
//   0; past the 5-byte header a basic query takes one string per engine key (qr2 key names), a full one parses all
//   keys (then basic and full are set). The pending bits clear, the ping is taken, it leaves the query list and the
//   callback hears success (0).
// blam-cc: EAX -> data, EBX -> server, stack -> engine, len

#include "gamespy.h"

#include "sb.h"

extern char *qr2_registered_key_list[0x100]; // 0x00683990

extern int NTSLengthSB(const char *buf, int len);

void ParseSingleQR2Reply(SBQueryEngine *engine, SBServer *server, char *data, int len)
{
    if (data[0] != 0) {
        return;
    }
    len -= 5;
    data += 5;
    if (server->state & 4) {
        int i;

        for (i = 0; i < engine->numserverkeys; i++) {
            int n = NTSLengthSB(data, len);

            if (n < 0) {
                break;
            }
            SBServerAddKeyValue(server, qr2_registered_key_list[engine->serverkeys[i]], data);
            len -= n;
            data += n;
        }
        server->state |= 1;
    } else {
        SBServerParseQR2FullKeysSingle(server, data, len);
        server->state |= 3;
    }
    server->state &= 0xf3;
    server->updatetime = current_time() - server->updatetime;
    FIFORemove(server, &engine->querylist);
    engine->ListCallback(engine, 0, server, engine->instance);
}
