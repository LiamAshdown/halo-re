// rasterizer_dynamic_geometry_dispose  (Ghidra: FUN_0051bcd0)
// address 0x51bcd0, size 137 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: types/rasterizer.h rasterizer_dynamic_vertex_cache (0x006d98e8, 20 entries, stride
// 0x0c matches the "+3 ints" stride here); rasterizer_vertex_buffer_slot (0x007bf060, stride
// 0x14; this function reaches it through the pre-base pointer 0x007bf04c + handle*0x14, i.e.
// slots[handle - 1], exactly as out/phase4/rasterizer_types_notes.md describes for the
// 0x0071d258/0x0071d25c bookkeeping pair); rasterizer_dynamic_index_buffer (0x006e09e8).
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *rasterizer_device;                                      // 0x0071d174
extern rasterizer_dynamic_vertex_cache rasterizer_dynamic_vertex_caches[k_rasterizer_vertex_type_count]; // 0x006d98e8
extern rasterizer_vertex_buffer_slot rasterizer_vertex_buffer_slots[k_rasterizer_vertex_buffer_slots];   // 0x007bf060
extern int32_t rasterizer_vertex_buffer_slot_count;                  // 0x0071d25c
extern int32_t rasterizer_vertex_buffer_slot_high_water;             // 0x0071d258
extern void *rasterizer_dynamic_index_buffer;                        // 0x006e09e8

// Releases every dynamic vertex buffer currently checked out by the 20 per vertex type dynamic
// caches, clearing their vertex buffer slots and the associated bookkeeping counters, then
// releases the shared dynamic index buffer. Only runs while the device is alive.
void rasterizer_dynamic_geometry_dispose(void)
{
    int32_t type_index;
    int32_t handle;
    rasterizer_vertex_buffer_slot *slot;
    void **object;
    void (__stdcall **vtable)(void *); // COM methods are __stdcall

    if (rasterizer_device != 0) {
        for (type_index = 0; type_index < k_rasterizer_vertex_type_count; type_index++) {
            handle = rasterizer_dynamic_vertex_caches[type_index].buffer_handle;
            if (handle != 0) {
                slot = &rasterizer_vertex_buffer_slots[handle - 1];
                object = (void **)slot->hardware_buffer;
                if (object != 0) {
                    vtable = *(void (__stdcall ***)(void *))object;
                    vtable[2](object); // slot +8, Release()
                }
                rasterizer_vertex_buffer_slot_count = rasterizer_vertex_buffer_slot_count - 1;
                slot->hardware_buffer = 0;
                slot->vertex_type = 0;
                slot->length = 0;
                slot->fvf = 0;
                if (rasterizer_vertex_buffer_slot_count == 0) {
                    rasterizer_vertex_buffer_slot_high_water = 0;
                }
                rasterizer_dynamic_vertex_caches[type_index].buffer_handle = 0;
            }
        }
        if (rasterizer_dynamic_index_buffer != 0) {
            object = (void **)rasterizer_dynamic_index_buffer;
            vtable = *(void (__stdcall ***)(void *))object;
            vtable[2](object); // slot +8, Release()
            rasterizer_dynamic_index_buffer = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x51bcd0):

void FUN_0051bcd0(void)

{
  int iVar1;
  int *piVar2;
  int iVar3;
  int *piVar4;

  if (DAT_0071d174 != 0) {
    piVar4 = &DAT_006d98f0;
    iVar3 = 0x14;
    do {
      iVar1 = *piVar4;
      if (iVar1 != 0) {
        piVar2 = *(int **)(&DAT_007bf04c + iVar1 * 10);
        if (piVar2 != (int *)0x0) {
          (**(code **)(*piVar2 + 8))(piVar2);
        }
        DAT_0071d25c = DAT_0071d25c + -1;
        *(undefined4 *)(&DAT_007bf04c + iVar1 * 10) = 0;
        (&DAT_007bf050)[iVar1 * 5] = 0;
        *(undefined4 *)(&DAT_007bf054 + iVar1 * 0x14) = 0;
        *(undefined4 *)(&DAT_007bf058 + iVar1 * 0x14) = 0;
        if (DAT_0071d25c == 0) {
          DAT_0071d258 = 0;
        }
        *piVar4 = 0;
      }
      piVar4 = piVar4 + 3;
      iVar3 = iVar3 + -1;
    } while (iVar3 != 0);
    if (DAT_006e09e8 != (int *)0x0) {
      (**(code **)(*DAT_006e09e8 + 8))(DAT_006e09e8);
      DAT_006e09e8 = (int *)0x0;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
