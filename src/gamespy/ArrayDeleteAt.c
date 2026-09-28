// ArrayDeleteAt  (GameSpy SDK in halo.exe; no C existed)
// address 0x61dec0, size 87 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61dec0..0x61df16: frees element n through the element free function, then
//   closes the gap.
// blam-cc: cdecl

#include "gamespy.h"

#define ELEM(array, i) ((void *)((char *)(array)->list + (i) * (array)->elemsize))

void ArrayDeleteAt(DArray array, int n)
{
    if (array->elemfreefn != 0) {
        array->elemfreefn(ELEM(array, n));
    }
    if (n < array->count - 1) {
        memmove(ELEM(array, n), ELEM(array, n + 1), (array->count - n - 1) * array->elemsize);
    }
    array->count--;
}
