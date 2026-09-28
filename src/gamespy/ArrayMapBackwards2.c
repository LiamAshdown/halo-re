// ArrayMapBackwards2  (GameSpy SDK in halo.exe; no C existed)
// address 0x61dcc0, size 58 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61dcc0..0x61dcf9: last to first, returns the first element for which fn returns
//   0 (else NULL).
// blam-cc: cdecl

#include "gamespy.h"

#define ELEM(array, i) ((void *)((char *)(array)->list + (i) * (array)->elemsize))

void *ArrayMapBackwards2(DArray array, ArrayMapFn2 fn, void *client_data)
{
    int i;

    for (i = array->count - 1; i >= 0; i--) {
        void *elem = ELEM(array, i);

        if (fn(elem, client_data) == 0) {
            return elem;
        }
    }
    return 0;
}
