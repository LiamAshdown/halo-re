// gti2ConnectionCompare  (GameSpy SDK in halo.exe; no C existed)
// address 0x61c320, size 38 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61c320..0x61c345: the connection table comparator: by remote ip, then (16-bit)
//   by remote port.
// blam-cc: cdecl

#include "gamespy.h"
#include "fn_gamespy.h"

#define FIELD(object, offset, type) (*(type *)((char *)(object) + (offset)))

int gti2ConnectionCompare(const void *elem1, const void *elem2)
{
    void *a = *(void *const *)elem1;
    void *b = *(void *const *)elem2;

    if (FIELD(a, 0x00, unsigned int) != FIELD(b, 0x00, unsigned int)) {
        return (int)(FIELD(a, 0x00, unsigned int) - FIELD(b, 0x00, unsigned int));
    }
    return (short)(FIELD(a, 0x04, unsigned short) - FIELD(b, 0x04, unsigned short));
}
