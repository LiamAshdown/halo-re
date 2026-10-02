// rasterizer_bind_texture_d3d9  (Ghidra: FUN_00518680, unnamed)
// address 0x518680, size 57 bytes
// name confidence: 0.5   rewrite confidence: 0.95
// evidence: touches the bitmap in the texture cache (texture_cache_get(bitmap, 1, 1)) and hands
//   its hardware texture (BitmapData +0x28) to IDirect3DDevice9::SetTexture (+0x104).
// Spot-check fix (phase 4 review, texture bind helpers 0x518680..0x518a60 rewritten together
//   from the raw code): the earlier rewrite passed NULL where the binary forwards the
//   resolved BitmapData in ESI, called bitmap_group_get_bitmap_data and texture_cache_get
//   without their register arguments (EAX tag / DX frame index, EAX bitmap) and did not pass the bitmap to texture_cache_get.
// register convention: ESI = bitmap, stack = stage; returns a bool in AL (0 for a NULL bitmap or
//   a failed SetTexture).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void *rasterizer_device;                                     // 0x0071d174
// blam-cc: EAX -> bitmap
extern void *texture_cache_get(BitmapData *bitmap, uint8_t wait, uint8_t allocate_if_missing); // 0x00444550

typedef int32_t (__stdcall *d3d_set_texture_fn)(void *device, uint32_t stage, void *texture);

// blam-cc: ESI -> bitmap, stack -> stage
uint8_t rasterizer_bind_texture_d3d9(int16_t stage, BitmapData *bitmap)
{
    void **vtable;

    if (bitmap == 0) {
        return 0;
    }
    texture_cache_get(bitmap, 1, 1);
    vtable = *(void ***)rasterizer_device;
    if (((d3d_set_texture_fn)vtable[0x104 / 4])(rasterizer_device, (uint32_t)(int32_t)stage,
                                                  *(void **)&((struct BitmapData *)bitmap)->hardware_texture) < 0) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x518680):

undefined4 FUN_00518680(short param_1)

{
  int iVar1;
  int unaff_ESI;

  if (unaff_ESI != 0) {
    texture_cache_get(1,1);
    iVar1 = (**(code **)(*DAT_0071d174 + 0x104))
                      (DAT_0071d174,(int)param_1,*(undefined4 *)(unaff_ESI + 0x28));
    if (-1 < iVar1) {
      return 1;
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
