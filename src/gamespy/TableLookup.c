// TableLookup  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e220, size 74 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e220..0x61e269: the equal element in its bucket, or NULL.
// blam-cc: cdecl

#include "gamespy.h"

void *TableLookup(HashTable table, const void *elem)
{
    int bucket = table->hashfn(elem, table->nbuckets);
    int index = ArraySearch(table->buckets[bucket], elem, table->compfn, 0, 0);

    if (index == -1) {
        return 0;
    }
    return ArrayNth(table->buckets[bucket], index);
}
