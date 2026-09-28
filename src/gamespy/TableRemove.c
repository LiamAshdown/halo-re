// TableRemove  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e1d0, size 79 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e1d0..0x61e21e: deletes an equal element from its bucket; whether one was
//   there.
// blam-cc: cdecl

#include "gamespy.h"

int TableRemove(HashTable table, const void *del_elem)
{
    int bucket = table->hashfn(del_elem, table->nbuckets);
    int index = ArraySearch(table->buckets[bucket], del_elem, table->compfn, 0, 0);

    if (index == -1) {
        return 0;
    }
    ArrayDeleteAt(table->buckets[bucket], index);
    return 1;
}
