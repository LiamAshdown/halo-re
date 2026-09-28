// hash_table_initialize  (reached only through a .data code pointer; no C existed)
// address 0x4f0470, size 70 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4f0470..0x4f04b5: ESI table, EAX bucket count: once (not yet initialized):
//   GlobalAlloc the buckets (8 bytes each, cleared), no entries, freelist or blocks; initialized.
// blam-cc: ESI -> table, EAX -> bucket_count

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern void *__stdcall GlobalAlloc(uint32_t flags, uint32_t bytes);

void hash_table_initialize(hash_table *table, int32_t bucket_count)
{
    int32_t i;

    if (table->initialized != 0) {
        return;
    }
    table->bucket_count = bucket_count;
    table->buckets = (hash_bucket *)GlobalAlloc(0, bucket_count * 8);
    for (i = 0; i < table->bucket_count; i++) {
        table->buckets[i].count = 0;
        table->buckets[i].first = 0;
    }
    table->entry_count = 0;
    table->freelist = 0;
    table->blocks = 0;
    table->initialized = 1;
}
