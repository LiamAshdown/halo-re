// AllKeysPresent  (GameSpy SDK in halo.exe; no C existed)
// address 0x61ed00, size 145 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61ed00..0x61ed90: EAX length, ECX data, stack list: whether a value for every
//   key of the key list fits: a string is a popular-value index byte, or 0xff and a NUL-terminated string; a byte 1;
//   a short 2; other types fail.
// blam-cc: EAX -> len, ECX -> data, stack -> slist

#include "gamespy.h"

#include "sb.h"

int AllKeysPresent(SBServerList *slist, const unsigned char *data, int len)
{
    int count = ArrayLength(slist->keylist);
    int k;

    for (k = 0; k < count; k++) {
        int type = ((SBKeyInfo *)ArrayNth(slist->keylist, k))->type;

        if (type == 0) {
            unsigned char index;

            if (len < 1) {
                return 0;
            }
            index = *data++;
            len--;
            if (index == 0xff) {
                int i = 0;

                if (len <= 0) {
                    return 0;
                }
                do {
                    if (data[i++] == 0) {
                        break;
                    }
                    if (i >= len) {
                        return 0;
                    }
                } while (1);
                if (i == -1) {
                    return 0;
                }
                data += i;
                len -= i;
            }
        } else if (type == 1) {
            data++;
            len--;
        } else if (type == 2) {
            data += 2;
            len -= 2;
        } else {
            return 0;
        }
        if (len < 0) {
            return 0;
        }
    }
    return 1;
}
