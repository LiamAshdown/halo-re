// ArrayClear  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e000, size 99 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e000..0x61e062: deletes every element from the last down (free function, then
//   the (empty) gap close).
// blam-cc: cdecl

#include "gamespy.h"

#define ELEM(array, i) ((void *)((char *)(array)->list + (i) * (array)->elemsize))

void ArrayClear(DArray array)
{
    int i;

    for (i = array->count - 1; i >= 0; i--) {
        if (array->elemfreefn != 0) {
            array->elemfreefn(ELEM(array, i));
        }
        if (i < array->count - 1) {
            memmove(ELEM(array, i), ELEM(array, i + 1), (array->count - i - 1) * array->elemsize);
        }
        array->count--;
    }
}
