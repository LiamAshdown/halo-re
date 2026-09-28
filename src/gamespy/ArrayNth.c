// ArrayNth  (GameSpy SDK in halo.exe; no C existed)
// address 0x61dc00, size 16 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61dc00..0x61dc0f: the address of element n.
// blam-cc: cdecl

#include "gamespy.h"

void *ArrayNth(DArray array, int n)
{
    return (char *)array->list + array->elemsize * n;
}
