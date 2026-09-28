// ArrayNew  (GameSpy SDK in halo.exe; no C existed)
// address 0x61dba0, size 86 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61dba0..0x61dbf5: allocates the 0x18 byte array (grow-by 8 when 0), capacity =
//   grow-by, and the element list.
// blam-cc: cdecl

#include "gamespy.h"

DArray ArrayNew(int elem_size, int grow_by, ArrayElementFreeFn elem_free_fn)
{
    DArray array = (DArray)malloc(sizeof(DArrayImplementation));

    if (grow_by == 0) {
        grow_by = 8;
    }
    array->count = 0;
    array->capacity = grow_by;
    array->elemsize = elem_size;
    array->growby = grow_by;
    array->elemfreefn = elem_free_fn;
    array->list = grow_by != 0 ? malloc(elem_size * grow_by) : 0;
    return array;
}
