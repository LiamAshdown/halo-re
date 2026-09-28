// ParseServerIPPort  (GameSpy SDK in halo.exe; no C existed)
// address 0x61eda0, size 50 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61eda0..0x61edd1: EAX data, EDX length, EBX ip out, ESI port out, stack list:
//   with 5 bytes, the ip after the flags byte; the port that follows when flag 0x10 says so (and 2 more bytes are
//   there), else the list  default port.
// blam-cc: EAX -> data, EDX -> len, EBX -> ip, ESI -> port, stack -> slist

#include "gamespy.h"

#include "sb.h"

void ParseServerIPPort(SBServerList *slist, const unsigned char *data, int len, unsigned int *ip,
    unsigned short *port)
{
    unsigned char flags;

    if (len < 5) {
        return;
    }
    flags = data[0];
    *ip = *(const unsigned int *)(data + 1);
    if (flags & 0x10) {
        if (len - 5 >= 2) {
            *port = *(const unsigned short *)(data + 5);
        }
    } else {
        *port = slist->defaultport;
    }
}
