// ParseServer  (GameSpy SDK in halo.exe; no C existed)
// address 0x61ede0, size 518 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61ede0..0x61efe5: EAX length, ECX data, EBX server, stack list, use popular
//   values: the flags byte goes to the server; past the address (and port with 0x10) come the private ip (2) and port
//   (0x20, else the default), the ICMP ip (8); with 0x40 one value per key-list key (short keys are network order;
//   strings may be popular-value indices when allowed, 0xff meaning an inline string) and the basic-keys state; with
//   0x80 inline key/value pairs to an empty key and the full-keys state. The bytes consumed.
// blam-cc: EAX -> len, ECX -> data, EBX -> server, stack -> slist, usepopularlist

#include "gamespy.h"

#include "sb.h"

int ParseServer(SBServerList *slist, SBServer *server, unsigned char *data, int len, int usepopularlist)
{
    unsigned char flags = data[0];
    int original = len;
    unsigned int privateip;
    unsigned short privateport;

    SBServerSetFlags(server, flags);
    data += 5;
    len -= 5;
    if (flags & 0x10) {
        data += 2;
        len -= 2;
    }
    if (flags & 2) {
        privateip = *(unsigned int *)data;
        data += 4;
        len -= 4;
    } else {
        privateip = 0;
    }
    if (flags & 0x20) {
        privateport = *(unsigned short *)data;
        data += 2;
        len -= 2;
    } else {
        privateport = slist->defaultport;
    }
    SBServerSetPrivateAddr(server, privateip, privateport);
    if (flags & 8) {
        unsigned int icmpip = *(unsigned int *)data;

        data += 4;
        len -= 4;
        SBServerSetICMPIP(server, icmpip);
    }
    if (flags & 0x40) {
        int count = ArrayLength(slist->keylist);
        int k;

        for (k = 0; k < count; k++) {
            SBKeyInfo *key = (SBKeyInfo *)ArrayNth(slist->keylist, k);

            if (key->type == 2) {
                SBServerAddIntKeyValue(server, key->name, ntohs(*(unsigned short *)data));
                data += 2;
                len -= 2;
            } else if (key->type == 1) {
                SBServerAddIntKeyValue(server, key->name, *data);
                data++;
                len--;
            } else if (key->type == 0) {
                int n;

                if (usepopularlist != 0) {
                    unsigned char index = *data++;

                    len--;
                    if (index != 0xff) {
                        SBServerAddKeyValue(server, key->name, slist->popularvalues[index]);
                        continue;
                    }
                }
                SBServerAddKeyValue(server, key->name, (const char *)data);
                n = (int)strlen((const char *)data) + 1;
                data += n;
                len -= n;
            }
        }
        SBServerSetState(server, (unsigned char)(SBServerGetState(server) | 1));
    }
    if (flags & 0x80) {
        while (*data != 0 && len > 0) {
            const char *keyname = (const char *)data;
            int n = (int)strlen(keyname) + 1;

            data += n;
            len -= n;
            SBServerAddKeyValue(server, keyname, (const char *)data);
            n = (int)strlen((const char *)data) + 1;
            data += n;
            len -= n;
        }
        len--;
        SBServerSetState(server, (unsigned char)(SBServerGetState(server) | 2));
    }
    return original - len;
}
