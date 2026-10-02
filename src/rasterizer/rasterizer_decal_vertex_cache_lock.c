// rasterizer_decal_vertex_cache_lock  (Ghidra: FUN_0051a770, unnamed)
// address 0x51a770, size 150 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: called by the decals module (0x45002d). Finds the decal block of the given decal
//   datum in the decal vertex cache (0x0071d1c0: data_array at +0x3c with 0x1c byte blocks whose
//   +8 is an offset, shifted left by the cache granularity at +0x2c), and locks the matching
//   byte range of the decal vertex buffer 0x0071d1bc (IDirect3DVertexBuffer9::Lock +0x2c,
//   flags 0). Offset and size are both scaled by 1.5 (the cache counts four vertex quads, the
//   buffer holds them expanded to six vertices) through __ftol. The lock state global
//   0x0069c632 reads 5 during the lock and 4 afterwards.
//   Spot-check fix (phase 4 review): rewritten from the raw code 0x51a770..0x51a805; the earlier
//   rewrite had no inputs, passed pointers to __ftol and returned 0 in every case. The decal
//   index arrives in EAX, the size on the stack, and the locked pointer is returned (NULL when
//   Lock fails).
// register convention: EAX = decal datum index, stack = byte_count; returns the locked pointer.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *rasterizer_decal_vertex_cache;                         // 0x0071d1bc IDirect3DVertexBuffer9
extern uint8_t *rasterizer_decal_vertex_cache_handle;               // 0x0071d1c0 UNSURE: +0x2c shift, +0x3c data_array
extern int16_t rasterizer_vertex_buffer_lock_state;                 // 0x0069c632

// __ftol (0x6391b4, input on the FPU stack, chops toward zero) is written as a (long long) cast below

typedef int32_t (__stdcall *d3d_lock_fn)(void *self, uint32_t offset, uint32_t size, void **data, uint32_t flags);

// blam-cc: EAX -> decal_index, stack -> byte_count
void *rasterizer_decal_vertex_cache_lock(uint32_t decal_index, int32_t byte_count)
{
    uint8_t *cache = rasterizer_decal_vertex_cache_handle;
    data_array *blocks = *(data_array **)(cache + 0x3c);
    uint32_t offset = *(uint32_t *)((uint8_t *)blocks->data + (decal_index & 0xffff) * 0x1c + 8)
                      << (*(uint32_t *)(cache + 0x2c) & 0x1f);
    void *buffer = rasterizer_decal_vertex_cache;
    void *data = 0;
    uint8_t succeeded = 1;
    int32_t size;

    rasterizer_vertex_buffer_lock_state = 5;
    size = (int32_t)(long long)((double)byte_count * 1.5);
    if (((d3d_lock_fn)(*(void ***)buffer)[0x2c / 4])(buffer, (uint32_t)(int32_t)(long long)((double)offset * 1.5), (uint32_t)size, &data, 0) < 0) {
        succeeded = 0;
    }
    rasterizer_vertex_buffer_lock_state = 4;
    return succeeded ? data : 0;
}

#if 0
Original Ghidra decompilation (0x51a770):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_0051a770(void)

{
  int *piVar1;
  undefined4 uVar2;
  int iVar3;
  undefined4 local_4;

  piVar1 = DAT_0071d1bc;
  local_4 = 0;
  iVar3 = *DAT_0071d1bc;
  _DAT_0069c632 = 5;
  uVar2 = __ftol(&local_4);
  uVar2 = __ftol(uVar2);
  iVar3 = (**(code **)(iVar3 + 0x2c))(piVar1,uVar2);
  _DAT_0069c632 = 4;
  if (iVar3 < 0) {
    return 0;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
