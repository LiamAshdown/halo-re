// TableNew2  (GameSpy SDK in halo.exe; no C existed)
// address 0x61e090, size 104 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61e090..0x61e0f7: a 0x14 byte table with num_buckets chains
//   (ArrayNew(elem_size, chains per bucket, free_fn)).
// blam-cc: cdecl

#include "gamespy.h"

HashTable TableNew2(int elem_size, int num_buckets, int num_chains_per_bucket, TableHashFn hash_fn,
    ArrayCompareFn comp_fn, ArrayElementFreeFn free_fn)
{
    HashTable table = (HashTable)malloc(sizeof(HashImplementation));
    int i;

    table->buckets = (DArray *)malloc(num_buckets * sizeof(DArray));
    for (i = 0; i < num_buckets; i++) {
        table->buckets[i] = ArrayNew(elem_size, num_chains_per_bucket, free_fn);
    }
    table->hashfn = hash_fn;
    table->nbuckets = num_buckets;
    table->freefn = free_fn;
    table->compfn = comp_fn;
    return table;
}
