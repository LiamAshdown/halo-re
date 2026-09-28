// ArraySearch  (GameSpy SDK in halo.exe; no C existed)
// address 0x61df70, size 140 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61df70..0x61dffb: from from_index: a binary search when sorted (0x61dd40, EAX
//   count) else a linear one (0x61dd00, EBX count, comparator(key, element)); both helpers exist only for this and
//   are statics here. The index, or -1 for none / an empty or NULL array.
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

// 0x61dd00 (EBX count): the first element for which comparator(key, element) is 0, else NULL.
static void *array_lsearch(const void *key, void *base, int elem_size, ArrayCompareFn comparator, int count)
{
    int i;
    char *elem = (char *)base;

    for (i = 0; i < count; i++, elem += elem_size) {
        if (comparator(key, elem) == 0) {
            return (char *)base + i * elem_size;
        }
    }
    return 0;
}

int ArraySearch(DArray array, const void *key, ArrayCompareFn comparator, int from_index, int is_sorted)
{
    int found = 1;
    char *at;

    if (array == 0 || array->count == 0) {
        return -1;
    }
    if (is_sorted != 0) {
        at = (char *)array_bsearch(array->count - from_index, key, (char *)array->list + array->elemsize * from_index,
            array->elemsize, comparator, &found);
    } else {
        at = (char *)array_lsearch(key, (char *)array->list + array->elemsize * from_index, array->elemsize, comparator,
            array->count - from_index);
    }
    if (at == 0 || found == 0) {
        return -1;
    }
    return (int)(at - (char *)array->list) / array->elemsize;
}
