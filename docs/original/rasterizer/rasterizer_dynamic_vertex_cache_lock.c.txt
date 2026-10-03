// rasterizer_dynamic_vertex_cache_lock  (Ghidra: FUN_0051be40, house name
// rasterizer_dynamic_index_cache_lock; per out/phase4/rasterizer_types_notes.md this locks a
// VERTEX slot range, not an index range)
// address 0x51be40, size 113 bytes
// name confidence: 0.6   rewrite confidence: 0.55
// evidence: types/rasterizer.h rasterizer_dynamic_vertex_slot (0x006d99d8, stride 0x10;
// locked_vertices at +0x0c is documented as "void* Lock result, NULL on failure"),
// rasterizer_dynamic_vertex_cache (0x006d98e8, buffer_handle at +0x08),
// rasterizer_vertex_buffer_slot (0x007bf060, hardware_buffer at +0x00), rasterizer_vertex_sizes
// (0x0065de00, int16 per vertex type). The vtable call at offset 0x2c is
// IDirect3DVertexBuffer9::Lock (IDirect3DResource9 has 11 methods ending at 0x2c).
// register convention: slot index in EAX.
// UNSURE: raw Ghidra renders the return value as `(uint)puVar4 & (iVar2 < 0) - 1`, i.e. the
// *address* of the local that was passed as Lock's ppbData out-parameter, not its dereferenced
// value -- almost certainly a decompiler mislabel (returning a stack address as a lock result
// would corrupt every caller). Rewritten as the locked pointer itself, matching the field's own
// "Lock result" documentation in types/rasterizer.h.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
typedef int32_t (__stdcall *d3d_vertex_buffer_lock_fn)(void *self, uint32_t offset, uint32_t size,
                                              void **out_data, uint32_t flags);

extern rasterizer_dynamic_vertex_slot rasterizer_dynamic_vertex_slots[k_rasterizer_dynamic_vertex_slots]; // 0x006d99d8
extern rasterizer_dynamic_vertex_cache rasterizer_dynamic_vertex_caches[k_rasterizer_vertex_type_count];  // 0x006d98e8
extern rasterizer_vertex_buffer_slot rasterizer_vertex_buffer_slots[k_rasterizer_vertex_buffer_slots];    // 0x007bf060
extern int16_t rasterizer_vertex_sizes[k_rasterizer_vertex_type_count];                                   // 0x0065de00

// Locks the vertex buffer range covered by dynamic vertex slot `slot_index` (see
// rasterizer_dynamic_vertex_cache_reserve) and stores the resulting pointer (or NULL on failure)
// into the slot's locked_vertices field, which is also the return value.
// blam-cc: EAX = slot_index
void *rasterizer_dynamic_vertex_cache_lock(int32_t slot_index)
{
    rasterizer_dynamic_vertex_slot *slot;
    rasterizer_dynamic_vertex_cache *cache;
    rasterizer_vertex_buffer_slot *buffer_slot;
    void **vtable;
    d3d_vertex_buffer_lock_fn lock;
    void *locked_data;
    int32_t hresult;
    int32_t stride;

    if (slot_index == -1) {
        return 0;
    }

    slot = &rasterizer_dynamic_vertex_slots[slot_index];
    cache = &rasterizer_dynamic_vertex_caches[slot->vertex_type];
    buffer_slot = &rasterizer_vertex_buffer_slots[cache->buffer_handle - 1];
    stride = rasterizer_vertex_sizes[slot->vertex_type];

    locked_data = 0;
    vtable = *(void ***)(void *)buffer_slot->hardware_buffer;
    lock = (d3d_vertex_buffer_lock_fn)vtable[0xb]; // +0x2c, IDirect3DVertexBuffer9::Lock
    hresult = lock((void *)buffer_slot->hardware_buffer, slot->first_vertex * stride,
                    slot->vertex_count * stride, &locked_data, 0x2000);

    slot->locked_vertices = (uint32_t)(hresult < 0 ? 0 : locked_data);
    return (void *)slot->locked_vertices;
}

#if 0
Original Ghidra decompilation (0x51be40):

uint FUN_0051be40(void)

{
  int in_EAX;
  uint uVar1;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  undefined4 local_4;

  uVar1 = 0;
  if (in_EAX != -1) {
    iVar3 = in_EAX * 0x10;
    puVar4 = &local_4;
    local_4 = 0;
    iVar2 = (**(code **)(**(int **)(&DAT_007bf04c +
                                   (&DAT_006d98f0)[*(short *)(&DAT_006d99d8 + iVar3) * 3] * 10) +
                        0x2c))(*(int **)(&DAT_007bf04c +
                                        (&DAT_006d98f0)[*(short *)(&DAT_006d99d8 + iVar3) * 3] * 10)
                               ,*(int *)(&DAT_006d99dc + iVar3) *
                                (int)*(short *)(&DAT_0065de00 +
                                               *(short *)(&DAT_006d99d8 + iVar3) * 2),
                               *(int *)(&DAT_006d99e0 + iVar3) *
                               (int)*(short *)(&DAT_0065de00 + *(short *)(&DAT_006d99d8 + iVar3) * 2
                                              ),puVar4,0x2000);
    uVar1 = (uint)puVar4 & (iVar2 < 0) - 1;
    *(uint *)(&DAT_006d99e4 + iVar3) = uVar1;
  }
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
