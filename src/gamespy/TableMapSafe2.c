// TableMapSafe2  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e2f0, size 67 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e2f0..0x61e332: ArrayMapBackwards2 over the buckets; the first element it
//   stops at, or NULL.
// blam-cc: cdecl

#include "gamespy.h"

void *TableMapSafe2(HashTable table, ArrayMapFn2 fn, void *client_data)
{
    int i;

    for (i = 0; i < table->nbuckets; i++) {
        void *elem = ArrayMapBackwards2(table->buckets[i], fn, client_data);

        if (elem != 0) {
            return elem;
        }
    }
    return 0;
}
