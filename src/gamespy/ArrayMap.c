// ArrayMap  (GameSpy SDK in halo.exe; no C existed)
// address 0x61dc50, size 56 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61dc50..0x61dc87: calls fn(element, client data) for every element, first to
//   last.
// blam-cc: cdecl

#include "gamespy.h"

#define ELEM(array, i) ((void *)((char *)(array)->list + (i) * (array)->elemsize))

void ArrayMap(DArray array, ArrayMapFn fn, void *client_data)
{
    int i;

    for (i = 0; i < array->count; i++) {
        fn(ELEM(array, i), client_data);
    }
}
