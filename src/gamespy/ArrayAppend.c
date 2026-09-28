// ArrayAppend  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e070, size 26 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e070..0x61e089: inserts at the end; nothing for a NULL array.
// blam-cc: cdecl

#include "gamespy.h"

void ArrayAppend(DArray array, const void *new_elem)
{
    if (array != 0) {
        ArrayInsertAt(array, new_elem, array->count);
    }
}
