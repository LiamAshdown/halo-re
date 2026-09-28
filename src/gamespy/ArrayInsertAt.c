// ArrayInsertAt  (GameSpy SDK in halo.exe; no C existed)
// address 0x61ddf0, size 137 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61ddf0..0x61de78: a full array grows by grow-by (realloc); the tail from n
//   moves up one and the element is copied in.
// blam-cc: cdecl

#include "gamespy.h"

#define ELEM(array, i) ((void *)((char *)(array)->list + (i) * (array)->elemsize))

void ArrayInsertAt(DArray array, const void *new_elem, int n)
{
    if (array->count == array->capacity) {
        array->capacity += array->growby;
        array->list = realloc(array->list, array->elemsize * array->capacity);
    }
    array->count++;
    if (n < array->count - 1) {
        memmove(ELEM(array, n + 1), ELEM(array, n), (array->count - n - 1) * array->elemsize);
    }
    memcpy(ELEM(array, n), new_elem, array->elemsize);
}
