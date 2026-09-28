// ArrayFree  (GameSpy SDK in halo.exe; no C existed)
// address 0x61dda0, size 68 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61dda0..0x61dde3: frees every element through the element free function, then
//   the list and the array.
// blam-cc: cdecl

#include "gamespy.h"

#define ELEM(array, i) ((void *)((char *)(array)->list + (i) * (array)->elemsize))

void ArrayFree(DArray array)
{
    int i;

    for (i = 0; i < array->count; i++) {
        if (array->elemfreefn != 0) {
            array->elemfreefn(ELEM(array, i));
        }
    }
    free(array->list);
    free(array);
}
