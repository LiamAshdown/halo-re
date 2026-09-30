// NTSLengthSB  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e9f0, size 33 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e9f0..0x61ea10: the length of the NUL-terminated string at buf including its
//   NUL, or -1 when no NUL lies within len bytes.
// blam-cc: cdecl

#include "gamespy.h"
#include "fn_gamespy.h"

int NTSLengthSB(const char *buf, int len)
{
    int i = 0;

    if (len > 0) {
        do {
            if (buf[i++] == 0) {
                return i;
            }
        } while (i < len);
    }
    return -1;
}
