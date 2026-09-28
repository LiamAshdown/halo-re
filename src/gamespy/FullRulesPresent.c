// FullRulesPresent  (GameSpy SDK in halo.exe; no C existed)
// address 0x61eca0, size 87 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61eca0..0x61ecf6: ECX length, EDX data: whether the buffer holds whole
//   key/value string pairs up to an empty key.
// blam-cc: ECX -> len, EDX -> data

#include "gamespy.h"

#include "sb.h"

int FullRulesPresent(const char *data, int len)
{
    int i;

    if (len > 0) {
        do {
            if (*data == 0) {
                return 1;
            }
            for (i = 0; i < len && data[i] != 0; i++) {
            }
            if (i >= len) {
                return 0;
            }
            i++;
            data += i;
            len -= i;
            if (len <= 0) {
                return 0;
            }
            for (i = 0; i < len && data[i] != 0; i++) {
            }
            if (i >= len) {
                return 0;
            }
            i++;
            len -= i;
            data += i;
        } while (len > 0);
    }
    if (len == 0) {
        return 0;
    }
    return *data == 0;
}
