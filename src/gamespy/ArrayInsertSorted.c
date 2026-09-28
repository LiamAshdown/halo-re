// ArrayInsertSorted  (GameSpy SDK in halo.exe; no C existed)
// address 0x61de80, size 57 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61de80..0x61deb8: inserts at the binary-search insertion point (0x61dd40, which
//   takes the count in EAX and exists only for this and ArraySearch; a static here).
// blam-cc: cdecl

#include "gamespy.h"

// 0x61dd40 (EAX count): binary search over count elements from base; comparator(element, key). Returns the
//   insertion point; *found is set when an equal element was seen.
static void *array_bsearch(int count, const void *key, void *base, int elem_size, ArrayCompareFn comparator, int *found)
{
    int low = 0;
    int high = count - 1;

    *found = 0;
    while (low <= high) {
        int mid = (high + low) >> 1;
        int result = comparator((char *)base + mid * elem_size, key);

        if (result == 0) {
            *found = 1;
            high = mid - 1;
        } else if (result > 0) {
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }
    return (char *)base + low * elem_size;
}

void ArrayInsertSorted(DArray array, const void *new_elem, ArrayCompareFn comparator)
{
    int found;
    char *at = (char *)array_bsearch(array->count, new_elem, array->list, array->elemsize, comparator, &found);

    ArrayInsertAt(array, new_elem, (int)(at - (char *)array->list) / array->elemsize);
}
