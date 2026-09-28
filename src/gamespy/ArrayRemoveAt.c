// ArrayRemoveAt  (GameSpy SDK in halo.exe; no C existed)
// address 0x61dc10, size 60 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61dc10..0x61dc4b: closes the gap over element n without freeing it.
// blam-cc: cdecl

#include "gamespy.h"

#define ELEM(array, i) ((void *)((char *)(array)->list + (i) * (array)->elemsize))

void ArrayRemoveAt(DArray array, int n)
{
    if (n < array->count - 1) {
        memmove(ELEM(array, n), ELEM(array, n + 1), (array->count - n - 1) * array->elemsize);
    }
    array->count--;
}
