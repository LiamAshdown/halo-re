// ArrayReplaceAt  (GameSpy SDK in halo.exe; no C existed)
// address 0x61df20, size 72 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61df20..0x61df67: frees element n through the element free function and copies
//   the new one over it.
// blam-cc: cdecl

#include "gamespy.h"

#define ELEM(array, i) ((void *)((char *)(array)->list + (i) * (array)->elemsize))

void ArrayReplaceAt(DArray array, const void *new_elem, int n)
{
    if (array->elemfreefn != 0) {
        array->elemfreefn(ELEM(array, n));
    }
    memcpy(ELEM(array, n), new_elem, array->elemsize);
}
