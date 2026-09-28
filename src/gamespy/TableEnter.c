// TableEnter  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e170, size 88 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e170..0x61e1c7: appends to the element's bucket, or replaces an equal element
//   (linear search).
// blam-cc: cdecl

#include "gamespy.h"

void TableEnter(HashTable table, const void *new_elem)
{
    int bucket = table->hashfn(new_elem, table->nbuckets);
    int index = ArraySearch(table->buckets[bucket], new_elem, table->compfn, 0, 0);

    if (index == -1) {
        ArrayAppend(table->buckets[bucket], new_elem);
    } else {
        ArrayReplaceAt(table->buckets[bucket], new_elem, index);
    }
}
