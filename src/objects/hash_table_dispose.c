// hash_table_dispose  (reached only through a .data code pointer; no C existed)
// address 0x4f04c0, size 104 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4f04c0..0x4f0527: EDI table: when initialized, clears the buckets, frees every
//   node block and its nodes (GlobalFree), then the buckets; everything zero.
// blam-cc: EDI -> table

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_objects.h"


void hash_table_dispose(hash_table *table)
{
    hash_node_block *block;
    int32_t i;

    if (table->initialized != 1) {
        return;
    }
    for (i = 0; i < table->bucket_count; i++) {
        table->buckets[i].first = 0;
        table->buckets[i].count = 0;
    }
    table->freelist = 0;
    block = table->blocks;
    while (block != 0) {
        hash_node_block *next = block->next;

        GlobalFree(block->nodes);
        block->nodes = 0;
        GlobalFree(block);
        block = next;
    }
    table->blocks = 0;
    GlobalFree(table->buckets);
    table->buckets = 0;
    table->bucket_count = 0;
    table->entry_count = 0;
    table->initialized = 0;
}
