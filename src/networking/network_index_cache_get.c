// network_index_cache_get  (Ghidra: FUN_004e9d20; named per this rewrite)
// address 0x4e9d20, size 25 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md ("Thin guarded wrapper around hash_table_get that
// treats a -1 key as 'no entry'.").
// register convention: ECX -> key; the hash_table pointer itself is presumably EAX, dropped by
// the decompile since it is passed straight through to hash_table_get unexamined.
//   // blam-cc: ECX -> key
// UNSURE: the table argument (in_EAX in the original, forwarded to hash_table_get) is modeled
// here as an explicit parameter since Ghidra shows zero visible arguments at the call site.

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

// Returns hash_table_get(table, key), or 0 if key is -1 (no entry).
int32_t network_index_cache_get(hash_table *table, int32_t key)
    // blam-cc: ECX -> key
{
    if (key == -1) {
        return 0;
    }
    return hash_table_get(table, key);
}

#if 0
Original Ghidra decompilation (0x4e9d20), from tools/pack.py 0x4e9d20:

undefined4 FUN_004e9d20(void)

{
  undefined4 uVar1;
  int in_ECX;

  uVar1 = 0;
  if (in_ECX != -1) {
    uVar1 = hash_table_get();
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
