// network_index_cache_remove  (Ghidra: FUN_004e9d40; named per this rewrite)
// address 0x4e9d40, size 101 bytes
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md ("Removes an entry from the index-cache hash table
// if present, clearing its bound slot."); see network_index_cache_find_or_allocate_slot.c (this
// batch, 0x4e9c20) for the shared network_index_cache layout, now in types/networking.h.
// register convention: EAX -> container, ESI -> key (unaff_ESI in the decompile).
//   // blam-cc: EAX -> container, ESI -> key
// UNSURE: hash_table_set_or_remove's real signature, same caveat as
// network_index_cache_find_or_allocate_slot.c.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include "networking.h"


#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hash_table_set_or_remove(hash_table *table, int32_t key, int32_t value); // 0x4f0530

// If key is bound in container's cache hash table, clears its slot to -1, removes the hash
// mapping, and returns true; otherwise returns false.
uint8_t network_index_cache_remove(uint8_t *container, int32_t key)
    // blam-cc: EAX -> container, ESI -> key
{
    network_index_cache *cache;
    hash_table *table;
    int32_t abs_key;
    hash_node *node;

    cache = *(network_index_cache **)(container + 0x58);

    table = (hash_table *)cache->table;   // types/objects.h hash_table, embedded at cache+0x0c
    if (table->initialized == 1 && key != -1) {
        abs_key = key < 0 ? -key : key;
        for (node = table->buckets[abs_key % table->bucket_count].first; node != 0;
             node = node->next) {
            if (node->key == key) {
                if (node->value == -1) {
                    return 0;
                }
                cache->slots[node->value] = -1;
                hash_table_set_or_remove(table, key, -1);
                return 1;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4e9d40), from tools/pack.py 0x4e9d40:

undefined4 FUN_004e9d40(void)

{
  int iVar1;
  int *piVar2;
  int in_EAX;
  int iVar3;
  int unaff_ESI;

  iVar1 = *(int *)(in_EAX + 0x58);
  if ((*(char *)(iVar1 + 0xc) == '\x01') && (unaff_ESI != -1)) {
    iVar3 = unaff_ESI;
    if (unaff_ESI < 0) {
      iVar3 = -unaff_ESI;
    }
    for (piVar2 = *(int **)(*(int *)(iVar1 + 0x14) + 4 + (iVar3 % *(int *)(iVar1 + 0x10)) * 8);
        piVar2 != (int *)0x0; piVar2 = (int *)piVar2[2]) {
      if (*piVar2 == unaff_ESI) {
        if (piVar2[1] == -1) {
          return 0;
        }
        *(undefined4 *)(*(int *)(iVar1 + 0x28) + piVar2[1] * 4) = 0xffffffff;
        hash_table_set_or_remove(0xffffffff);
        return 1;
      }
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
