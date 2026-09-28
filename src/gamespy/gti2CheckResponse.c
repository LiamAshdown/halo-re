// gti2CheckResponse  (GameSpy SDK in halo.exe; no C existed)
// address 0x620860, size 59 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x620860..0x62089a: 1 when the responses match everywhere but bytes 0 and 13.
// blam-cc: cdecl

#include "gamespy.h"

int gti2CheckResponse(const unsigned char *response1, const unsigned char *response2)
{
    int i;

    for (i = 0; i < 0x20; i++) {
        if (i != 0 && i != 0xd && response1[i] != response2[i]) {
            return 0;
        }
    }
    return 1;
}
