// TableMap  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e270, size 61 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e270..0x61e2ac: ArrayMap over every bucket.
// blam-cc: cdecl

#include "gamespy.h"

void TableMap(HashTable table, ArrayMapFn fn, void *client_data)
{
    int i;

    for (i = 0; i < table->nbuckets; i++) {
        ArrayMap(table->buckets[i], fn, client_data);
    }
}
