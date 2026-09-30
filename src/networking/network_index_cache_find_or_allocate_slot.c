// network_index_cache_find_or_allocate_slot  (Ghidra: FUN_004e9c20; named per this rewrite)
// address 0x4e9c20, size 161 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md ("Looks up or allocates a free slot in a
// fixed-size hash-indexed table, evicting via a rotating cursor when no existing entry matches.");
// out/phase4/networking_types_notes.md's note on 0x4e9c20/0x4e9cd0/0x4e9d20/0x4e9d40 (a
// hash-indexed object-to-network-index cache; the underlying hash_table, types/objects.h, is
// generic and owned by another module, but the eviction cursor behaviour here is local); the
// sibling functions in this file group confirm the wrapper's hash_table is embedded at +0xc
// (matching object_network_id_table/remote_player_index_remap_table's own +0xc convention).
// register convention: EAX -> container (whose +0x58 holds the cache pointer), stack -> key.
//   // blam-cc: EAX -> container, stack -> key
// UNSURE: hash_table_set_or_remove's real signature; called here with only the evicted slot index
// visible, so EAX/ECX are assumed to carry the table and the new key (dropped by the decompile).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include "networking.h"
#include "fn_objects.h"


    // memory module; UNSURE: signature inferred, see file header

// Looks up key in container's index cache; if present, returns its cached slot. Otherwise scans
// forward from the cache's rotating cursor for a slot whose value is -1 (evicting/reusing it),
// binds key to that slot in the hash table, and returns it. Returns -1 if the whole table was
// scanned without finding a free slot.
int32_t network_index_cache_find_or_allocate_slot(uint8_t *container, int32_t key)
    // blam-cc: EAX -> container, stack -> key
{
    network_index_cache *cache;
    hash_table *table;
    int32_t slot;
    int32_t abs_key;
    hash_node *node;
    int32_t start_cursor;
    int32_t cursor;
    int32_t free_slot;

    cache = *(network_index_cache **)(container + 0x58);

    table = (hash_table *)cache->table;   // types/objects.h hash_table, embedded at cache+0x0c
    slot = -1;
    if (table->initialized == 1 && key != -1) {
        abs_key = key < 0 ? -key : key;
        for (node = table->buckets[abs_key % table->bucket_count].first; node != 0;
             node = node->next) {
            if (node->key == key) {
                slot = node->value;
                break;
            }
        }
    }
    if (slot != -1) {
        return slot;
    }

    start_cursor = cache->cursor;
    free_slot = -1;
    cursor = start_cursor;
    for (;;) {
        if (cache->slots[cursor] == -1) {
            free_slot = cursor;
        }
        cursor = cursor + 1;
        if (cache->capacity <= cursor) {
            cursor = 0;
        }
        cache->cursor = cursor;
        if (start_cursor == cursor) {
            break;
        }
        if (free_slot != -1) {
            hash_table_set_or_remove(table, key, free_slot);
            cache->slots[free_slot] = key;
            return free_slot;
        }
    }
    if (free_slot == -1) {
        return -1;
    }
    hash_table_set_or_remove(table, key, free_slot);
    cache->slots[free_slot] = key;
    return free_slot;
}

#if 0
Original Ghidra decompilation (0x4e9c20), from tools/pack.py 0x4e9c20:

int FUN_004e9c20(int param_1)

{
  int *piVar1;
  int *piVar2;
  int in_EAX;
  int iVar3;
  int iVar4;
  int iVar5;

  piVar1 = *(int **)(in_EAX + 0x58);
  iVar4 = -1;
  if (((char)piVar1[3] == '\x01') && (param_1 != -1)) {
    iVar3 = param_1;
    if (param_1 < 0) {
      iVar3 = -param_1;
    }
    for (piVar2 = *(int **)(piVar1[5] + 4 + (iVar3 % piVar1[4]) * 8); piVar2 != (int *)0x0;
        piVar2 = (int *)piVar2[2]) {
      if (*piVar2 == param_1) {
        iVar4 = piVar2[1];
        break;
      }
    }
  }
  if (iVar4 != -1) {
    return iVar4;
  }
  iVar4 = piVar1[9];
  iVar5 = -1;
  iVar3 = iVar4;
  while( true ) {
    if (*(int *)(piVar1[10] + iVar3 * 4) == -1) {
      iVar5 = iVar3;
    }
    piVar1[9] = iVar3 + 1;
    if (*piVar1 <= iVar3 + 1) {
      piVar1[9] = 0;
    }
    iVar3 = piVar1[9];
    if (iVar4 == iVar3) break;
    if (iVar5 != -1) {
LAB_004e9c9d:
      hash_table_set_or_remove(iVar5);
      *(int *)(piVar1[10] + iVar5 * 4) = param_1;
      return iVar5;
    }
  }
  if (iVar5 == -1) {
    return -1;
  }
  goto LAB_004e9c9d;
}
#endif
