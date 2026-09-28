// ghiAppendIntToBuffer  (GameSpy SDK in halo.exe; no C existed)
// address 0x622db0, size 62 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622db0..0x622ded: the number in decimal.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

int ghiAppendIntToBuffer(GHIBuffer *buffer, int i)
{
    char text[0x10];

    sprintf(text, "%d", i);
    return ghiAppendDataToBuffer(buffer, text, 0);
}
