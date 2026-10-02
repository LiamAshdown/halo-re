// network_buffer_pair_pool_clear  (Ghidra: FUN_004e3ed0; renamed -- see evidence)
// address 0x4e3ed0, size 89 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md ("Frees every entry of a small array of paired
// GlobalAlloc allocations and resets its bookkeeping globals, most likely tearing down a cached
// list of dynamically-allocated network buffers"). No caller was found in this batch or
// elsewhere in the codebase (out/functions.json lists one caller, outside this batch's evidence
// window), so the pool's producer/consumer is not identified.
// register convention: __cdecl, no arguments.
// UNSURE: this function's exact relationship to the growable_array-shaped ban_list bookkeeping
// (0x006b859c region, see network_banlist_save.c) is not established; the three globals here
// (0x006b85a8/ac/b0) are a separate three-field record (an element-size-like sentinel, a count,
// and a data pointer to size*8 bytes) that happens to sit right after ban_list's own fields in
// memory, but nothing in this batch proves they are related.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// network_buffer_pair is types/networking.h (0x08; folded out of this file by the review pass).

extern int32_t network_buffer_pair_pool; // 0x006b85a8, set to -1 here, UNSURE meaning
extern int32_t network_buffer_pair_pool_count; // 0x006b85ac
extern network_buffer_pair *network_buffer_pair_pool_data; // 0x006b85b0


// Frees both allocations of every entry in the pool, then resets the pool to empty (count and
// the unknown flag both -1, data pointer freed and cleared).
void network_buffer_pair_pool_clear(void)
{
    int32_t i;

    if (0 < network_buffer_pair_pool_count) {
        for (i = 0; i < network_buffer_pair_pool_count; i = i + 1) {
            GlobalFree(network_buffer_pair_pool_data[i].first);
            GlobalFree(network_buffer_pair_pool_data[i].second);
        }
    }
    network_buffer_pair_pool = -1;
    network_buffer_pair_pool_count = -1;
    if (network_buffer_pair_pool_data != 0) {
        GlobalFree(network_buffer_pair_pool_data);
        network_buffer_pair_pool_data = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4e3ed0), from tools/pack.py 0x4e3ed0:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004e3ed0(void)

{
  HGLOBAL pvVar1;
  int iVar2;

  iVar2 = 0;
  if (0 < DAT_006b85ac) {
    do {
      pvVar1 = DAT_006b85b0;
      GlobalFree(*(HGLOBAL *)((int)DAT_006b85b0 + iVar2 * 8));
      GlobalFree(*(HGLOBAL *)((int)pvVar1 + iVar2 * 8 + 4));
      iVar2 = iVar2 + 1;
    } while (iVar2 < DAT_006b85ac);
  }
  _DAT_006b85a8 = 0xffffffff;
  DAT_006b85ac = 0xffffffff;
  if (DAT_006b85b0 != (HGLOBAL)0x0) {
    GlobalFree(DAT_006b85b0);
    DAT_006b85b0 = (HGLOBAL)0x0;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
