// network_channel_list_new  (Ghidra: FUN_00441960, still unnamed -> renamed)
// address 0x441960, size 109 bytes
// name confidence: 0.45   rewrite confidence: 0.6
// evidence: out/phase4/networking_types_notes.md "network_channel_list (0x441960 allocate,
// 0x441a40 add, 0x441b00 remove, 0x4419d0 the mark-readable pass)": every field this function
// writes (fd_count, entries, capacity, last_index, unknown_110) matches the declared struct,
// and the "constructor refuses more than 0x40" note is this function's `in_AX < 0x41` check.
// register convention: requested capacity in AX, the low 16 bits of EAX (in_AX).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"


// blam-cc: requested capacity in AX (in_AX, low 16 bits of EAX)
// Allocates a network_channel_list and its parallel dedup array (GMEM_ZEROINIT, requested
// capacity * 4 bytes); frees the outer allocation and returns NULL if the capacity is 0x41 or
// higher or the inner allocation fails.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
network_channel_list *network_channel_list_new(int16_t requested_capacity)
{
    network_channel_list *list;
    void *entries;

    list = (network_channel_list *)GlobalAlloc(0, 0x114);
    if (list == 0) {
        return 0;
    }
    if (requested_capacity < 0x41) {
        list->fd_count = 0;
        entries = GlobalAlloc(0x40, requested_capacity * 4);
        list->entries = (network_receive_queue **)entries;
        if (entries != 0) {
            list->capacity = requested_capacity;
            list->last_index = -1;
            list->service_cursor = 0;
            return list;
        }
    }
    GlobalFree(list);
    return 0;
}

#if 0
Original Ghidra decompilation (0x441960):

undefined4 * FUN_00441960(void)

{
  short in_AX;
  undefined4 *hMem;
  HGLOBAL pvVar1;

  hMem = GlobalAlloc(0,0x114);
  if (hMem == (undefined4 *)0x0) {
    return (undefined4 *)0x0;
  }
  if (in_AX < 0x41) {
    *hMem = 0;
    pvVar1 = GlobalAlloc(0x40,in_AX * 4);
    hMem[0x41] = pvVar1;
    if (pvVar1 != (HGLOBAL)0x0) {
      hMem[0x42] = (int)in_AX;
      hMem[0x43] = 0xffffffff;
      hMem[0x44] = 0;
      return hMem;
    }
  }
  GlobalFree(hMem);
  return (undefined4 *)0x0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
