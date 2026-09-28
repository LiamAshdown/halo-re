// TableMapSafe  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e2b0, size 61 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e2b0..0x61e2ec: ArrayMapBackwards over every bucket.
// blam-cc: cdecl

#include "gamespy.h"

void TableMapSafe(HashTable table, ArrayMapFn fn, void *client_data)
{
    int i;

    for (i = 0; i < table->nbuckets; i++) {
        ArrayMapBackwards(table->buckets[i], fn, client_data);
    }
}
