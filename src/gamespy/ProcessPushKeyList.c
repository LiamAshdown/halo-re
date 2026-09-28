// ProcessPushKeyList  (GameSpy SDK in halo.exe; no C existed)
// address 0x61fa10, size 203 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61fa10..0x61fada: EAX data, ECX list, stack length: a pushed key list replaces
//   the old one (count, then type byte and name per key): 5 out of memory, 4 when truncated, else 0.
// blam-cc: EAX -> data, ECX -> slist, stack -> len

#include "gamespy.h"

#include "sb.h"

int ProcessPushKeyList(SBServerList *slist, unsigned char *data, int len)
{
    int count = data[0];
    int k;

    data++;
    len--;
    if (slist->keylist != 0) {
        FreeKeyList(slist);
    }
    slist->keylist = ArrayNew(sizeof(SBKeyInfo), count, 0);
    if (slist->keylist == 0) {
        return 5;
    }
    for (k = 0; k < count; k++) {
        SBKeyInfo key;
        int i;
        int n;

        if (len < 2) {
            return 4;
        }
        for (i = 0; i < len - 1 && data[1 + i] != 0; i++) {
        }
        if (i >= len - 1) {
            return 4;
        }
        n = i + 1;
        if (n == -1) {
            return 4;
        }
        key.type = data[0];
        key.name = SBRefStr(slist, (const char *)data + 1);
        ArrayAppend(slist->keylist, &key);
        len += -1 - n;
        data += n + 1;
    }
    return 0;
}
