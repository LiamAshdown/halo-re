// ArrayMapBackwards  (GameSpy SDK in halo.exe; no C existed)
// address 0x61dc90, size 47 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61dc90..0x61dcbe: calls fn(element, client data) for every element, last to
//   first.
// blam-cc: cdecl

#include "gamespy.h"

#define ELEM(array, i) ((void *)((char *)(array)->list + (i) * (array)->elemsize))

void ArrayMapBackwards(DArray array, ArrayMapFn fn, void *client_data)
{
    int i;

    for (i = array->count - 1; i >= 0; i--) {
        fn(ELEM(array, i), client_data);
    }
}
