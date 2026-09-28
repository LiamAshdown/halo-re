// TableFree  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e100, size 58 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e100..0x61e139: frees every bucket, the bucket list and the table.
// blam-cc: cdecl

#include "gamespy.h"

void TableFree(HashTable table)
{
    int i;

    for (i = 0; i < table->nbuckets; i++) {
        ArrayFree(table->buckets[i]);
    }
    free(table->buckets);
    free(table);
}
