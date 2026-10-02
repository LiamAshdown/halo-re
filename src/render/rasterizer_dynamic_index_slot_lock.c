// rasterizer_dynamic_index_slot_lock  (Ghidra: FUN_00511e80; new name, evidence below)
// address 0x511e80, size 66 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: types/rasterizer.h rasterizer_dynamic_index_slot (element of the table at
//   0x006dd9e0, count 0x006e09e0) and rasterizer_dynamic_index_buffer (0x006e09e8,
//   IDirect3DIndexBuffer9). The vtable+0x2c call matches IDirect3DIndexBuffer9::Lock(offset,
//   size, &data, flags); this function is the first writer of slot.unknown_08 found so far (the
//   header's own note says "no writer found in this module" because it was written from the
//   rasterizer module's own functions, not this one).
// register convention: ECX = slot index, no other arguments.
//   // blam-cc: ECX -> slot_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *rasterizer_dynamic_index_buffer;                        // 0x006e09e8 IDirect3DIndexBuffer9
extern rasterizer_dynamic_index_slot rasterizer_dynamic_index_slots[]; // 0x006dd9e0

typedef int32_t (__stdcall *d3d_lock_fn)(void *self, uint32_t offset, uint32_t size, void **data, uint32_t flags); // COM: __stdcall (no add esp after 0x511eba)

// Locks the index range described by rasterizer_dynamic_index_slots[slot_index] (offsets scaled
// by 6, matching the buffer's index format) and returns the locked pointer, or 0 for slot index
// -1 (no slot).
void *rasterizer_dynamic_index_slot_lock(int32_t slot_index) // blam-cc: ECX -> slot_index
{
    rasterizer_dynamic_index_slot *slot;
    d3d_lock_fn lock;

    if (slot_index == -1) {
        return 0;
    }

    slot = &rasterizer_dynamic_index_slots[slot_index];
    lock = *(d3d_lock_fn *)((uint8_t *)*(void **)rasterizer_dynamic_index_buffer + 0x2c);
    lock(rasterizer_dynamic_index_buffer, slot->first_index * 6, slot->index_count * 6,
         (void **)&slot->locked_indices, 0x1000);
    return (void *)slot->locked_indices;
}

#if 0
Original Ghidra decompilation (0x511e80):

undefined4 FUN_00511e80(void)

{
  undefined4 uVar1;
  int in_ECX;

  uVar1 = 0;
  if (in_ECX != -1) {
    (**(code **)(*DAT_006e09e8 + 0x2c))
              (DAT_006e09e8,*(int *)(&DAT_006dd9e0 + in_ECX * 0xc) * 6,
               *(int *)(&DAT_006dd9e4 + in_ECX * 0xc) * 6,&DAT_006dd9e8 + in_ECX * 0xc,0x1000);
    uVar1 = *(undefined4 *)(&DAT_006dd9e8 + in_ECX * 0xc);
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
