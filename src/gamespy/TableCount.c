// TableCount  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e140, size 48 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e140..0x61e16f: the element count over every bucket.
// blam-cc: cdecl

#include "gamespy.h"

int TableCount(HashTable table)
{
    int count = 0;
    int i;

    for (i = 0; i < table->nbuckets; i++) {
        count += ArrayLength(table->buckets[i]);
    }
    return count;
}
