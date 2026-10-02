// network_index_cache_insert_if_free  (Ghidra: FUN_004e9cd0; named per this rewrite)
// address 0x4e9cd0, size 74 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md ("Inserts a value into an index-cache slot only if
// that slot is not already occupied and no existing hash mapping exists."); see
// network_index_cache_find_or_allocate_slot.c (this batch, 0x4e9c20) for the shared
// network_index_cache layout (slots array at +0x28), now declared in types/networking.h.
// register convention: EAX -> container, ECX -> key, stack -> slot.
//   // blam-cc: EAX -> container, ECX -> key, stack -> slot
// FIXED (register inputs, objdump): ECX carries key (read at 0x4e9ced, live across the call to
// hash_table_get, which never writes ecx -- confirmed against its own disassembly). Ghidra's
// extraout_ECX was this same incoming key, not a second value; the old "key_value" stack
// parameter was a phantom -- the function only ever reads one stack slot (slot).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif


extern int32_t hash_table_get(hash_table *table, uint32_t key); // 0x4f05e0, memory module
extern void hash_table_set_or_remove(hash_table *table, int32_t key, int32_t value); // 0x4f0530

// If container's cache slot is unoccupied (-1) and key is not already present in the hash
// table, binds slot to key and returns true.
uint8_t network_index_cache_insert_if_free(uint8_t *container, int32_t slot, int32_t key)
    // blam-cc: EAX -> container, ECX -> key, stack -> slot
{
    network_index_cache *cache;
    hash_table *table;
    int32_t *slot_ptr;

    cache = *(network_index_cache **)(container + 0x58);

    table = (hash_table *)cache->table;   // types/objects.h hash_table, embedded at cache+0x0c
    slot_ptr = &cache->slots[slot];
    if (*slot_ptr != -1) {
        return 0;
    }
    if (hash_table_get(table, key) == -1) {
        *slot_ptr = key;
        hash_table_set_or_remove(table, key, slot);
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4e9cd0), from tools/pack.py 0x4e9cd0:

undefined4 FUN_004e9cd0(int param_1)

{
  int *piVar1;
  int in_EAX;
  int iVar2;
  int extraout_ECX;

  piVar1 = (int *)(*(int *)(*(int *)(in_EAX + 0x58) + 0x28) + param_1 * 4);
  if (*piVar1 != -1) {
    return 0;
  }
  iVar2 = hash_table_get();
  if (iVar2 == -1) {
    *piVar1 = extraout_ECX;
    hash_table_set_or_remove(param_1);
    return 1;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
