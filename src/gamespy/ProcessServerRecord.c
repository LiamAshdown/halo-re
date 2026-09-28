// ProcessServerRecord  (GameSpy SDK in halo.exe; no C existed)
// address 0x61f5a0, size 244 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61f5a0..0x61f693: EAX data, stack length, list: 0 until the whole record
//   (header by flags, key values, rules) is there; -1 for the all-ones end-of-list address; otherwise the server is
//   allocated (-2 when that fails), parsed with popular values and appended; the bytes used.
// blam-cc: EAX -> data, stack -> len, slist

#include "gamespy.h"

#include "sb.h"

extern SBServer *SBNullServer; // 0x006a27f8

int ProcessServerRecord(SBServerList *slist, unsigned char *data, int len)
{
    unsigned char flags;
    int header;
    unsigned int ip;
    unsigned short port = (unsigned short)len;
    SBServer *server;
    int used;

    if (len < 1) {
        return 0;
    }
    flags = data[0];
    header = (flags & 2) ? 9 : 5;
    if (flags & 8) {
        header += 4;
    }
    if (flags & 0x10) {
        header += 2;
    }
    if (flags & 0x20) {
        header += 2;
    }
    if (len < header) {
        return 0;
    }
    if ((flags & 0x40) && !AllKeysPresent(slist, data + header, len - header)) {
        return 0;
    }
    if ((flags & 0x80) && !FullRulesPresent((const char *)data + header, len - header)) {
        return 0;
    }
    if (*(unsigned int *)(data + 1) == 0xffffffff) {
        return -1;
    }
    ParseServerIPPort(slist, data, len, &ip, &port);
    server = SBAllocServer(slist, ip, port);
    if (SBIsNullServer(server)) {
        return -2;
    }
    used = ParseServer(slist, server, data, len, 1);
    SBServerListAppendServer(slist, server);
    return used;
}
