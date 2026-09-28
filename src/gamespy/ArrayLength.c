// ArrayLength  (GameSpy SDK in halo.exe; no C existed)
// address 0x6175f0, size 7 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6175f0..0x6175f6: the element count.
// blam-cc: cdecl

#include "gamespy.h"

int ArrayLength(DArray array)
{
    return array->count;
}
